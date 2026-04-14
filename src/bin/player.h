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

/* ------------------------------
   Album Structure
   ------------------------------ */
typedef struct {
    const char *artist;        /* eina_stringshare: primary/album artist */
    const char *album;         /* eina_stringshare */
    const char *path;          /* eina_stringshare: directory */
    const char *art_path;      /* eina_stringshare */

    /* Compilation handling */
    Eina_Bool   is_compilation;
    const char *display_artist;
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
   Eina_List *artists;      /* char* (sorted alphabetically) */
   Eina_List *albums;       /* Album_Entry* (sorted by artist → album) */
   /* key: "artist|album", value: Eina_List* of Track* */
   Eina_Hash *album_tracks;
} Library;

/* ------------------------------
   Settings
   ------------------------------ */
typedef struct _Settings {
   char *music_folder;
} Settings;

/* ------------------------------
   Player State
   ------------------------------ */
typedef struct _Player_State
{
    /* Main window */
    Evas_Object *win;

    /* LEFT PANE */
    Evas_Object *artist_grid;
    Evas_Object *genlist;
    Evas_Object *gengrid;

    /* Search bar */
    Evas_Object *search_entry;
    Eina_Bool    search_visible;

    /* RIGHT PANE */
    Evas_Object *title_label;
    Evas_Object *album_art;
    Evas_Object *emotion;
    Evas_Object *slider;
    Evas_Object *album_tracklist;
    Evas_Object *volume_slider;
    Evas_Object *slider_indicator;
    Evas_Object *lbl_time_text;
    Evas_Object *lbl_time_total;

    /* Playback state */
    Eina_List *current_album_tracks;   /* playback-owned cloned tracks */
    int current_index;
    Eina_Bool suppress_tracklist_callbacks;

    /* Album playback mode */
    Eina_Bool   album_mode;
    const char *current_album;         /* key: "artist|album" */

    /* Settings */
    Settings *settings;

    /* Library */
    Library *lib;

    /* Current filter */
    Filter_Mode filter;

    /* Artist filter */
    const char *current_artist;

    Eina_Bool    duration_set;
    Ecore_Timer *progress_timer;
    Eina_Bool    needs_restart;

    Track *current_track;              /* playback-owned or single track */
    Eina_Bool album_finished;

    Eina_Bool suppress_search_callbacks;

} Player_State;

/* ------------------------------
   Library API
   ------------------------------ */
Library *library_new(void);
void     library_free(Library *lib);
void     library_add_track(Library *lib, Track *t);
Eina_List *library_tracks_for_album_dir(Library *lib, const Album_Entry *ae);
Eina_Bool  library_album_contains_artist(Library *lib, const Album_Entry *ae, const char *artist);
Eina_Bool  library_directory_is_compilation(Library *lib, const char *dir);
void       library_mark_compilations(Library *lib);

/* ------------------------------
   Playback API
   ------------------------------ */
void playback_init(Player_State *ps);
void playback_play(Player_State *ps);
void playback_pause(Player_State *ps);
void playback_resume(Player_State *ps);
void playback_seek(Player_State *ps, double rel);
void playback_set_volume(Player_State *ps, double vol);

void playback_track_start(Player_State *ps, Track *t);
void playback_album_start(Player_State *ps, const char *album_key);
void playback_next(Player_State *ps);
void playback_prev(Player_State *ps);
void playback_play_specific(Player_State *ps, Track *t);


/* ------------------------------
   UI API
   ------------------------------ */
void ui_setup(Player_State *ps);
void ui_refresh_current(Player_State *ps);
void ui_update_album_art(Player_State *ps, Track *t);

/* ------------------------------
   Scanner API
   ------------------------------ */
void scanner_start(Player_State *ps, const char *path);
void scanner_shutdown(void);
void scanner_reset(void);

/* ------------------------------------------------------------
   Playback helpers (local to playback.c but declared here
   so playback.c stays clean and self-contained)
   ------------------------------------------------------------ */

/* Clone a track for playback-owned lists */
Track *_track_clone(const Track *src);

/* Free a playback-owned track */
void _track_free_playback(Track *t);

#endif /* PLAYER_H */
