#include "player.h"

static Eina_Lock _lib_lock;

/* ============================================================
   Helpers
   ============================================================ */

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

    if (!t1 && !t2) return 0;
    if (!t1) return -1;
    if (!t2) return 1;

    const char *title1 = t1->title ? t1->title : "";
    const char *title2 = t2->title ? t2->title : "";

    if (t1->track_no < t2->track_no) return -1;
    if (t1->track_no > t2->track_no) return 1;

    return strcasecmp(title1, title2);
}

static int
_album_sort_cb(const void *d1, const void *d2)
{
    const Album_Entry *a = d1;
    const Album_Entry *b = d2;

    const char *artist_a = a && a->artist ? a->artist : "";
    const char *artist_b = b && b->artist ? b->artist : "";
    const char *album_a  = a && a->album  ? a->album  : "";
    const char *album_b  = b && b->album  ? b->album  : "";

    int r = strcasecmp(artist_a, artist_b);
    if (r != 0) return r;

    return strcasecmp(album_a, album_b);
}

/* ============================================================
   Detect album art
   ============================================================ */

static void
_album_detect_art(Album_Entry *ae)
{
    if (!ae || !ae->path) {
        ae->art_path = eina_stringshare_add("data/noart.png");
        return;
    }

    char cover[PATH_MAX];
    char folder[PATH_MAX];

    const char *cover_names[] = {
        "cover.jpg", "cover.png", "Cover.jpg", "Cover.png",
        NULL
    };

    const char *folder_names[] = {
        "folder.jpg", "folder.png", "Folder.jpg", "Folder.png",
        NULL
    };

    for (int i = 0; cover_names[i]; i++) {
        snprintf(cover, sizeof(cover), "%s/%s", ae->path, cover_names[i]);
        if (ecore_file_exists(cover)) {
            ae->art_path = eina_stringshare_add(cover);
            return;
        }
    }

    for (int i = 0; folder_names[i]; i++) {
        snprintf(folder, sizeof(folder), "%s/%s", ae->path, folder_names[i]);
        if (ecore_file_exists(folder)) {
            ae->art_path = eina_stringshare_add(folder);
            return;
        }
    }

    ae->art_path = eina_stringshare_add("data/noart.png");
}

/* ============================================================
   Compilation Detection
   ============================================================ */

static void
_library_mark_compilations(Library *lib)
{
    Album_Entry *ae;
    Eina_List *l;

    EINA_LIST_FOREACH(lib->albums, l, ae) {

        const char *dir = ae->path;
        const char *first_artist = NULL;
        const char *first_album  = NULL;
        Eina_Bool multi_artist = EINA_FALSE;

        Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
        Eina_List *tracks_list;

        EINA_ITERATOR_FOREACH(it, tracks_list) {
            Track *t;
            Eina_List *tl;

            EINA_LIST_FOREACH(tracks_list, tl, t) {

                if (!t->dir || strcmp(t->dir, dir) != 0)
                    continue;

                if (!first_artist) {
                    first_artist = t->artist;
                    first_album  = t->album;
                } else {
                    if (strcasecmp(first_artist, t->artist) != 0)
                        multi_artist = EINA_TRUE;
                }
            }
        }

        eina_iterator_free(it);

        if (multi_artist &&
            first_album && ae->album &&
            strcasecmp(first_album, ae->album) == 0)
        {
            ae->is_compilation = EINA_TRUE;
        }
    }
}

void library_mark_compilations(Library *lib)
{
    _library_mark_compilations(lib);
}

/* ============================================================
   Library Init
   ============================================================ */

Library *
library_new(void)
{
    Library *lib = calloc(1, sizeof(Library));
    if (!lib) return NULL;

    lib->album_tracks = eina_hash_string_superfast_new(NULL);
    eina_lock_new(&_lib_lock);

    return lib;
}

/* ============================================================
   Add Track
   ============================================================ */

