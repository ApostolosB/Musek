#include <strings.h>

#include "ui_internal.h"
#include "artist_image_fetch.h"

/* genlist item classes (tracks view) */
Elm_Genlist_Item_Class itc_album_header;
Elm_Genlist_Item_Class itc_track;

/* gengrid item classes */
Elm_Gengrid_Item_Class itc_artist_tile;   /* NEW */
Elm_Gengrid_Item_Class itc_artist_group;
Elm_Gengrid_Item_Class itc_album;

/* Called when artist thumbnail is ready */
static void
_artist_thumb_ready_cb(const char *thumb_path, void *data)
{
    Item_Data *id = data;
    if (!id || !id->gengrid_item) return;

    /* Force gengrid to redraw this tile */
    elm_gengrid_item_update(id->gengrid_item);
}


static void _album_del(void *data, Evas_Object *obj);


/* Build a stable thumbnail path under ~/.cache/musek/album_thumbs */
 void
_album_thumb_path_get(const char *dir, char *out, size_t out_size)
{
    const char *home = getenv("HOME");
    if (!home) home = ".";

    char cache_dir[PATH_MAX];
    snprintf(cache_dir, sizeof(cache_dir),
             "%s/.cache/musek/album_thumbs", home);

    /* Ensure directory exists */
    ecore_file_mkpath(cache_dir);

    /* Hash the album directory to get a safe filename */
    unsigned int h = eina_hash_superfast(dir, strlen(dir));

    snprintf(out, out_size, "%s/%u.jpg", cache_dir, h);
}

/* Generate a small thumbnail from src → dst using the given Evas canvas.
 * src may be:
 *   - a real image file (folder.jpg, cover.png, etc.)
 *   - an audio file with embedded artwork
 */
static Eina_Bool
_album_thumb_generate(Evas *evas,
                      const char *src,
                      const char *dst,
                      int size)
{
    Eina_Bool ok = EINA_FALSE;

    /* 1. Try loading src as a normal image */
    Evas_Object *img = evas_object_image_add(evas);
    evas_object_image_file_set(img, src, NULL);

    int w = 0, h = 0;
    evas_object_image_size_get(img, &w, &h);

    /* If loading failed, try embedded artwork via Emotion */
    if (w <= 0 || h <= 0)
    {
        evas_object_del(img);

        /* Create a temporary Emotion object */
        Evas_Object *em = emotion_object_add(evas);
        if (!em) return EINA_FALSE;

        emotion_object_file_set(em, src);

        /* Try preview artwork first */
        Evas_Object *art = emotion_file_meta_artwork_get(
            em, src, EMOTION_ARTWORK_PREVIEW_IMAGE);

        if (!art)
            art = emotion_file_meta_artwork_get(
                em, src, EMOTION_ARTWORK_IMAGE);

        if (!art)
        {
            evas_object_del(em);
            return EINA_FALSE;  /* No embedded art either */
        }

        /* Save embedded artwork to dst */
        ok = evas_object_image_save(art, dst, NULL, "quality=90");
        evas_object_del(art);
        evas_object_del(em);

        return ok;
    }

    /* 2. Normal image path worked — generate thumbnail */
    int max_side = (w > h) ? w : h;
    double scale = (double)size / (double)max_side;
    int nw = (int)(w * scale);
    int nh = (int)(h * scale);

    if (nw <= 0) nw = 1;
    if (nh <= 0) nh = 1;

    evas_object_image_fill_set(img, 0, 0, nw, nh);
    evas_object_resize(img, nw, nh);

    ok = evas_object_image_save(img, dst, NULL, "quality=90");

    evas_object_del(img);
    return ok;
}



/* ============================================================
   ARTIST TILE (gengrid)
   ============================================================ */

static char *
_artist_tile_text_get(void *data, Evas_Object *obj, const char *part)
{
    Item_Data *id = data;

    if (!strcmp(part, "elm.text"))
        return strdup(id->u.name ? id->u.name : "");

    return NULL;
}

