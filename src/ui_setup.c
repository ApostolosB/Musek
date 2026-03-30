#include "ui_internal.h"

static void
_show_artist_grid(Player_State *ps)
{
    evas_object_show(ps->artist_grid);
    evas_object_hide(ps->genlist);
    evas_object_hide(ps->gengrid);
}

static void
_show_genlist(Player_State *ps)
{
    evas_object_show(ps->genlist);
    evas_object_hide(ps->artist_grid);
    evas_object_hide(ps->gengrid);
}

static void
_show_gengrid(Player_State *ps)
{
    evas_object_show(ps->gengrid);
    evas_object_hide(ps->artist_grid);
    evas_object_hide(ps->genlist);
}

void
ui_refresh_current(Player_State *ps)
{
    switch (ps->filter) {

    case FILTER_ARTISTS:
        _show_artist_grid(ps);
        populate_artists_grid(ps);
        break;

    case FILTER_ALBUMS:
        _show_gengrid(ps);

        if (ps->current_artist)
            populate_albums_for_artist(ps, ps->current_artist);
        else
            populate_albums(ps);
        break;

    case FILTER_TRACKS:
        _show_genlist(ps);
        populate_tracks(ps);
        break;
    }
}

void
populate_current_album_tracklist(Player_State *ps)
{
    if (!ps->album_tracklist)
        return;

    ps->suppress_tracklist_callbacks = EINA_TRUE;
    elm_genlist_clear(ps->album_tracklist);

    if (!ps->current_album_tracks) {
        ps->suppress_tracklist_callbacks = EINA_FALSE;
        return;
    }

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
            album_track_selected_cb,
            ps
        );

        if (index == ps->current_index)
            elm_genlist_item_selected_set(it, EINA_TRUE);

        index++;
    }

    ps->suppress_tracklist_callbacks = EINA_FALSE;
}