void
library_add_track(Library *lib, Track *t)
{
    if (!lib || !t) return;
    if (!t->title || !t->artist || !t->album || !t->path || !t->dir) return;

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

    /* --- Add album entry (directory-based) --- */
    Eina_List *l;
    Album_Entry *ae;
    Eina_Bool exists = EINA_FALSE;

    EINA_LIST_FOREACH(lib->albums, l, ae) {
        if (ae && ae->path && !strcmp(ae->path, t->dir)) {
            exists = EINA_TRUE;
            break;
        }
    }

    if (!exists) {
        Album_Entry *new_ae = calloc(1, sizeof(Album_Entry));
        new_ae->artist = eina_stringshare_add(t->artist);
        new_ae->album  = eina_stringshare_add(t->album);
        new_ae->path   = eina_stringshare_add(t->dir);
        new_ae->display_artist = new_ae->artist;

        _album_detect_art(new_ae);

        lib->albums = eina_list_append(lib->albums, new_ae);
        lib->albums = eina_list_sort(lib->albums,
                                     eina_list_count(lib->albums),
                                     _album_sort_cb);
    }

    /* --- Add track to album_tracks hash using artist|album key --- */
    char keybuf[512];
    snprintf(keybuf, sizeof(keybuf), "%s|%s",
             t->artist ? t->artist : "",
             t->album  ? t->album  : "");

/* --- Add track to album_tracks hash using directory key --- */
const char *key = eina_stringshare_add(t->dir);

Eina_List *tracks = eina_hash_find(lib->album_tracks, key);
tracks = eina_list_append(tracks, t);
tracks = eina_list_sort(tracks,
                        eina_list_count(tracks),
                        _track_no_cmp);

eina_hash_set(lib->album_tracks, key, tracks);


    eina_lock_release(&_lib_lock);
}

/* ============================================================
   Tracks for album directory
   ============================================================ */

Eina_List *
library_tracks_for_album_dir(Library *lib, const Album_Entry *ae)
{
    if (!lib || !ae || !ae->path) return NULL;

    Eina_List *result = NULL;

    Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
    Eina_List *tracks_list;

    EINA_ITERATOR_FOREACH(it, tracks_list) {
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(tracks_list, l, t) {
            if (t->dir && !strcmp(t->dir, ae->path)) {
                result = eina_list_append(result, t);
            }
        }
    }

    eina_iterator_free(it);

    result = eina_list_sort(result,
                            eina_list_count(result),
                            _track_no_cmp);

    return result;
}

/* ============================================================
   Free Library
   ============================================================ */

void
library_free(Library *lib)
{
    if (!lib) return;

    eina_lock_free(&_lib_lock);

    /* Free tracks */
    Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
    Eina_List *tracks_list;

    EINA_ITERATOR_FOREACH(it, tracks_list) {
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(tracks_list, l, t) {
            eina_stringshare_del(t->title);
            eina_stringshare_del(t->artist);
            eina_stringshare_del(t->album);
            eina_stringshare_del(t->path);
            eina_stringshare_del(t->dir);
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

    /* Free albums */
    Album_Entry *ae;

    EINA_LIST_FOREACH(lib->albums, l, ae) {
        eina_stringshare_del(ae->artist);
        eina_stringshare_del(ae->album);
        eina_stringshare_del(ae->path);
        eina_stringshare_del(ae->art_path);

        if (ae->display_artist && ae->display_artist != ae->artist)
            eina_stringshare_del(ae->display_artist);

        free(ae);
    }

    eina_list_free(lib->albums);

    free(lib);
}
/* ============================================================
   Public wrappers (used by UI)
   ============================================================ */

Eina_Bool
library_album_contains_artist(Library *lib, const Album_Entry *ae, const char *artist)
{
    if (!lib || !ae || !artist) return EINA_FALSE;

    Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
    Eina_List *track_list;

    EINA_ITERATOR_FOREACH(it, track_list) {
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(track_list, l, t) {
            if (t->dir && !strcmp(t->dir, ae->path)) {
                if (!strcasecmp(t->artist, artist)) {
                    eina_iterator_free(it);
                    return EINA_TRUE;
                }
            }
        }
    }

    eina_iterator_free(it);
    return EINA_FALSE;
}

Eina_Bool
library_directory_is_compilation(Library *lib, const char *dir)
{
    if (!lib || !dir) return EINA_FALSE;

    const char *first_artist = NULL;
    const char *first_album = NULL;

    Eina_Iterator *it = eina_hash_iterator_data_new(lib->album_tracks);
    Eina_List *tracks_list;

    EINA_ITERATOR_FOREACH(it, tracks_list) {
        Track *t;
        Eina_List *l;

        EINA_LIST_FOREACH(tracks_list, l, t) {
            if (!t->dir || strcmp(t->dir, dir) != 0)
                continue;

            if (!first_artist) {
                first_artist = t->artist;
                first_album  = t->album;
            } else {
                if (strcasecmp(first_artist, t->artist) != 0) {
                    if (first_album && t->album &&
                        !strcasecmp(first_album, t->album)) {

                        eina_iterator_free(it);
                        return EINA_TRUE;
                    }
                }
            }
        }
    }

    eina_iterator_free(it);
    return EINA_FALSE;
}
