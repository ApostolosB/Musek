#include <strings.h>

#include "player.h"
#include "ui_internal.h"

/* ============================================================
   ALBUM TILE SELECTED (GENGRID)
   ============================================================ */
void
album_tile_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);

    if (!id || id->type != ITEM_ALBUM)
        return;

    Album_Entry *a = id->u.album_entry;
    if (!a) return;

    /* Enter album mode */
    ps->album_mode = EINA_TRUE;
    ps->current_album = a->album;   /* album name string */

    /* Load album tracks */
    ps->current_album_tracks =
        eina_hash_find(ps->lib->album_tracks, a->album);

    if (!ps->current_album_tracks) {
        elm_genlist_clear(ps->album_tracklist);
        return;
    }

    /* Start playback at first track */
    ps->current_index = 0;
    Track *first = eina_list_nth(ps->current_album_tracks, 0);

    if (first)
        playback_track_start(ps, first);

    populate_current_album_tracklist(ps);
}

/* ============================================================
   TRACK SELECTED (GENLIST)
   ============================================================ */
void
album_track_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    if (ps->suppress_tracklist_callbacks)
        return;

    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);
    if (!id || id->type != ITEM_TRACK)
        return;

    Track *clicked = id->u.track;
    const char *album = id->album;   /* album name string */

    /* ---------------------------------------------------------
       TRACKS VIEW: show ONLY the clicked track in the tracklist
       --------------------------------------------------------- */
    if (ps->filter == FILTER_TRACKS) {

        /* Leave album mode entirely */
        ps->album_mode = EINA_FALSE;
        ps->current_album = NULL;
        ps->current_album_tracks = NULL;
        ps->current_index = 0;

        /* Rebuild right-side tracklist with just this track */
        ps->suppress_tracklist_callbacks = EINA_TRUE;
        elm_genlist_clear(ps->album_tracklist);

        Item_Data *nid = calloc(1, sizeof(Item_Data));
        if (nid) {
            nid->type = ITEM_TRACK;
            nid->u.track = clicked;
            nid->album = clicked->album;  /* or album */

            elm_genlist_item_append(
                ps->album_tracklist,
                &itc_track,
                nid,
                NULL,
                ELM_GENLIST_ITEM_NONE,
                album_track_selected_cb,
                ps
            );
        }

        ps->suppress_tracklist_callbacks = EINA_FALSE;

        /* Play only this track */
        playback_track_start(ps, clicked);
        return;
    }

    /* ---------------------------------------------------------
       ALBUM MODE (unchanged)
       --------------------------------------------------------- */
    if (!ps->album_mode) {

        ps->current_album = album;

        ps->current_album_tracks =
            eina_hash_find(ps->lib->album_tracks, ps->current_album);

        /* Find index of clicked track */
        int idx = 0;
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(ps->current_album_tracks, l, t) {
            if (t == clicked) {
                ps->current_index = idx;
                break;
            }
            idx++;
        }

        ps->album_mode = EINA_TRUE;

        ps->suppress_tracklist_callbacks = EINA_TRUE;
        populate_current_album_tracklist(ps);
        ps->suppress_tracklist_callbacks = EINA_FALSE;
    }
    else {
        /* Already in album mode — update index */
        int idx = 0;
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(ps->current_album_tracks, l, t) {
            if (t == clicked) {
                ps->current_index = idx;
                break;
            }
            idx++;
        }

        ps->suppress_tracklist_callbacks = EINA_TRUE;
        populate_current_album_tracklist(ps);
        ps->suppress_tracklist_callbacks = EINA_FALSE;
    }

    playback_track_start(ps, clicked);
}



/* ============================================================
   WINDOW CLOSE
   ============================================================ */
void
win_del_cb(void *data, Evas_Object *obj, void *event_info)
{
    elm_exit();
}

/* ============================================================
   FILTER BUTTONS
   ============================================================ */
void
btn_artists_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    ps->filter = FILTER_ARTISTS;
    ui_refresh_current(ps);
}

void
btn_albums_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    /* Clear any artist filter so Albums shows ALL albums */
    ps->current_artist = NULL;

    ps->filter = FILTER_ALBUMS;
    ui_refresh_current(ps);
}


void
btn_tracks_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    ps->filter = FILTER_TRACKS;
    ui_refresh_current(ps);
}

/* ============================================================
   PLAYBACK CONTROLS
   ============================================================ */
void
play_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_resume((Player_State *)data);
}

void
pause_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_pause((Player_State *)data);
}

void
btn_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_prev((Player_State *)data);
}

void
btn_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_next((Player_State *)data);
}

/* ============================================================
   SLIDERS
   ============================================================ */
void
slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    double pos = elm_slider_value_get(obj);
    playback_seek(ps, pos);
}

void
volume_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    double vol = elm_slider_value_get(obj);
    playback_set_volume(ps, vol);
}

/* ============================================================
   RIGHT CLICK (OPTIONAL)
   ============================================================ */
void
_right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    /* No-op */
}
