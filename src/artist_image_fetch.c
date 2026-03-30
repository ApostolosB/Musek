/*
 * artist_image_fetch.c
 *
 * Google Images-based artist thumbnail fetcher
 * modeled after Rage's albumart.c
 */
#include "player.h"
#include "ui_internal.h"


#include "artist_image_fetch.h"   /* for Artist_Image_Fetch_Done_Cb */

static Eina_List *artist_queue = NULL;
static Eina_Bool queue_running = EINA_FALSE;

static void _artist_queue_next(void);
static void _artist_queue_done(const char *path, void *data);
/* Forward declaration from ui_populate.c */
void populate_artists_grid(Player_State *ps);


/* --------------------------------------------------------------------- */
/* Google query                                                          */
/* --------------------------------------------------------------------- */

#define Q_START "http://www.google.com/search?as_st=y&tbm=isch&hl=en&as_q="
#define Q_END "&as_epq=&as_oq=&as_eq=&cr=&as_sitesearch=&safe=images&tbs=ift:jpg"


#define USER_AGENT \
  "Mozilla/5.0 (Linux; Android 4.0.4; Galaxy Nexus Build/IMM76B) " \
  "AppleWebKit/535.19 (KHTML, like Gecko) Chrome/18.0.1025.133 " \
  "Mobile Safari/535.19"

/* --------------------------------------------------------------------- */
/* Globals                                                               */
/* --------------------------------------------------------------------- */

static Ecore_Con_Url *fetch = NULL;
static Eina_Bool      fetch_image = EINA_FALSE;
static FILE          *fout = NULL;
static Eina_Strbuf   *sb_result = NULL;

static char *fetchpath  = NULL;   /* final thumb path */
static char *fetchpath2 = NULL;   /* temp path        */
static char *fetch_artist = NULL;

static Ecore_Event_Handler *handle_data     = NULL;
static Ecore_Event_Handler *handle_complete = NULL;

static Artist_Image_Fetch_Done_Cb _fetch_done_cb   = NULL;
static void                      *_fetch_done_data = NULL;



/* --------------------------------------------------------------------- */
/* Helpers                                                               */
/* --------------------------------------------------------------------- */

typedef struct {
    char *artist;
    Player_State *ps;
} Artist_Queue_Item;


static Ecore_Con_Url *
_fetch(Eina_Strbuf *sb)
{
   Ecore_Con_Url *f;
   const char *qs;

   qs = eina_strbuf_string_get(sb);
   if (!qs) return NULL;

   f = ecore_con_url_new(qs);
   if (!f) return NULL;

   ecore_con_url_additional_header_add(f, "user-agent", USER_AGENT);
   ecore_con_url_get(f);

   return f;
}

static Eina_Bool
_search_append(Eina_Strbuf *sb, const char *str, Eina_Bool hadword)
{
   const char *s;
   Eina_Bool word = EINA_FALSE;

   for (s = str; *s; s++)
     {
        if (((*s >= 'a') && (*s <= 'z')) ||
            ((*s >= 'A') && (*s <= 'Z')) ||
            ((*s >= '0') && (*s <= '9')))
          {
             if (!word)
               {
                  if (hadword)
                    {
                       eina_strbuf_append_char(sb, '+');
                       word = EINA_FALSE;
                    }
               }
             eina_strbuf_append_char(sb, *s);
             word = EINA_TRUE;
             hadword = EINA_TRUE;
          }
        else word = EINA_FALSE;
        if (*s == '.') break;
     }
   return hadword;
}

/* --------------------------------------------------------------------- */
/* HTTP callbacks                                                        */
/* --------------------------------------------------------------------- */

static Eina_Bool
_cb_http_data(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Con_Event_Url_Data *ev = event;

   if (ev->url_con != fetch) return EINA_TRUE;

   if (fetch_image)
     {
        if (fout) fwrite(ev->data, ev->size, 1, fout);
     }
   else if (sb_result)
     {
        eina_strbuf_append_length(sb_result,
                                  (const char *)ev->data,
                                  (size_t)ev->size);
     }

   return EINA_FALSE;
}

static Eina_Bool _delay_fetch(void *data);

