#include "ui_internal.h"

void
ui_refresh_current(Player_State *ps)
{
    switch (ps->filter) {
    case FILTER_ARTISTS: populate_artists(ps); break;
    case FILTER_ALBUMS:  populate_albums(ps);  break;
    case FILTER_TRACKS:  populate_tracks(ps);  break;
    }
}

void
populate_current_album_tracklist(Player_State *ps)
{
    if (!ps->album_tracklist)
        return;

    elm_genlist_clear(ps->album_tracklist);

    if (!ps->current_album_tracks)
        return;

    Track *t;
    Eina_List *l;
    int index = 0;

    EINA_LIST_FOREACH(ps->current_album_tracks, l, t) {
        Item_Data *id = calloc(1, sizeof(Item_Data));
        id->type = ITEM_TRACK;
        id->u.track = t;

        Elm_Object_Item *it = elm_genlist_item_append(
            ps->album_tracklist,
            &itc_track,
            id,
            NULL,
            ELM_GENLIST_ITEM_NONE,
            genlist_selected_cb,
            ps
        );

        if (index == ps->current_index)
            elm_genlist_item_selected_set(it, EINA_TRUE);

        index++;
    }
}

void
ui_setup(Player_State *ps)
{
    /* initialize item classes */
    ui_populate_init();

    Evas_Object *win = ps->win;
    evas_object_smart_callback_add(win, "delete,request", win_del_cb, NULL);

    /* Right-click menu */
    evas_object_event_callback_add(win, EVAS_CALLBACK_MOUSE_DOWN, _right_click_cb, ps);

    Evas_Object *panes = elm_panes_add(win);
    elm_panes_horizontal_set(panes, EINA_FALSE);
    elm_panes_content_left_size_set(panes, 0.40);
    evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(panes);
    elm_win_resize_object_add(win, panes);

    /* left pane */
    Evas_Object *left_box = elm_box_add(panes);
    elm_box_padding_set(left_box, 0, 5);
    evas_object_size_hint_weight_set(left_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(left_box);
    elm_object_part_content_set(panes, "left", left_box);

    Evas_Object *filter_box = elm_box_add(left_box);
    elm_box_horizontal_set(filter_box, EINA_TRUE);
    evas_object_size_hint_weight_set(filter_box, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(filter_box, EVAS_HINT_FILL, 0.0);
    evas_object_show(filter_box);
    elm_box_pack_end(left_box, filter_box);

    Evas_Object *genlist = elm_genlist_add(left_box);
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(genlist);
    elm_box_pack_end(left_box, genlist);
    ps->genlist = genlist;

    Evas_Object *btn_artists = elm_button_add(filter_box);
    elm_object_text_set(btn_artists, "Artists");
    evas_object_show(btn_artists);
    elm_box_pack_end(filter_box, btn_artists);

    Evas_Object *btn_albums = elm_button_add(filter_box);
    elm_object_text_set(btn_albums, "Albums");
    evas_object_show(btn_albums);
    elm_box_pack_end(filter_box, btn_albums);

    Evas_Object *btn_tracks = elm_button_add(filter_box);
    elm_object_text_set(btn_tracks, "Tracks");
    evas_object_show(btn_tracks);
    elm_box_pack_end(filter_box, btn_tracks);

    evas_object_smart_callback_add(btn_artists, "clicked", btn_artists_cb, ps);
    evas_object_smart_callback_add(btn_albums, "clicked", btn_albums_cb, ps);
    evas_object_smart_callback_add(btn_tracks, "clicked", btn_tracks_cb, ps);

    evas_object_smart_callback_add(genlist, "selected", genlist_selected_cb, ps);

    /* right pane */
    Evas_Object *right = elm_box_add(panes);
    evas_object_size_hint_weight_set(right, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_padding_set(right, 0, 10);
    evas_object_show(right);
    elm_object_part_content_set(panes, "right", right);

    Evas_Object *album_art = elm_image_add(right);
    evas_object_size_hint_weight_set(album_art, 0.0, 0.0);
    evas_object_size_hint_align_set(album_art, 0.5, 0.0);
    evas_object_show(album_art);
    elm_box_pack_end(right, album_art);
    ps->album_art = album_art;

    Evas_Object *title = elm_label_add(right);
    elm_object_text_set(title, "<b>Track Title</b><br/>Artist Name");
    evas_object_size_hint_align_set(title, 0.5, 0.0);
    evas_object_show(title);
    elm_box_pack_end(right, title);
    ps->title_label = title;

    Evas_Object *emotion = emotion_object_add(evas_object_evas_get(win));
    emotion_object_init(emotion, NULL);
    evas_object_size_hint_weight_set(emotion, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(emotion, EVAS_HINT_FILL, 0.0);
    evas_object_show(emotion);
    elm_box_pack_end(right, emotion);
    ps->emotion = emotion;

    Evas_Object *controls = elm_box_add(right);
    elm_box_horizontal_set(controls, EINA_TRUE);
    evas_object_size_hint_weight_set(controls, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(controls, 0.5, 0.0);
    evas_object_show(controls);
    elm_box_pack_end(right, controls);

    Evas_Object *btn_prev = elm_button_add(controls);
    elm_object_text_set(btn_prev, "<<");
    evas_object_show(btn_prev);
    elm_box_pack_end(controls, btn_prev);

    Evas_Object *btn_play = elm_button_add(controls);
    elm_object_text_set(btn_play, "Play");
    evas_object_show(btn_play);
    elm_box_pack_end(controls, btn_play);

    Evas_Object *btn_pause = elm_button_add(controls);
    elm_object_text_set(btn_pause, "Pause");
    evas_object_show(btn_pause);
    elm_box_pack_end(controls, btn_pause);

    Evas_Object *btn_next = elm_button_add(controls);
    elm_object_text_set(btn_next, ">>");
    evas_object_show(btn_next);
    elm_box_pack_end(controls, btn_next);

    Evas_Object *slider = elm_slider_add(right);
    elm_object_text_set(slider, "Progress");
    elm_slider_min_max_set(slider, 0.0, 1.0);
    elm_slider_value_set(slider, 0.0);
    evas_object_size_hint_weight_set(slider, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(slider, EVAS_HINT_FILL, 0.0);
    evas_object_show(slider);
    elm_box_pack_end(right, slider);
    ps->slider = slider;

    /* tracklist under progress bar */
    Evas_Object *tracklist = elm_genlist_add(right);
    evas_object_size_hint_weight_set(tracklist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(tracklist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(tracklist);
    elm_box_pack_end(right, tracklist);
    ps->album_tracklist = tracklist;

    elm_genlist_clear(tracklist);

    evas_object_smart_callback_add(btn_play, "clicked", play_cb, ps);
    evas_object_smart_callback_add(btn_pause, "clicked", pause_cb, ps);
    evas_object_smart_callback_add(btn_prev, "clicked", btn_prev_cb, ps);
    evas_object_smart_callback_add(btn_next, "clicked", btn_next_cb, ps);
    evas_object_smart_callback_add(slider, "changed", slider_changed_cb, ps);

    playback_init(ps);

    ps->filter = FILTER_ALBUMS;
    ui_refresh_current(ps);
}
