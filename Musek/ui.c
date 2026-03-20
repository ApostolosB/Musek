#include "player.h"
#include <string.h>
#include <stdlib.h>

static void populate_albums_by_artist(Player_State *ps, const char *artist);
void populate_current_album_tracklist(struct _Player_State *ps);

/* settings popup forward declarations */
static void _right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _settings_open_cb(void *data, Evas_Object *obj, void *event_info);
static void _settings_save_cb(void *data, Evas_Object *obj, void *event_info);

/* genlist item types */

typedef enum {
   ITEM_ARTIST,
   ITEM_ALBUM,
   ITEM_ALBUM_HEADER,
   ITEM_TRACK
} Item_Type;

typedef struct _Item_Data {
   Item_Type type;
   union {
      const char *name;
      Track      *track;
   } u;
} Item_Data;

static Elm_Genlist_Item_Class itc_artist;
static Elm_Genlist_Item_Class itc_album;
static Elm_Genlist_Item_Class itc_album_header;
static Elm_Genlist_Item_Class itc_track;


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

/* populate helpers */

static void
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

static void
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

static void
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
/* NEW: populate albums filtered by artist */
static void
populate_albums_by_artist(Player_State *ps, const char *artist)
{
   elm_genlist_clear(ps->genlist);

   char *album_name;
   Eina_List *l;

  /* iterate all albums */
   EINA_LIST_FOREACH(ps->lib->albums, l, album_name) {
      /* get tracks for this album */
      Eina_List *tracks = eina_hash_find(ps->lib->album_tracks, album_name);
      if (!tracks) continue;

      /* check if any track matches the artist */
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


void
ui_refresh_current(Player_State *ps)
{
   switch (ps->filter) {
   case FILTER_ARTISTS: populate_artists(ps); break;
   case FILTER_ALBUMS:  populate_albums(ps);  break;
   case FILTER_TRACKS:  populate_tracks(ps);  break;
   }
}

/* callbacks */

static void
genlist_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
   Player_State *ps = data;
   Elm_Object_Item *it = event_info;
   Item_Data *id = elm_object_item_data_get(it);
   if (!id) return;   

   /* NEW: clicking an artist filters albums */
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

    // Find index of clicked track inside current album
    int idx = 0;
    Eina_List *l;
    char *path;

    EINA_LIST_FOREACH(ps->current_album_tracks, l, path) {
        if (strcmp(path, id->u.track->path) == 0) {
            ps->current_index = idx;
            break;
        }
        idx++;
    }

    playback_track_start(ps, id->u.track);
    break;
}
      break;
   default:
      break;
   }
}

static void
btn_artists_cb(void *data, Evas_Object *obj, void *event_info)
{
   Player_State *ps = data;
   ps->filter = FILTER_ARTISTS;
   ui_refresh_current(ps);
}

static void
btn_albums_cb(void *data, Evas_Object *obj, void *event_info)
{
   Player_State *ps = data;
   ps->filter = FILTER_ALBUMS;
   ui_refresh_current(ps);
}

static void
btn_tracks_cb(void *data, Evas_Object *obj, void *event_info)
{
   Player_State *ps = data;
   ps->filter = FILTER_TRACKS;
   ui_refresh_current(ps);
}

static void
play_cb(void *data, Evas_Object *obj, void *event_info)
{
   playback_play(data);
}

static void
pause_cb(void *data, Evas_Object *obj, void *event_info)
{
   playback_pause(data);
}

static void
slider_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Player_State *ps = data;
   double val = elm_slider_value_get(obj);
   playback_seek(ps, val);
}

static void
win_del_cb(void *data, Evas_Object *obj, void *event_info)
{
   elm_exit();
}

static void
btn_next_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    if (!ps->album_mode || !ps->current_album_tracks)
        return;

    ps->current_index++;
    if (ps->current_index >= eina_list_count(ps->current_album_tracks))
        ps->current_index = 0;  // wrap around

    Track *t = eina_list_nth(ps->current_album_tracks, ps->current_index);
    if (t)
        playback_track_start(ps, t);
}

static void
btn_prev_cb(void *data, Evas_Object *obj, void *event_info)
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
}


/* setup */

void
ui_setup(Player_State *ps)
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
   
   // Tracklist under progress bar
   Evas_Object *tracklist = elm_genlist_add(right);
   evas_object_size_hint_weight_set(tracklist, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(tracklist, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(tracklist);
   elm_box_pack_end(right, tracklist);
   ps->album_tracklist = tracklist;
     
   // start empty
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



/* ============================
 * Right-click → Settings popup
 * ============================ */

static void
_right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *ev = event_info;
    if (ev->button != 3) return; /* right-click only */

    Player_State *ps = data;

    Evas_Object *menu = elm_menu_add(obj);
    elm_menu_item_add(menu, NULL, NULL, "Settings…", _settings_open_cb, ps);
    elm_menu_move(menu, ev->canvas.x, ev->canvas.y);
    evas_object_show(menu);
}

void populate_current_album_tracklist(Player_State *ps)
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

static void
_settings_open_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    Evas_Object *popup = elm_popup_add(obj);
    elm_object_part_text_set(popup, "title,text", "Settings");

    /* store ps inside popup */
    evas_object_data_set(popup, "player_state", ps);

    Evas_Object *box = elm_box_add(popup);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_horizontal_set(box, EINA_FALSE);
    evas_object_show(box);

    Evas_Object *label = elm_label_add(box);
    elm_object_text_set(label, "Music directory:");
    evas_object_show(label);
    elm_box_pack_end(box, label);

    Evas_Object *entry = elm_entry_add(box);
    elm_entry_single_line_set(entry, EINA_TRUE);
    elm_object_text_set(entry, ps->settings->music_folder);
    evas_object_show(entry);
    elm_box_pack_end(box, entry);

    elm_object_content_set(popup, box);

    elm_popup_item_append(popup, "OK", NULL, _settings_save_cb, entry);
    elm_popup_item_append(popup, "Cancel", NULL, NULL, NULL);

    evas_object_show(popup);
}


static void
_settings_save_cb(void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *entry = data;
    Player_State *ps = evas_object_data_get(obj, "player_state");

    const char *newdir = elm_object_text_get(entry);
    if (newdir && newdir[0]) {
        free(ps->settings->music_folder);
        ps->settings->music_folder = strdup(newdir);
    }

    evas_object_del(obj); /* close popup */
}