static Eina_Bool
_cb_http_complete(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
    static int retry = 0;

    Ecore_Con_Event_Url_Complete *ev = event;
    Eina_Bool ok = EINA_FALSE;

    if (ev->url_con != fetch) return EINA_TRUE;


    /* ------------------------------------------------------------ */
    /* IMAGE DOWNLOAD PHASE                                         */
    /* ------------------------------------------------------------ */
    if (fetch_image)
    {
        fetch_image = EINA_FALSE;

        if (fout)
        {
            fclose(fout);
            fout = NULL;
        }

        if (ecore_file_size(fetchpath2) < 0)
            ecore_file_unlink(fetchpath2);
        else
            ecore_file_mv(fetchpath2, fetchpath);

        if (_fetch_done_cb)
            _fetch_done_cb(fetchpath, _fetch_done_data);

        /* FULL RESET */
        if (fetch) { ecore_con_url_free(fetch); fetch = NULL; }
        if (fetchpath) { free(fetchpath); fetchpath = NULL; }
        if (fetchpath2) { free(fetchpath2); fetchpath2 = NULL; }
        if (fetch_artist) { free(fetch_artist); fetch_artist = NULL; }
        if (sb_result) { eina_strbuf_free(sb_result); sb_result = NULL; }

        _fetch_done_cb = NULL;
        _fetch_done_data = NULL;
        retry = 0;

        return EINA_FALSE;
    }

    /* ------------------------------------------------------------ */
    /* HTML PHASE — PARSE GOOGLE IMAGES HTML                        */
    /* ------------------------------------------------------------ */
    if (sb_result)
    {
        const char *res = eina_strbuf_string_get(sb_result);
        printf("HTML RESPONSE (first 500 bytes):\n%.*s\n\n", 500, res ? res : "");
        if (res)
        {

            const char *p, *pe;
            Eina_Strbuf *sb = eina_strbuf_new();

            /* NEW GOOGLE: "ou":"http..." */
            p = strstr(res, "\"ou\":\"http");
            if (p)
            {
                p += strlen("\"ou\":\"");
                pe = strchr(p, '"');
                if (pe)
                {
                    eina_strbuf_append_length(sb, p, pe - p);
                    ok = EINA_TRUE;
                }
            }

            /* Rage pattern: data-src="http..." */
            if (!ok)
            {
                p = strstr(res, "data-src=\"http");
                if (p)
                {
                    p += strlen("data-src=\"");
                    pe = strchr(p, '"');
                    if (pe)
                    {
                        eina_strbuf_append_length(sb, p, pe - p);
                        ok = EINA_TRUE;
                    }
                }
            }

            /* Rage pattern: imgurl=...&amp; */
            if (!ok)
            {
                p = strstr(res, "imgurl=");
                if (p)
                {
                    p += strlen("imgurl=");
                    pe = strstr(p, "&amp;");
                    if (pe)
                    {
                        eina_strbuf_append_length(sb, p, pe - p);
                        ok = EINA_TRUE;
                    }
                }
            }

            /* LAST RESORT: encrypted-tbn0 thumbnail */
            if (!ok)
            {
                p = strstr(res, "https://encrypted-tbn0.gstatic.com");
                if (p)
                {
                    pe = strchr(p, '"');
                    if (pe)
                    {
                        eina_strbuf_append_length(sb, p, pe - p);
                        ok = EINA_TRUE;
                    }
                }
            }

            /* ------------------------------------------------------------ */
            /* If we found an image URL, start the second fetch              */
            /* ------------------------------------------------------------ */
            if (ok)
            {
                char *path, *path2;

                ecore_con_url_free(fetch);
                fetch = NULL;

                path = artist_image_thumb_path_get(fetch_artist);
                if (path)
                {
                    path2 = malloc(strlen(path) + 5);
                    sprintf(path2, "%s.tmo", path);

                    fout = fopen(path2, "wb");
                    if (fout)
                    {
                        fetch_image = EINA_TRUE;

                        if (fetchpath) free(fetchpath);
                        if (fetchpath2) free(fetchpath2);

                        fetchpath = strdup(path);
                        fetchpath2 = strdup(path2);

                        /* Delay second fetch */
                        ecore_timer_add(1.5, _delay_fetch, sb);
                        sb = NULL;
                    }

                    free(path);
                    free(path2);
                }
            }

            if (sb) eina_strbuf_free(sb);
        }

        eina_strbuf_free(sb_result);
        sb_result = NULL;
    }

    /* ------------------------------------------------------------ */
    /* FAILURE — TRY RETRY QUERY                                    */
    /* ------------------------------------------------------------ */
    if (!ok)
    {
        if (retry == 0)
        {
            retry = 1;

            printf("Retrying with alternate keywords...\n");

            Eina_Strbuf *sb2 = eina_strbuf_new();
            eina_strbuf_append(sb2, Q_START);

            Eina_Bool had = EINA_FALSE;
            had = _search_append(sb2, fetch_artist, had);
            had = _search_append(sb2, "musician", had);
            had = _search_append(sb2, "artist", had);
            had = _search_append(sb2, "last.fm", had);
            had = _search_append(sb2, "photo", had);

            eina_strbuf_append(sb2, Q_END);

            sb_result = eina_strbuf_new();
            fetch = _fetch(sb2);
            eina_strbuf_free(sb2);

            return EINA_FALSE;
        }

        /* FINAL FAILURE — CLEAN UP */
        if (fetch) { ecore_con_url_free(fetch); fetch = NULL; }
        if (fetch_artist) { free(fetch_artist); fetch_artist = NULL; }
        if (sb_result) { eina_strbuf_free(sb_result); sb_result = NULL; }

        if (_fetch_done_cb)
            _fetch_done_cb(NULL, _fetch_done_data);

        _fetch_done_cb = NULL;
        _fetch_done_data = NULL;
        retry = 0;
    }

    return EINA_FALSE;
}