static Evas_Object *
_artist_tile_content_get(void *data, Evas_Object *obj, const char *part)
{
    /* Support both common swallow parts */
    if (strcmp(part, "elm.swallow.icon") &&
        strcmp(part, "elm.swallow.content"))
        return NULL;

    Item_Data *id = data;

    Evas_Object *img = elm_image_add(obj);
    elm_image_aspect_fixed_set(img, EINA_TRUE);
    evas_object_size_hint_min_set(img, 64, 64);

    /* Build path to artist.png in the installed data directory */
    const char *data_dir = elm_app_data_dir_get();
    char artist_path[PATH_MAX];
    snprintf(artist_path, sizeof(artist_path), "%s/artist.png", data_dir);

    /* Compute expected thumbnail path */
    char *thumb = artist_image_thumb_path_get(id->u.name);

    if (thumb && ecore_file_exists(thumb)) {
        /* Thumbnail already exists */
        elm_image_file_set(img, thumb, NULL);
    } else {
        /* Show placeholder */
        elm_image_file_set(img, artist_path, NULL);

        printf("FETCH: requesting thumb for artist: %s\n", id->u.name);
        printf("UI: prefetch running? %d\n", artist_image_prefetch_is_running());

        /* Only trigger UI fetch if prefetcher is NOT running */
        if (!artist_image_prefetch_is_running()) {
            printf("UI: prefetch NOT running, doing direct fetch\n");
            artist_image_fetch(id->u.name, _artist_thumb_ready_cb, id);
        } else {
            printf("UI: prefetch IS running, skipping UI fetch\n");
        }
    }

    free(thumb);

    evas_object_show(img);
    return img;
}







static void
_artist_tile_del(void *data, Evas_Object *obj)
{
    free(data);
}

/* ============================================================
   ARTIST TILE SELECTED CALLBACK
   ============================================================ */

void
artist_tile_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;
    Elm_Object_Item *it = event_info;
    Item_Data *id = elm_object_item_data_get(it);

    if (!id || !id->u.name)
        return;

    ps->current_artist = id->u.name;
    ps->filter = FILTER_ALBUMS;

    ui_refresh_current(ps);
}
/* ============================================================
   POPULATE ARTISTS GRID 
   ============================================================ */
void
populate_artists_grid(Player_State *ps)
{
    elm_gengrid_clear(ps->artist_grid);

    char *name;
    Eina_List *l;

    EINA_LIST_FOREACH(ps->lib->artists, l, name) {

        Item_Data *id = calloc(1, sizeof(Item_Data));
        id->type = ITEM_ARTIST;
        id->u.name = name;
        id->ps = ps;

        /* Append tile and store the gengrid item pointer */
        Elm_Object_Item *it = elm_gengrid_item_append(
            ps->artist_grid,
            &itc_artist_tile,
            id,
            artist_tile_selected_cb,
            ps
        );

        id->gengrid_item = it;   /* <-- REQUIRED for async thumbnail updates */
    }
}



/* ============================================================
   ALBUM TILE TEXT + CONTENT
   ============================================================ */

static char *
_album_text_get(void *data, Evas_Object *obj, const char *part)
{
    Item_Data *id = data;
    Album_Entry *a = id->u.album_entry;

    if (!strcmp(part, "elm.text"))
        return strdup(a && a->album ? a->album : "");

    return NULL;
}

static Evas_Object *
_album_content_get(void *data, Evas_Object *obj, const char *part)
{
    if (strcmp(part, "elm.swallow.icon"))
        return NULL;

    Item_Data   *id = data;
    Album_Entry *a  = id->u.album_entry;

    Evas_Object *img = elm_image_add(obj);
    elm_image_aspect_fixed_set(img, EINA_TRUE);
    evas_object_show(img);

    /* ---------------------------------------------------------
     * Fallback path that works BOTH installed and uninstalled
     * --------------------------------------------------------- */
    char noart_path[PATH_MAX];

    /* 1) Installed case: /usr/share/musek/data/noart.png */
    snprintf(noart_path, sizeof(noart_path),
             "%s/data/noart.png", elm_app_data_dir_get());

    /* 2) Uninstalled case: ./data/noart.png */
    if (!ecore_file_exists(noart_path)) {
        snprintf(noart_path, sizeof(noart_path),
                 "data/noart.png");
    }

    /* --------------------------------------------------------- */

    if (!a || !a->path || !a->path[0]) {
        elm_image_file_set(img, noart_path, NULL);
        return img;
    }

    char thumb_path[PATH_MAX];
    _album_thumb_path_get(a->path, thumb_path, sizeof(thumb_path));

    if (!ecore_file_exists(thumb_path)) {
        Evas     *evas = evas_object_evas_get(obj);
        Eina_Bool ok   = EINA_FALSE;

        /* 1. Try folder art */
        if (a->art_path &&
            strcmp(a->art_path, "data/noart.png") != 0)
        {
            ok = _album_thumb_generate(evas, a->art_path, thumb_path, 256);
        }

        /* 2. Try embedded art */
        if (!ok && id->ps && id->ps->lib && id->ps->emotion) {
            Eina_List *tracks = library_tracks_for_album_dir(id->ps->lib, a);
            if (tracks) {
                Track *t = eina_list_data_get(tracks);
                if (t && t->path) {
                    Evas_Object *art =
                        emotion_file_meta_artwork_get(id->ps->emotion,
                                                      t->path,
                                                      EMOTION_ARTWORK_PREVIEW_IMAGE);

                    if (!art)
                        art = emotion_file_meta_artwork_get(id->ps->emotion,
                                                            t->path,
                                                            EMOTION_ARTWORK_IMAGE);

                    if (art) {
                        evas_object_image_save(art, thumb_path, NULL, "quality=90");
                        evas_object_del(art);
                        ok = EINA_TRUE;
                    }
                }
            }
        }

        /* 3. Save fallback into cache */
        if (!ok) {
            Evas_Object *tmp = evas_object_image_add(evas);
            evas_object_image_file_set(tmp, noart_path, NULL);
            evas_object_image_save(tmp, thumb_path, NULL, "quality=90");
            evas_object_del(tmp);
        }
    }

    elm_image_file_set(img, thumb_path, NULL);
    return img;
}


