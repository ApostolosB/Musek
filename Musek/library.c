#include "player.h"
#include <string.h>
#include <stdlib.h>

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

Library *
library_new(void)
{
   Library *lib = calloc(1, sizeof(Library));
   lib->album_tracks = eina_hash_string_superfast_new(NULL);
   return lib;
}

static Eina_Bool
_list_contains_str(Eina_List *list, const char *str)
{
   char *s;
   Eina_List *l;
   EINA_LIST_FOREACH(list, l, s) {
      if (!strcasecmp(s, str)) return EINA_TRUE;
   }
   return EINA_FALSE;
}

void
library_add_track(Library *lib, Track *t)
{
   if (!lib || !t) return;

   if (t->artist && t->artist[0]) {
      if (!_list_contains_str(lib->artists, t->artist)) {
         lib->artists = eina_list_append(lib->artists, _strdup0(t->artist));
         lib->artists = eina_list_sort(lib->artists, eina_list_count(lib->artists), _strcasecmp_cb);
      }
   }

   if (t->album && t->album[0]) {
      if (!_list_contains_str(lib->albums, t->album)) {
         lib->albums = eina_list_append(lib->albums, _strdup0(t->album));
         lib->albums = eina_list_sort(lib->albums, eina_list_count(lib->albums), _strcasecmp_cb);
      }
   }

   const char *album_key = (t->album && t->album[0]) ? t->album : "";
   Eina_List *tracks = eina_hash_find(lib->album_tracks, album_key);
   tracks = eina_list_append(tracks, t);
   tracks = eina_list_sort(tracks, eina_list_count(tracks), _track_no_cmp);
   eina_hash_set(lib->album_tracks, album_key, tracks);
}

void
library_free(Library *lib)
{
   if (!lib) return;

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

   char *s;
   Eina_List *l;
   EINA_LIST_FOREACH(lib->artists, l, s) free(s);
   eina_list_free(lib->artists);

   EINA_LIST_FOREACH(lib->albums, l, s) free(s);
   eina_list_free(lib->albums);

   free(lib);
}
