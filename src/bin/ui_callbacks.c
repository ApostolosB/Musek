#include <strings.h>

#include "player.h"
#include "ui_internal.h"

/* Forward declarations */
void tracklist_left_cb(void *data, Evas_Object *obj, void *event_info);
void tracklist_right_cb(void *data, Evas_Object *obj, void *event_info);


/* ============================================================
   ALBUM TILE SELECTED (GENGRID)
   ============================================================ */
void
album_tile_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);
    Album_Entry *ae = id->u.album_entry;

    if (!ps || !ae)
        return;

    /* Build track list from directory (normal or compilation) */
    Eina_List *tracks = library_tracks_for_album_dir(ps->lib, ae);
    if (!tracks)
        return;

    ps->current_album_tracks = tracks;
    ps->current_index = 0;
    ps->album_mode = EINA_TRUE;

    /* Use directory as album identity */
    ps->current_album = eina_stringshare_add(ae->path);

    /* Start playback */
    Track *first = eina_list_data_get(tracks);
    if (first)
        playback_track_start(ps, first);

    /* FIX: update right-side playlist immediately */
    populate_current_album_tracklist(ps);
}




/* ============================================================
   TRACK CLICKED IN TRACKS VIEW (LEFT LIST)
   → ALWAYS SINGLE-TRACK MODE
   ============================================================ */
void
tracklist_left_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);
    if (!id || id->type != ITEM_TRACK)
        return;

    Track *clicked = id->u.track;

    /* Enter single-track mode */
    ps->album_mode = EINA_FALSE;
    ps->current_album = NULL;
    ps->current_album_tracks = NULL;
    ps->current_index = 0;

    /* Clear playlist and show ONLY clicked track */
    ps->suppress_tracklist_callbacks = EINA_TRUE;
    elm_genlist_clear(ps->album_tracklist);

    Item_Data *nid = calloc(1, sizeof(Item_Data));
    nid->type = ITEM_TRACK;
    nid->ps = ps;
    nid->u.track = clicked;

    elm_genlist_item_append(ps->album_tracklist,
                            &itc_track,
                            nid,
                            NULL,
                            ELM_GENLIST_ITEM_NONE,
                            tracklist_right_cb,
                            ps);

    ps->suppress_tracklist_callbacks = EINA_FALSE;

    playback_track_start(ps, clicked);
}


/* ============================================================
   TRACK CLICKED IN PLAYLIST (RIGHT LIST)
   → ALBUM MODE BEHAVIOR
   ============================================================ */
void
tracklist_right_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    if (ps->suppress_tracklist_callbacks)
        return;

    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);
    if (!id || id->type != ITEM_TRACK)
        return;

    Track *clicked = id->u.track;

    /* If not in album mode, enter album mode using directory */
    if (!ps->album_mode)
    {
        ps->album_mode = EINA_TRUE;
        ps->current_album = eina_stringshare_add(clicked->dir);
        ps->current_album_tracks =
            library_tracks_for_album_dir(ps->lib, id->u.album_entry);
    }

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

    ps->suppress_tracklist_callbacks = EINA_TRUE;
    populate_current_album_tracklist(ps);
    ps->suppress_tracklist_callbacks = EINA_FALSE;

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

    ps->current_artist = NULL;
    ps->filter = FILTER_ALBUMS;
    ui_refresh_current(ps);
}

void
btn_tracks_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    /* Switch left view only — keep playlist intact */
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
    Player_State *ps = data;
    Evas_Event_Mouse_Down *ev = event_info;

    if (ev->button != 3)
        return;

    _settings_open_cb(ps, ps->win, NULL);
}
