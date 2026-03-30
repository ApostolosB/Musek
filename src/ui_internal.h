#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H

#include <Emotion.h>
#include <Elementary.h>

#include "ui.h"

/* ------------------------------
   Item Types
   ------------------------------ */
typedef enum {
    ITEM_ARTIST,         /* Artist name (genlist or gengrid group header) */
    ITEM_ALBUM,          /* Album tile (gengrid) */
    ITEM_ALBUM_HEADER,   /* Album header in Tracks view (genlist) */
    ITEM_TRACK           /* Track item (genlist) */
} Item_Type;

/* ------------------------------
   Item Data
   ------------------------------ */
typedef struct _Item_Data {
    Item_Type type;
    Player_State *ps;

    const char *album;   /* album name for TRACK items (stringshare) */

    union {
        const char     *name;        /* Artist name (stringshare) */
        Track          *track;       /* Track (already using stringshare) */
        Album_Entry    *album_entry; /* Album entry (will use stringshare) */
    } u;

    /* NEW: needed so async artist thumbnails can update the tile */
    Elm_Object_Item *gengrid_item;

} Item_Data;



/* ------------------------------
   GENLIST Item Classes
   (Artists view + Tracks view)
   ------------------------------ */
extern Elm_Genlist_Item_Class itc_artist;
extern Elm_Genlist_Item_Class itc_album_header;
extern Elm_Genlist_Item_Class itc_track;

/* ------------------------------
   GENGRID Item Classes
   (Albums view)
   ------------------------------ */
extern Elm_Gengrid_Item_Class itc_artist_group;   /* group header */
extern Elm_Gengrid_Item_Class itc_album;          /* album tile */

/* ------------------------------
   Populate Functions
   ------------------------------ */
void populate_artists(Player_State *ps);
void populate_artists_grid(Player_State *ps);
void populate_albums(Player_State *ps);
void populate_tracks(Player_State *ps);
void populate_albums_for_artist(Player_State *ps, const char *artist);

/* ------------------------------
   UI Callbacks
   ------------------------------ */
void genlist_selected_cb(void *data, Evas_Object *obj, void *event_info);
void btn_artists_cb(void *data, Evas_Object *obj, void *event_info);
void btn_albums_cb(void *data, Evas_Object *obj, void *event_info);
void btn_tracks_cb(void *data, Evas_Object *obj, void *event_info);

void album_tile_selected_cb(void *data, Evas_Object *obj, void *event_info);
void album_track_selected_cb(void *data, Evas_Object *obj, void *event_info);

void play_cb(void *data, Evas_Object *obj, void *event_info);
void pause_cb(void *data, Evas_Object *obj, void *event_info);
void slider_changed_cb(void *data, Evas_Object *obj, void *event_info);
void volume_changed_cb(void *data, Evas_Object *obj, void *event_info);   /* FIXED: missing */
void btn_next_cb(void *data, Evas_Object *obj, void *event_info);
void btn_prev_cb(void *data, Evas_Object *obj, void *event_info);

void win_del_cb(void *data, Evas_Object *obj, void *event_info);
void _right_click_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* ------------------------------
   Init
   ------------------------------ */
void ui_populate_init(void);

#endif
