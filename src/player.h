#ifndef PLAYER_H
#define PLAYER_H

#include <Eio.h>
#include <Emotion.h>
#include <Elementary.h>

#include <taglib/tag_c.h>

/* ------------------------------
   Track Structure
   ------------------------------ */
typedef struct _Track {
    const char *title;   /* eina_stringshare */
    const char *artist;  /* eina_stringshare */
    const char *album;   /* eina_stringshare */
    const char *path;    /* eina_stringshare */
    const char *dir;     /* eina_stringshare */
    int   track_no;
} Track;


typedef struct {
    const char *artist;    /* eina_stringshare */
    const char *album;     /* eina_stringshare */
    const char *path;      /* eina_stringshare */
    const char *art_path;  /* eina_stringshare */
} Album_Entry;


/* ------------------------------
   Filter Modes
   ------------------------------ */
typedef enum {
   FILTER_ARTISTS,
   FILTER_ALBUMS,
   FILTER_TRACKS
} Filter_Mode;

/* ------------------------------
   Library Structure
   ------------------------------ */
typedef struct _Library {
   Eina_List *artists;      // char* (sorted alphabetically)
   Eina_List *albums;       // Album_Entry* (sorted by artist → album)
   Eina_Hash *album_tracks; // key: album name (char*), value: Eina_List* of Track*
} Library;

/* ------------------------------
   Settings
   ------------------------------ */
typedef struct _Settings {
   char *music_folder;   /* directory to scan */
} Settings;

/* ------------------------------
   Player State
   ------------------------------ */
typedef struct _Player_State
{
    /* Main window */
    Evas_Object *win;

    /* LEFT PANE */
    Evas_Object *artist_grid; /* NEW: Artists view (gengrid) */
    Evas_Object *genlist;     /* Tracks view */
    Evas_Object *gengrid;     /* Albums view */

    /* RIGHT PANE */
    Evas_Object *title_label;
    Evas_Object *album_art;
    Evas_Object *emotion;
    Evas_Object *slider;
    Evas_Object *album_tracklist;
    Evas_Object *volume_slider;
    Evas_Object *slider_indicator;
    Evas_Object *lbl_time_text;     // left label: "time"
    Evas_Object *lbl_time_total;    // right label: duration



    /* Playback state */
    Eina_List *current_album_tracks;
    int current_index;
    Eina_Bool suppress_tracklist_callbacks;

    /* Album playback mode */
    Eina_Bool album_mode;
    const char *current_album;   /* ✔ correct */

    /* Settings */
    Settings *settings;

    /* Library */
    Library *lib;

    /* Current filter */
    Filter_Mode filter;

    /* Artist filter (used by artist grid → albums) */
    const char *current_artist;

    Eina_Bool duration_set;


} Player_State;



/* ------------------------------
   Function Declarations
   ------------------------------ */

/* library.c */
Library *library_new(void);
void     library_free(Library *lib);
void     library_add_track(Library *lib, Track *t);

/* playback.c */
void playback_init(Player_State *ps);
void playback_play(Player_State *ps);
void playback_pause(Player_State *ps);
void playback_seek(Player_State *ps, double rel);
void playback_track_start(Player_State *ps, Track *t);
void playback_album_start(Player_State *ps, const char *album);
void playback_set_volume(Player_State *ps, double vol);

/* ui.c */
void ui_setup(Player_State *ps);
void ui_refresh_current(Player_State *ps);

/* scanner.c */
void scanner_start(Player_State *ps, const char *path);

/* album art update */
void ui_update_album_art(Player_State *ps, Track *t);

/* Playback API */
void playback_resume(Player_State *ps);
void playback_pause(Player_State *ps);
void playback_next(Player_State *ps);
void playback_prev(Player_State *ps);
void playback_seek(Player_State *ps, double pos);
void playback_set_volume(Player_State *ps, double vol);
void playback_track_start(Player_State *ps, Track *t);


#endif