void
ui_setup(Player_State *ps)
{
    ui_populate_init();

    Evas_Object *win = ps->win;
    evas_object_size_hint_min_set(win, 800, 600);

    evas_object_smart_callback_add(win, "delete,request", win_del_cb, NULL);
    evas_object_event_callback_add(win, EVAS_CALLBACK_MOUSE_DOWN, _right_click_cb, ps);

    /* ---------------------------------------------------------
       MAIN PANES
       --------------------------------------------------------- */
    Evas_Object *panes = elm_panes_add(win);
    elm_panes_horizontal_set(panes, EINA_FALSE);
    elm_panes_content_left_size_set(panes, 0.50);
    evas_object_size_hint_weight_set(panes, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(panes, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(panes);
    elm_win_resize_object_add(win, panes);

    /* ---------------- LEFT PANE ---------------- */
    Evas_Object *left_box = elm_box_add(panes);
    evas_object_size_hint_weight_set(left_box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(left_box);
    elm_object_part_content_set(panes, "left", left_box);

    /* Filter buttons */
    Evas_Object *filter_box = elm_box_add(left_box);
    elm_box_horizontal_set(filter_box, EINA_TRUE);
    evas_object_size_hint_align_set(filter_box, EVAS_HINT_FILL, 0.0);
    evas_object_show(filter_box);
    elm_box_pack_end(left_box, filter_box);

    /* ---------------------------------------------------------
       STACKED VIEW CONTAINER (artist grid + album grid + tracklist)
       --------------------------------------------------------- */
    Evas_Object *stack = elm_table_add(left_box);
    evas_object_size_hint_weight_set(stack, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(stack, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(stack);
    elm_box_pack_end(left_box, stack);

    /* ARTIST GRID (new) */
    Evas_Object *artist_grid = elm_gengrid_add(stack);
    elm_gengrid_horizontal_set(artist_grid, EINA_FALSE);
    elm_gengrid_item_size_set(artist_grid, 110, 150);
    evas_object_size_hint_weight_set(artist_grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(artist_grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(stack, artist_grid, 0, 0, 1, 1);
    evas_object_hide(artist_grid);
    ps->artist_grid = artist_grid;

    /* GENLIST (Tracks view) */
    Evas_Object *genlist = elm_genlist_add(stack);
    evas_object_size_hint_weight_set(genlist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(stack, genlist, 0, 0, 1, 1);
    ps->genlist = genlist;

    /* GENGRID (Albums) */
    Evas_Object *gengrid = elm_gengrid_add(stack);
    elm_gengrid_horizontal_set(gengrid, EINA_FALSE);
    elm_gengrid_item_size_set(gengrid, 110, 150);
    evas_object_size_hint_weight_set(gengrid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(gengrid, EVAS_HINT_FILL, EVAS_HINT_FILL);
    elm_table_pack(stack, gengrid, 0, 0, 1, 1);
    evas_object_hide(gengrid);
    ps->gengrid = gengrid;

    /* Filter buttons */
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

    /* ---------------- RIGHT PANE ---------------- */
    Evas_Object *right = elm_box_add(panes);
    elm_box_padding_set(right, 0, 10);
    evas_object_size_hint_weight_set(right, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(right, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(right);
    elm_object_part_content_set(panes, "right", right);

    /* Title */
    Evas_Object *title = elm_label_add(right);
    elm_object_text_set(title, "<b>Artist Name - Track Title</b>");
    evas_object_size_hint_weight_set(title, 0.0, 0.0);
    evas_object_size_hint_align_set(title, EVAS_HINT_FILL, 0.0);
    evas_object_show(title);
    elm_box_pack_end(right, title);
    ps->title_label = title;

    /* Album art */
    Evas_Object *album_art = elm_image_add(right);
    elm_image_aspect_fixed_set(album_art, EINA_TRUE);
    elm_image_resizable_set(album_art, EINA_TRUE, EINA_TRUE);
    evas_object_size_hint_min_set(album_art, 250, 250);
    evas_object_size_hint_weight_set(album_art, 0.0, 0.0);
    evas_object_size_hint_align_set(album_art, EVAS_HINT_FILL, 0.0);
    evas_object_show(album_art);
    elm_box_pack_end(right, album_art);
    ps->album_art = album_art;

    elm_image_file_set(album_art, "/data/noart.png", NULL);

    /* Emotion player */
    Evas_Object *emotion = emotion_object_add(evas_object_evas_get(win));
    emotion_object_init(emotion, NULL);
    evas_object_size_hint_weight_set(emotion, 0.0, 0.0);
    evas_object_size_hint_align_set(emotion, EVAS_HINT_FILL, 0.0);
    evas_object_show(emotion);
    elm_box_pack_end(right, emotion);
    ps->emotion = emotion;

    /* Playback controls */
    Evas_Object *controls = elm_box_add(right);
    elm_box_horizontal_set(controls, EINA_TRUE);
    evas_object_size_hint_weight_set(controls, 0.0, 0.0);
    evas_object_size_hint_align_set(controls, EVAS_HINT_FILL, 0.0);
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

    /* Progress bar row */
    Evas_Object *hbox = elm_box_add(right);
    elm_box_horizontal_set(hbox, EINA_TRUE);
    evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, 0.0);
    evas_object_show(hbox);
    elm_box_pack_end(right, hbox);

    /* Left label: "time" */
    ps->lbl_time_text = elm_label_add(hbox);
    elm_object_text_set(ps->lbl_time_text, "0:00");
    evas_object_show(ps->lbl_time_text);
    elm_box_pack_end(hbox, ps->lbl_time_text);

    /* Slider */
    ps->slider = elm_slider_add(hbox);
    elm_slider_indicator_show_set(ps->slider, EINA_FALSE);
    elm_slider_min_max_set(ps->slider, 0.0, 1.0);
    evas_object_size_hint_weight_set(ps->slider, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(ps->slider, EVAS_HINT_FILL, 0.0);
    evas_object_show(ps->slider);
    elm_box_pack_end(hbox, ps->slider);

    /* Tooltip on slider knob showing current time */
    elm_object_tooltip_text_set(ps->slider, "0:00");
    ps->slider_indicator = ps->slider;


    /* Right label: total duration */
    ps->lbl_time_total = elm_label_add(hbox);
    elm_object_text_set(ps->lbl_time_total, "0:00");
    evas_object_show(ps->lbl_time_total);
    elm_box_pack_end(hbox, ps->lbl_time_total);


    /* Tracklist */
    Evas_Object *tracklist = elm_genlist_add(right);
    evas_object_size_hint_weight_set(tracklist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(tracklist, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(tracklist);
    elm_box_pack_end(right, tracklist);
    ps->album_tracklist = tracklist;

    /* Volume */
    Evas_Object *vol_box = elm_box_add(right);
    elm_box_horizontal_set(vol_box, EINA_TRUE);
    evas_object_size_hint_weight_set(vol_box, 0.0, 0.0);
    evas_object_size_hint_align_set(vol_box, 1.0, 1.0);
    evas_object_show(vol_box);
    elm_box_pack_end(right, vol_box);

    Evas_Object *vol = elm_slider_add(vol_box);
    elm_slider_horizontal_set(vol, EINA_TRUE);
    elm_slider_min_max_set(vol, 0.0, 1.0);
    elm_slider_value_set(vol, 1.0);
    elm_object_text_set(vol, "Vol.");
    evas_object_size_hint_weight_set(vol, 0.0, 0.0);
    evas_object_size_hint_align_set(vol, 1.0, 0.5);
    evas_object_size_hint_min_set(vol, 120, 1);
    evas_object_show(vol);
    elm_box_pack_end(vol_box, vol);
    ps->volume_slider = vol;

    /* Callbacks */
    evas_object_smart_callback_add(btn_play, "clicked", play_cb, ps);
    evas_object_smart_callback_add(btn_pause, "clicked", pause_cb, ps);
    evas_object_smart_callback_add(btn_prev, "clicked", btn_prev_cb, ps);
    evas_object_smart_callback_add(btn_next, "clicked", btn_next_cb, ps);
    evas_object_smart_callback_add(ps->slider, "changed", slider_changed_cb, ps);
    evas_object_smart_callback_add(vol, "changed", volume_changed_cb, ps);

    /* Init playback */
    playback_init(ps);

    /* Default view */
    ps->filter = FILTER_ARTISTS;
    ui_refresh_current(ps);
}