/* ============================================================
   GENGRID ARTIST GROUP HEADER TEXT
   ============================================================ */

static char *
_artist_group_text_get(void *data, Evas_Object *obj, const char *part)
{
    Item_Data *id = data;

    if (!strcmp(part, "elm.text"))
        return strdup(id->u.name ? id->u.name : "");

    return NULL;
}

/* ============================================================
   POPULATE ALBUMS (full list)
   ============================================================ */

void
populate_albums(Player_State *ps)
{
    elm_gengrid_clear(ps->gengrid);

    elm_gengrid_horizontal_set(ps->gengrid, EINA_FALSE);
    elm_gengrid_item_size_set(ps->gengrid, 110, 150);

    Album_Entry *ae;
    Eina_List *l;

    /* Track directories already shown */
    Eina_Hash *seen_dirs = eina_hash_string_superfast_new(NULL);

    const char *last_artist = NULL;
    Eina_Bool compilation_header_added = EINA_FALSE;

    /* PASS 1 — Normal albums grouped by artist */
    EINA_LIST_FOREACH(ps->lib->albums, l, ae) {

        /* Skip duplicate directories */
        if (eina_hash_find(seen_dirs, ae->path))
            continue;

        /* Skip compilations in this pass */
        if (ae->is_compilation)
            continue;

        eina_hash_add(seen_dirs, ae->path, (void*)1);

        if (!last_artist || strcasecmp(last_artist, ae->artist) != 0) {

            Item_Data *gid = calloc(1, sizeof(Item_Data));
            gid->type = ITEM_ARTIST;
            gid->u.name = ae->artist;
            gid->ps = ps;

            elm_gengrid_item_append(
                ps->gengrid,
                &itc_artist_group,
                gid,
                NULL,
                NULL
            );

            last_artist = ae->artist;
        }

        Item_Data *aid = calloc(1, sizeof(Item_Data));
        aid->type = ITEM_ALBUM;
        aid->u.album_entry = ae;
        aid->ps = ps;

        elm_gengrid_item_append(
            ps->gengrid,
            &itc_album,
            aid,
            album_tile_selected_cb,
            ps
        );
    }

    /* PASS 2 — Compilation section */
    EINA_LIST_FOREACH(ps->lib->albums, l, ae) {

        if (!ae->is_compilation)
            continue;

        /* Ensure each compilation directory only appears once */
        if (eina_hash_find(seen_dirs, ae->path))
            continue;

        eina_hash_add(seen_dirs, ae->path, (void*)1);

        if (!compilation_header_added) {
            Item_Data *gid = calloc(1, sizeof(Item_Data));
            gid->type = ITEM_ARTIST;
            gid->u.name = "Compilations";
            gid->ps = ps;

            elm_gengrid_item_append(
                ps->gengrid,
                &itc_artist_group,
                gid,
                NULL,
                NULL
            );

            compilation_header_added = EINA_TRUE;
        }

        Item_Data *aid = calloc(1, sizeof(Item_Data));
        aid->type = ITEM_ALBUM;
        aid->u.album_entry = ae;
        aid->ps = ps;

        elm_gengrid_item_append(
            ps->gengrid,
            &itc_album,
            aid,
            album_tile_selected_cb,
            ps
        );
    }

    eina_hash_free(seen_dirs);
}


/* ============================================================
   POPULATE ALBUMS FOR SPECIFIC ARTIST
   ============================================================ */

