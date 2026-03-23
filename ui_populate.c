#include "ui_internal.h"
#include <string.h>
#include <stdlib.h>

/* item classes */
Elm_Genlist_Item_Class itc_artist;
Elm_Genlist_Item_Class itc_album;
Elm_Genlist_Item_Class itc_album_header;
Elm_Genlist_Item_Class itc_track;

/* text + del functions */
static char *
_gl_text_get(void *data, Evas_Object *obj, const char *part)
{
    Item_Data *id = data;
    if (!id) return NULL;

    switch (id->type) {
    case ITEM_ARTIST:
    case ITEM_ALBUM:
    case ITEM_ALBUM_HEADER:
        return strdup(id->u.name ? id->u.name : "");
    case ITEM_TRACK:
        return strdup(id->u.track->title ? id->u.track->title : "");
    default:
        return NULL;
    }
}

static void
_gl_del(void *data, Evas_Object *obj)
{
    free(data);
}

/* populate artists */
void
populate_artists(Player_State *ps)
{
    elm_genlist_clear(ps->genlist);

    char *name;
    Eina_List *l;
    EINA_LIST_FOREACH(ps->lib->artists, l, name) {
        Item_Data *id = calloc(1, sizeof(Item_Data));
        id->type = ITEM_ARTIST;
        id->u.name = name;
        elm_genlist_item_append(ps->genlist, &itc_artist, id, NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, ps);
    }
}

/* populate albums */
void
populate_albums(Player_State *ps)
{
    elm_genlist_clear(ps->genlist);

    char *name;
    Eina_List *l;
    EINA_LIST_FOREACH(ps->lib->albums, l, name) {
        Item_Data *id = calloc(1, sizeof(Item_Data));
        id->type = ITEM_ALBUM;
        id->u.name = name;
        elm_genlist_item_append(ps->genlist, &itc_album, id, NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, ps);
    }
}

/* populate tracks */
void
populate_tracks(Player_State *ps)
{
    elm_genlist_clear(ps->genlist);

    char *album_name;
    Eina_List *l;
    EINA_LIST_FOREACH(ps->lib->albums, l, album_name) {
        Item_Data *id_header = calloc(1, sizeof(Item_Data));
        id_header->type = ITEM_ALBUM_HEADER;
        id_header->u.name = album_name;
        elm_genlist_item_append(ps->genlist, &itc_album_header, id_header, NULL,
                                ELM_GENLIST_ITEM_NONE, NULL, ps);

        Eina_List *tracks = eina_hash_find(ps->lib->album_tracks, album_name);
        Track *t;
        Eina_List *lt;
        EINA_LIST_FOREACH(tracks, lt, t) {
            Item_Data *id = calloc(1, sizeof(Item_Data));
            id->type = ITEM_TRACK;
            id->u.track = t;
            elm_genlist_item_append(ps->genlist, &itc_track, id, NULL,
                                    ELM_GENLIST_ITEM_NONE, NULL, ps);
        }
    }
}

/* populate albums filtered by artist */
void
populate_albums_by_artist(Player_State *ps, const char *artist)
{
    elm_genlist_clear(ps->genlist);

    char *album_name;
    Eina_List *l;

    EINA_LIST_FOREACH(ps->lib->albums, l, album_name) {
        Eina_List *tracks = eina_hash_find(ps->lib->album_tracks, album_name);
        if (!tracks) continue;

        Track *t;
        Eina_List *lt;
        Eina_Bool match = EINA_FALSE;

        EINA_LIST_FOREACH(tracks, lt, t) {
            if (t->artist && strcmp(t->artist, artist) == 0) {
                match = EINA_TRUE;
                break;
            }
        }

        if (match) {
            Item_Data *id = calloc(1, sizeof(Item_Data));
            id->type = ITEM_ALBUM;
            id->u.name = album_name;
            elm_genlist_item_append(ps->genlist, &itc_album, id, NULL,
                                    ELM_GENLIST_ITEM_NONE, NULL, ps);
        }
    }
}
void ui_populate_init(void)
{
    memset(&itc_artist, 0, sizeof(itc_artist));
    itc_artist.item_style = "default";
    itc_artist.func.text_get = _gl_text_get;
    itc_artist.func.del = _gl_del;

    memset(&itc_album, 0, sizeof(itc_album));
    itc_album.item_style = "default";
    itc_album.func.text_get = _gl_text_get;
    itc_album.func.del = _gl_del;

    memset(&itc_album_header, 0, sizeof(itc_album_header));
    itc_album_header.item_style = "default";
    itc_album_header.func.text_get = _gl_text_get;
    itc_album_header.func.del = _gl_del;

    memset(&itc_track, 0, sizeof(itc_track));
    itc_track.item_style = "default";
    itc_track.func.text_get = _gl_text_get;
    itc_track.func.del = _gl_del;
}
