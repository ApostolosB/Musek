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
                &itc_artist_tile,
                id,
                artist_tile_selected_cb,
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
                    &itc_artist_group,
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
                &itc_album,
                aid,
                album_tile_selected_cb,
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

    Album_Entry *ae;
    Eina_List *l;

    /* Iterate over albums in sorted order */
    EINA_LIST_FOREACH(ps->lib->albums, l, ae)
    {
        /* Build album key: artist|album */
        char key[512];
        snprintf(key, sizeof(key), "%s|%s",
                 ae->artist ? ae->artist : "",
                 ae->album  ? ae->album  : "");

        /* Get track list for this album */
        Eina_List *tracks = eina_hash_find(ps->lib->album_tracks, key);
        if (!tracks)
            continue;

        Track *t;
        Eina_List *lt;

        /* Iterate over tracks in this album */
        EINA_LIST_FOREACH(tracks, lt, t)
        {
            char buf[512];
            snprintf(buf, sizeof(buf), "%s %s %s",
                     t->artist ? t->artist : "",
                     t->album  ? t->album  : "",
                     t->title  ? t->title  : "");

            char *low = lowerdup(buf);

            if (strstr(low, q_low))
            {
                Item_Data *id = calloc(1, sizeof(Item_Data));
                id->type = ITEM_TRACK;
                id->ps = ps;
                id->album = t->album;
                id->u.track = t;

                /* SEARCH RESULTS BEHAVE LIKE TRACKS VIEW */
                elm_genlist_item_append(
                    ps->genlist,
                    &itc_track,
                    id,
                    NULL,
                    ELM_GENLIST_ITEM_NONE,
                    tracklist_left_cb,   /* <-- IMPORTANT */
                    ps
                );
            }

            free(low);
        }
    }

    free(q_low);
}
