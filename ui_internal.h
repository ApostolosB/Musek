#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include "ui.h"
#include <Elementary.h>
#include <Emotion.h>

void ui_populate_init(void);


/* item types */
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

/* shared item classes */
extern Elm_Genlist_Item_Class itc_artist;
extern Elm_Genlist_Item_Class itc_album;
extern Elm_Genlist_Item_Class itc_album_header;
extern Elm_Genlist_Item_Class itc_track;

/* populate functions */
void populate_artists(Player_State *ps);
void populate_albums(Player_State *ps);
void populate_tracks(Player_State *ps);
void populate_albums_by_artist(Player_State *ps, const char *artist);


/* callbacks from ui_callbacks.c */
void genlist_selected_cb(void *data, Evas_Object *obj, void *event_info);
void btn_artists_cb(void *data, Evas_Object *obj, void *event_info);
void btn_albums_cb(void *data, Evas_Object *obj, void *event_info);
void btn_tracks_cb(void *data, Evas_Object *obj, void *event_info);
void play_cb(void *data, Evas_Object *obj, void *event_info);
void pause_cb(void *data, Evas_Object *obj, void *event_info);
void slider_changed_cb(void *data, Evas_Object *obj, void *event_info);
void btn_next_cb(void *data, Evas_Object *obj, void *event_info);
void btn_prev_cb(void *data, Evas_Object *obj, void *event_info);
void win_del_cb(void *data, Evas_Object *obj, void *event_info);

/* callbacks from ui_settings.c */
void _right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);


#endif