static Eina_Bool
_delay_fetch(void *data)
{
   Eina_Strbuf *sb = data;

   fetch = _fetch(sb);
   eina_strbuf_free(sb);

   return EINA_FALSE;
}

/* --------------------------------------------------------------------- */
/* Public API                                                            */
/* --------------------------------------------------------------------- */

void
artist_image_fetch(const char *artist,
                   Artist_Image_Fetch_Done_Cb cb,
                   void *data)
{
    Eina_Strbuf *sb;
    char *thumb;

    printf("FETCH: artist_image_fetch() called for '%s'\n", artist ? artist : "(null)");

    if (!artist || !artist[0])
    {
        if (cb) cb(NULL, data);
        return;
    }

    /* If thumb already exists, return immediately */
    thumb = artist_image_thumb_path_get(artist);
    if (thumb && ecore_file_exists(thumb))
    {
        printf("FETCH: thumb already exists: %s\n", thumb);
        if (cb) cb(thumb, data);
        free(thumb);
        return;
    }
    free(thumb);

    /* ------------------------------------------------------------ */
    /* FULL RESET OF ANY PREVIOUS FETCH                             */
    /* ------------------------------------------------------------ */
    if (fetch)
    {
        ecore_con_url_free(fetch);
        fetch = NULL;
    }

    fetch_image = EINA_FALSE;

    if (fout)
    {
        fclose(fout);
        fout = NULL;
    }

    if (sb_result)
    {
        eina_strbuf_free(sb_result);
        sb_result = NULL;
    }

    if (fetchpath)
    {
        free(fetchpath);
        fetchpath = NULL;
    }

    if (fetchpath2)
    {
        free(fetchpath2);
        fetchpath2 = NULL;
    }

    if (fetch_artist)
    {
        free(fetch_artist);
        fetch_artist = NULL;
    }

    _fetch_done_cb = NULL;
    _fetch_done_data = NULL;

    /* ------------------------------------------------------------ */
    /* Register event handlers once                                 */
    /* ------------------------------------------------------------ */
    if (!handle_data)
        handle_data = ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA,
                                              _cb_http_data, NULL);

    if (!handle_complete)
        handle_complete = ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE,
                                                  _cb_http_complete, NULL);

    /* ------------------------------------------------------------ */
    /* Prepare new fetch                                            */
    /* ------------------------------------------------------------ */
    fetch_artist     = strdup(artist);
    _fetch_done_cb   = cb;
    _fetch_done_data = data;

    /* Build Google Images query (stronger keywords) */
    sb = eina_strbuf_new();
    eina_strbuf_append(sb, Q_START);

    Eina_Bool had = EINA_FALSE;
    had = _search_append(sb, artist, had);
    had = _search_append(sb, "musician", had);
    had = _search_append(sb, "image", had);
    had = _search_append(sb, "picture", had);
    had = _search_append(sb, "last.fm", had);

    eina_strbuf_append(sb, Q_END);

    printf("FETCH: Google query URL = %s\n", eina_strbuf_string_get(sb));

    sb_result = eina_strbuf_new();

    fetch = _fetch(sb);
    eina_strbuf_free(sb);
}


