#include "player.h"
#include <string.h>
#include <stdlib.h>
#include <Eina.h>

static Eina_Lock _lib_lock;

/* ------------------------------
   Helpers
   ------------------------------ */
static char *
_strdup0(const char *s)
{
   if (!s) return strdup("");
   return strdup(s);
}

static int
_strcasecmp_cb(const void *d1, const void *d2)
{
   const char *a = d1;
   const char *b = d2;
   return strcasecmp(a, b);
}

static int
_track_no_cmp(const void *d1, const void *d2)
{
   const Track *t1 = d1;
   const Track *t2 = d2;

   if (t1->track_no < t2->track_no) return -1;
   if (t1->track_no > t2->track_no) return 1;

   return strcasecmp(t1->title, t2->title);
}

/* Sort albums by artist → album */
static int
_album_sort_cb(const void *d1, const void *d2)
{
    const Album_Entry *a = d1;
    const Album_Entry *b = d2;

    int r = strcasecmp(a->artist, b->artist);
    if (r != 0) return r;

    return strcasecmp(a->album, b->album);
}

/* ------------------------------
   Library Init
   ------------------------------ */
Library *
library_new(void)
{
   Library *lib = calloc(1, sizeof(Library));
   lib->album_tracks = eina_hash_string_superfast_new(NULL);

   eina_lock_new(&_lib_lock);

   return lib;
}

/* ------------------------------
   Add Track to Library
   ------------------------------ */
void
library_add_track(Library *lib, Track *t)
{
   if (!lib || !t) return;

   eina_lock_take(&_lib_lock);

   /* --- Add artist --- */
   if (t->artist && t->artist[0]) {
      if (!eina_list_search_unsorted(lib->artists, _strcasecmp_cb, t->artist)) {
         lib->artists = eina_list_append(lib->artists, _strdup0(t->artist));
         lib->artists = eina_list_sort(lib->artists,
                                       eina_list_count(lib->artists),
                                       _strcasecmp_cb);
      }
   }

   /* --- Add album entry (artist + album) only once --- */
   if (t->album && t->album[0]) {

      Eina_List *l;
      Album_Entry *ae;
      Eina_Bool exists = EINA_FALSE;

      /* Check if album already exists */
      EINA_LIST_FOREACH(lib->albums, l, ae) {
         if (!strcasecmp(ae->artist, t->artist) &&
             !strcasecmp(ae->album,  t->album)) {
            exists = EINA_TRUE;
            break;
         }
      }

      /* Add album only if not already present */
      if (!exists) {
         Album_Entry *new_ae = calloc(1, sizeof(Album_Entry));
         new_ae->artist = _strdup0(t->artist);
         new_ae->album  = _strdup0(t->album);

         lib->albums = eina_list_append(lib->albums, new_ae);
         lib->albums = eina_list_sort(lib->albums,
                                      eina_list_count(lib->albums),
                                      _album_sort_cb);
      }
   }

   /* --- Add track to album_tracks hash --- */
   const char *album_key = (t->album && t->album[0]) ? t->album : "";

   Eina_List *tracks = eina_hash_find(lib->album_tracks, album_key);
   tracks = eina_list_append(tracks, t);
   tracks = eina_list_sort(tracks,
                           eina_list_count(tracks),
                           _track_no_cmp);

   eina_hash_set(lib->album_tracks, album_key, tracks);

   eina_lock_release(&_lib_lock);
}

/* ------------------------------
   Free Library
   ------------------------------ */
void
library_free(Library *lib)
{
   if (!lib) return;

   eina_lock_free(&_lib_lock);

   /* Free album_tracks hash */
   Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
   Eina_List *tracks_list;

   EINA_ITERATOR_FOREACH(it, tracks_list) {
      Track *t;
      Eina_List *l;

      EINA_LIST_FOREACH(tracks_list, l, t) {
         free(t->title);
         free(t->artist);
         free(t->album);
         free(t->path);
         free(t);
      }

      eina_list_free(tracks_list);
   }

   eina_iterator_free(it);
   eina_hash_free(lib->album_tracks);

   /* Free artists */
   char *s;
   Eina_List *l;

   EINA_LIST_FOREACH(lib->artists, l, s)
      free(s);
   eina_list_free(lib->artists);

   /* Free albums (Album_Entry structs) */
   Album_Entry *ae;
   EINA_LIST_FOREACH(lib->albums, l, ae) {
      free(ae->artist);
      free(ae->album);
      free(ae);
   }
   eina_list_free(lib->albums);

   free(lib);
}
