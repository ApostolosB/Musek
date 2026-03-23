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
    case ITEM_ALBUM:
        playback_album_start(ps, id->u.name);
        populate_current_album_tracklist(ps);
        break;

    case ITEM_TRACK:
    {
        ps->album_mode = EINA_TRUE;

        ps->current_album_tracks =
            eina_hash_find(ps->lib->album_tracks, ps->current_album);

        int idx = 0;
        Eina_List *l;
        Track *t;

        EINA_LIST_FOREACH(ps->current_album_tracks, l, t) {
            if (t == id->u.track) {
                ps->current_index = idx;
                break;
            }
            idx++;
        }

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