char *
artist_image_thumb_path_get(const char *artist)
{
    if (!artist || !artist[0]) return NULL;

    const char *home = getenv("HOME");
    if (!home) home = "/tmp";

    /* ~/.cache/musek/artist_thumbs/ */
    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/.cache/musek/artist_thumbs", home);

    /* ensure directory exists */
    ecore_file_mkpath(dir);

    /* compute SHA1 hash of artist name */
    unsigned char sha1_out[20];
    eina_sha1((const unsigned char *)artist, strlen(artist), sha1_out);

    char hex[41];
    for (int i = 0; i < 20; i++)
        sprintf(hex + (i * 2), "%02x", sha1_out[i]);
    hex[40] = 0;

    /* final path: <dir>/<sha1>.jpg */
    char *path = malloc(strlen(dir) + 1 + 40 + 4 + 1);
    sprintf(path, "%s/%s.jpg", dir, hex);

    return path;
}

void artist_image_prefetch_all(Player_State *ps)
{
    printf("PREFETCH: artist_image_prefetch_all() CALLED\n");

    if (!queue_running)
        queue_running = EINA_TRUE;   // <-- MUST BE FIRST

    // Now build the queue
    Eina_List *l;
    char *artist;

    EINA_LIST_FOREACH(ps->lib->artists, l, artist)
    {
        char *thumb = artist_image_thumb_path_get(artist);
        if (!ecore_file_exists(thumb))
        {
            Artist_Queue_Item *qi = calloc(1, sizeof(Artist_Queue_Item));
            qi->artist = strdup(artist);
            qi->ps = ps;

            artist_queue = eina_list_append(artist_queue, qi);
        }
        free(thumb);
    }

    if (artist_queue)
        _artist_queue_next();
}

static Eina_Bool
_artist_queue_next_delayed(void *data)
{
    Artist_Queue_Item *qi = data;
    artist_image_fetch(qi->artist, _artist_queue_done, qi);
    return EINA_FALSE; // run once
}




static void
_artist_queue_next(void)
{
    if (!artist_queue)
    {
        queue_running = EINA_FALSE;
        printf("QUEUE: empty, stopping\n");
        return;
    }

    queue_running = EINA_FALSE;

    Artist_Queue_Item *qi = artist_queue->data;
    artist_queue = eina_list_remove_list(artist_queue, artist_queue);

        ecore_timer_add(2.0, _artist_queue_next_delayed, qi);
    printf("QUEUE: fetching '%s'\n", qi->artist);

    artist_image_fetch(qi->artist, _artist_queue_done, qi);
}

static void
_artist_queue_done(const char *path, void *data)
{
    Artist_Queue_Item *qi = data;

    printf("QUEUE: done, got thumb '%s'\n", path ? path : "(null)");

    if (path)
    {
        // Find the gengrid item for this artist
        Eina_List *l;
        char *name;
        EINA_LIST_FOREACH(qi->ps->lib->artists, l, name)
        {
            if (!strcmp(name, qi->artist))
            {
                // Find the Item_Data for this artist
                Elm_Object_Item *it = elm_gengrid_first_item_get(qi->ps->artist_grid);
                while (it)
                {
                    Item_Data *id = elm_object_item_data_get(it);
                    if (id && id->u.name && !strcmp(id->u.name, qi->artist))
                    {
                        elm_gengrid_item_update(it);
                        break;
                    }
                    it = elm_gengrid_item_next_get(it);
                }
                break;
            }
        }
    }

    free(qi->artist);
    free(qi);

    _artist_queue_next();
}



Eina_Bool
artist_image_prefetch_is_running(void)
{
    return queue_running;
}