void
populate_albums_for_artist(Player_State *ps, const char *artist)
{
    elm_gengrid_clear(ps->gengrid);

    elm_gengrid_horizontal_set(ps->gengrid, EINA_FALSE);
    elm_gengrid_item_size_set(ps->gengrid, 110, 150);

    Album_Entry *ae;
    Eina_List *l;

    Eina_Hash *seen_dirs = eina_hash_string_superfast_new(NULL);

    EINA_LIST_FOREACH(ps->lib->albums, l, ae) {

        /* Skip duplicate directories */
        if (eina_hash_find(seen_dirs, ae->path))
            continue;

        eina_hash_add(seen_dirs, ae->path, (void*)1);

        /* Compilation logic: include only if artist appears in that directory */
        if (library_directory_is_compilation(ps->lib, ae->path)) {
            if (!library_album_contains_artist(ps->lib, ae, artist))
                continue;
        } else {
            /* Normal album: match artist */
            if (strcasecmp(ae->artist, artist) != 0)
                continue;
        }

        Item_Data *aid = calloc(1, sizeof(Item_Data));
        aid->type = ITEM_ALBUM;
        aid->u.album_entry = ae;
        aid->ps = ps;

        elm_gengrid_item_append(
            ps->gengrid,
            &itc_album,
            aid,
            album_tile_selected_cb,
            ps
        );
    }

    eina_hash_free(seen_dirs);
}

/* ============================================================
   TRACK TEXT (FIXED)
   ============================================================ */

static char *
_track_text_get(void *data, Evas_Object *obj, const char *part)
{
    Item_Data *id = data;
    Track *t = id->u.track;

    if (!strcmp(part, "elm.text"))
        return strdup(t && t->title ? t->title : "");

    return NULL;
}

/* ============================================================
   POPULATE TRACKS (genlist)
   ============================================================ */

void
populate_tracks(Player_State *ps)
{
    elm_genlist_clear(ps->genlist);

    Album_Entry *ae;
    Eina_List *l;

    /* Avoid duplicate directories */
    Eina_Hash *seen_dirs = eina_hash_string_superfast_new(NULL);

    EINA_LIST_FOREACH(ps->lib->albums, l, ae) {

        if (eina_hash_find(seen_dirs, ae->path))
            continue;

        eina_hash_add(seen_dirs, ae->path, (void*)1);

        /* Header */
        Item_Data *id_header = calloc(1, sizeof(Item_Data));
        id_header->type = ITEM_ALBUM_HEADER;
        id_header->album = ae->album;
        id_header->u.album_entry = ae;
        id_header->ps = ps;

        elm_genlist_item_append(ps->genlist,
                                &itc_album_header,
                                id_header,
                                NULL,
                                ELM_GENLIST_ITEM_NONE,
                                NULL,
                                ps);

        /* Collect all tracks in this directory */
        Eina_List *tracks = library_tracks_for_album_dir(ps->lib, ae);
        if (!tracks)
            continue;

        Track *t;
        Eina_List *lt;

        EINA_LIST_FOREACH(tracks, lt, t) {

            Item_Data *id = calloc(1, sizeof(Item_Data));
            id->type = ITEM_TRACK;
            id->album = ae->album;
            id->u.track = t;
            id->ps = ps;

            elm_genlist_item_append(ps->genlist,
                                    &itc_track,
                                    id,
                                    NULL,
                                    ELM_GENLIST_ITEM_NONE,
                                    tracklist_left_cb,
                                    ps);
        }
    }

    eina_hash_free(seen_dirs);
}


static void
_album_del(void *data, Evas_Object *obj)
{
    free(data);
}



/* ============================================================
   INIT ITEM CLASSES
   ============================================================ */

void
ui_populate_init(void)
{
    /* Artist tile (gengrid) */
    memset(&itc_artist_tile, 0, sizeof(itc_artist_tile));
    itc_artist_tile.item_style = "thumb";
    itc_artist_tile.func.text_get = _artist_tile_text_get;
    itc_artist_tile.func.content_get = _artist_tile_content_get;
    itc_artist_tile.func.del = _artist_tile_del;

    /* Album headers (tracks view) */
    memset(&itc_album_header, 0, sizeof(itc_album_header));
    itc_album_header.item_style = "default";
    itc_album_header.func.text_get = _album_text_get;
    itc_album_header.func.content_get = _album_content_get;
    itc_album_header.func.del = _album_del;

    /* Tracks (genlist) — FIXED */
    memset(&itc_track, 0, sizeof(itc_track));
    itc_track.item_style = "default";
    itc_track.func.text_get = _track_text_get;   /* FIX */
    itc_track.func.del = _album_del;

    /* Artist group headers (gengrid) */
    memset(&itc_artist_group, 0, sizeof(itc_artist_group));
    itc_artist_group.item_style = "group_index";
    itc_artist_group.func.text_get = _artist_group_text_get;
    itc_artist_group.func.del = _artist_tile_del;

    /* Album tiles (gengrid) */
    memset(&itc_album, 0, sizeof(itc_album));
    itc_album.item_style = "thumb";
    itc_album.func.text_get = _album_text_get;
    itc_album.func.content_get = _album_content_get;
    itc_album.func.del = _album_del;
}

