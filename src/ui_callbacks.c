#include "ui_internal.h"

void
genlist_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);
    if (!id) return;

    if (id->type == ITEM_ARTIST) {
        ps->filter = FILTER_ALBUMS;
        populate_albums_by_artist(ps, id->u.name);
        return;
    }

    switch (id->type) {

    /* ------------------------------
       ALBUM SELECTED
       ------------------------------ */
    case ITEM_ALBUM:
    {
        const char *album = id->u.name;

        ps->album_mode = EINA_TRUE;
        ps->current_album = album;

        /* Load tracks for this album */
        ps->current_album_tracks =
            eina_hash_find(ps->lib->album_tracks, album);

        if (!ps->current_album_tracks) {
            elm_genlist_clear(ps->album_tracklist);
            break;
        }

        /* Start playback at first track */
        ps->current_index = 0;
        Track *first = eina_list_nth(ps->current_album_tracks, 0);

        if (first)
            playback_track_start(ps, first);

        populate_current_album_tracklist(ps);
        break;
    }

    /* ------------------------------
       TRACK SELECTED
       ------------------------------ */
    case ITEM_TRACK:
    {
        /* Disable album mode */
        ps->album_mode = EINA_FALSE;
        ps->current_album = NULL;

        /* Clear right pane */
        elm_genlist_clear(ps->album_tracklist);

        /* IMPORTANT:
         * Do NOT free ps->current_album_tracks.
         * It may point to a list owned by lib->album_tracks.
         */

        /* Create a new one-element list */
        ps->current_album_tracks = eina_list_append(NULL, id->u.track);
        ps->current_index = 0;

        /* Add selected track to right pane */
        Item_Data *tid = calloc(1, sizeof(Item_Data));
        tid->type = ITEM_TRACK;
        tid->u.track = id->u.track;

        elm_genlist_item_append(ps->album_tracklist,
                                &itc_track,
                                tid,
                                NULL,
                                ELM_GENLIST_ITEM_NONE,
                                NULL,
                                ps);

        /* Start playback */
        playback_track_start(ps, id->u.track);
        break;
    }

    default:
        break;
    }
}

void btn_artists_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    ps->filter = FILTER_ARTISTS;
    ui_refresh_current(ps);
}

void btn_albums_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    ps->filter = FILTER_ALBUMS;
    ui_refresh_current(ps);
}

void btn_tracks_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    ps->filter = FILTER_TRACKS;
    ui_refresh_current(ps);
}

void play_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_play(data);
}

void pause_cb(void *data, Evas_Object *obj, void *event_info)
{
    playback_pause(data);
}

void slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    double val = elm_slider_value_get(obj);
    playback_seek(ps, val);
}

void win_del_cb(void *data, Evas_Object *obj, void *event_info)
{
    elm_exit();
}

void btn_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    if (!ps->album_mode || !ps->current_album_tracks)
        return;

    ps->current_index++;
    if (ps->current_index >= eina_list_count(ps->current_album_tracks))
        ps->current_index = 0;

    Track *t = eina_list_nth(ps->current_album_tracks, ps->current_index);
    if (t)
        playback_track_start(ps, t);

    populate_current_album_tracklist(ps);
}

void btn_prev_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    if (!ps->album_mode || !ps->current_album_tracks)
        return;

    if (ps->current_index == 0)
        ps->current_index = eina_list_count(ps->current_album_tracks) - 1;
    else
        ps->current_index--;

    Track *t = eina_list_nth(ps->current_album_tracks, ps->current_index);
    if (t)
        playback_track_start(ps, t);

    populate_current_album_tracklist(ps);
}

void
volume_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    double vol = elm_slider_value_get(obj);
    playback_set_volume(ps, vol);
}
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

    /* Find index */
    int idx = 0;
    Track *t;
    Eina_List *l;

    EINA_LIST_FOREACH(ps->current_album_tracks, l, t) {
        if (t == id->u.track) {
            ps->current_index = idx;
            break;
        }
        idx++;
    }

    playback_track_start(ps, id->u.track);

    /* Refresh right pane */
    populate_current_album_tracklist(ps);
}

