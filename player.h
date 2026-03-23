#ifndef PLAYER_H
#define PLAYER_H

#include <Elementary.h>
#include <Emotion.h>
#include <Eio.h>
#include <Eina.h>
#include <Ecore.h>
#include <taglib/tag_c.h>

typedef struct _Track {
   char *title;
   char *artist;
   char *album;
   char *path;
   int   track_no;
} Track;

typedef enum {
   FILTER_ARTISTS,
   FILTER_ALBUMS,
   FILTER_TRACKS
} Filter_Mode;

typedef struct _Library {
   Eina_List *artists;      // char* (sorted)
   Eina_List *albums;       // char* (sorted)
   Eina_Hash *album_tracks; // key: album name (char*), value: Eina_List* of Track*
} Library; 

/* Simple settings structure (expandable later) */
typedef struct _Settings {
   char *music_folder;   /* directory to scan */
} Settings;


typedef struct _Player_State {
   Evas_Object *win;
   Evas_Object *emotion;
   Evas_Object *slider;
   Evas_Object *title_label;
   Evas_Object *album_art;
   Evas_Object *genlist;
   Evas_Object *album_tracklist;

   Library     *lib;
   Filter_Mode  filter;

   // album playback state
   Eina_Bool    album_mode;
   const char  *current_album;
   Eina_List   *current_album_tracks;
   int          current_index;

   Settings    *settings;   /* user settings */
} Player_State;

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

/* ui.c */
void ui_setup(Player_State *ps);
void ui_refresh_current(Player_State *ps);

/* scanner.c */
void scanner_start(Player_State *ps, const char *path);

#endif
