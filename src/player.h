#ifndef PLAYER_H
#define PLAYER_H

#include <Elementary.h>
#include <Emotion.h>
#include <Eio.h>
#include <Eina.h>
#include <Ecore.h>
#include <taglib/tag_c.h>

/* ------------------------------
   Track Structure
   ------------------------------ */
typedef struct _Track {
   char *title;
   char *artist;
   char *album;
   char *path;
   int   track_no;
} Track;

/* ------------------------------
   Album Entry (NEW)
   Used for hierarchical sorting:
   artist → album
   ------------------------------ */
typedef struct {
   char *artist;
   char *album;
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
typedef struct _Player_State {
   Evas_Object *win;
   Evas_Object *emotion;
   Evas_Object *slider;
   Evas_Object *title_label;
   Evas_Object *album_art;
   Evas_Object *genlist;
   Evas_Object *album_tracklist;
   Evas_Object *volume_slider;

   Library     *lib;
   Filter_Mode  filter;

   /* album playback state */
   Eina_Bool    album_mode;   
   Eina_Bool suppress_tracklist_callbacks;

   const char  *current_album;
   Eina_List   *current_album_tracks;
   int          current_index;

   Settings    *settings;   /* user settings */
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

#endif
