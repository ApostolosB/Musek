#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "player.h"
#include "ui_internal.h"
#include "search.h"

/* ------------------------------------------------------------
   Helper: lowercase duplicate
   ------------------------------------------------------------ */
static char *lowerdup(const char *s)
{
    if (!s) return strdup("");
    char *out = strdup(s);
    for (char *p = out; *p; p++)
        *p = tolower(*p);
    return out;
}

/* ------------------------------------------------------------
   FILTER ARTISTS (global)
   ------------------------------------------------------------ */
void
search_filter_artists(Player_State *ps, const char *q)
{
    elm_gengrid_clear(ps->artist_grid);

    char *q_low = lowerdup(q);

    Eina_List *l;
    const char *artist;

    EINA_LIST_FOREACH(ps->lib->artists, l, artist)
    {
        char *a_low = lowerdup(artist);

        if (strstr(a_low, q_low))
        {
            Item_Data *id = calloc(1, sizeof(Item_Data));
            id->type = ITEM_ARTIST;
            id->ps = ps;
            id->album = NULL;
            id->u.name = artist;

            Elm_Object_Item *it = elm_gengrid_item_append(
                ps->artist_grid,
                &itc_artist_tile,        /* EXACTLY like populate_artists_grid */
                id,
                artist_tile_selected_cb, /* EXACT callback */
                ps
            );

            id->gengrid_item = it;
        }

        free(a_low);
    }

    free(q_low);
}

/* ------------------------------------------------------------
   FILTER ALBUMS (global)
   ------------------------------------------------------------ */
void
search_filter_albums(Player_State *ps, const char *q)
{
    elm_gengrid_clear(ps->gengrid);

    char *q_low = lowerdup(q);

    Eina_List *l;
    Album_Entry *ae;
    const char *last_artist = NULL;

    EINA_LIST_FOREACH(ps->lib->albums, l, ae)
    {
        /* Artist group header */
        if (!last_artist || strcasecmp(last_artist, ae->artist) != 0)
        {
            char *a_low = lowerdup(ae->artist);

            if (strstr(a_low, q_low))
            {
                Item_Data *gid = calloc(1, sizeof(Item_Data));
                gid->type = ITEM_ARTIST;
                gid->ps = ps;
                gid->u.name = ae->artist;

                elm_gengrid_item_append(
                    ps->gengrid,
                    &itc_artist_group,   /* EXACT item class */
                    gid,
                    NULL,
                    NULL
                );
            }

            free(a_low);
            last_artist = ae->artist;
        }

        /* Album tile */
        char buf[512];
        snprintf(buf, sizeof(buf), "%s %s", ae->artist, ae->album);
        char *low = lowerdup(buf);

        if (strstr(low, q_low))
        {
            Item_Data *aid = calloc(1, sizeof(Item_Data));
            aid->type = ITEM_ALBUM;
            aid->ps = ps;
            aid->album = ae->album;
            aid->u.album_entry = ae;

            Elm_Object_Item *it = elm_gengrid_item_append(
                ps->gengrid,
                &itc_album,              /* EXACT item class */
                aid,
                album_tile_selected_cb,  /* EXACT callback */
                ps
            );

            aid->gengrid_item = it;
        }

        free(low);
    }

    free(q_low);
}

/* ------------------------------------------------------------
   FILTER TRACKS (global)
   ------------------------------------------------------------ */
void
search_filter_tracks(Player_State *ps, const char *q)
{
    elm_genlist_clear(ps->genlist);

    char *q_low = lowerdup(q);

    /* album_tracks: album_name → Eina_List* of Track* */
    Eina_Iterator *it = eina_hash_iterator_data_new(ps->lib->album_tracks);

    Eina_List *tracklist;
    while (eina_iterator_next(it, (void **)&tracklist))
    {
        Track *t;
        Eina_List *lt;

        EINA_LIST_FOREACH(tracklist, lt, t)
        {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s %s %s",
                     t->artist, t->album, t->title);

            char *low = lowerdup(buf);

            if (strstr(low, q_low))
            {
                Item_Data *id = calloc(1, sizeof(Item_Data));
                id->type = ITEM_TRACK;
                id->ps = ps;
                id->album = t->album;
                id->u.track = t;

                /* EXACT signature from your populate_tracks() */
                elm_genlist_item_append(
                    ps->genlist,
                    &itc_track,
                    id,
                    NULL,                     /* parent */
                    ELM_GENLIST_ITEM_NONE,    /* type */
                    album_track_selected_cb,  /* callback */
                    ps                        /* func_data */
                );
            }

            free(low);
        }
    }

    eina_iterator_free(it);
    free(q_low);
}
