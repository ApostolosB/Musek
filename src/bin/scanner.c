#include "player.h"

void artist_image_prefetch_all(Player_State *ps);

/* Keep track of active scan jobs (instead of storing all Eio_File handles) */
static int scan_jobs = 0;

/* Serialize TagLib access */
static Eina_Lock taglib_lock;
static Ecore_Timer *ui_refresh_timer = NULL;

/* ------------------------------------------------------------
 * Worker job: passed to ecore_thread_pool_run()
 * ------------------------------------------------------------ */
typedef struct _Worker_Job {
    Player_State *ps;
    char *path;
} Worker_Job;

typedef struct _Add_Job {
    Player_State *ps;
    Track        *t;
} Add_Job;

static Eina_Bool
_ui_refresh_cb(void *data)
{
    Player_State *ps = data;
    ui_refresh_timer = NULL;
    ui_refresh_current(ps);
    return EINA_FALSE;
}

static void
ui_refresh_schedule(Player_State *ps)
{
    if (ui_refresh_timer)
        return;

    /* Coalesce many scan updates into one UI rebuild. */
    ui_refresh_timer = ecore_timer_add(0.25, _ui_refresh_cb, ps);
}

static void
track_free(Track *t)
{
    if (!t) return;

    eina_stringshare_del(t->title);
    eina_stringshare_del(t->artist);
    eina_stringshare_del(t->album);
    eina_stringshare_del(t->path);
    eina_stringshare_del(t->dir);
    free(t);
}

/* ------------------------------------------------------------
 * Done / Error callbacks
 * ------------------------------------------------------------ */
static void
scan_done_cb(void *data, Eio_File *handler)
{
    Player_State *ps = data;

    if (scan_jobs > 0 && --scan_jobs == 0) {
        /* All scanning is finished here */

        /* 1. Stop any pending coalesced UI refresh */
        if (ui_refresh_timer) {
            ecore_timer_del(ui_refresh_timer);
            ui_refresh_timer = NULL;
        }

        /* 2. Mark compilation albums now that the library is complete */
        if (ps && ps->lib) {
            library_mark_compilations(ps->lib);
        }

        /* 3. Refresh UI once with the final, marked library */
        if (ps)
            ui_refresh_current(ps);

        printf("SCAN COMPLETE: starting artist image prefetch\n");

        if (ps)
            artist_image_prefetch_all(ps);
    }
}


static void
scan_error_cb(void *data, Eio_File *handler, int error)
{
    if (scan_jobs > 0)
        scan_jobs--;
}

/* ------------------------------------------------------------
 * Extract metadata from a file using TagLib
 * ------------------------------------------------------------ */
static Track *
track_from_file(const char *path)
{
    if (!path || !path[0])
        return NULL;

    TagLib_File *tf = taglib_file_new(path);
    if (!tf) return NULL;

    TagLib_Tag *tag = taglib_file_tag(tf);
    if (!tag) {
        taglib_file_free(tf);
        return NULL;
    }

    const char *title  = taglib_tag_title(tag);
    const char *artist = taglib_tag_artist(tag);
    const char *album  = taglib_tag_album(tag);
    unsigned int track_no = taglib_tag_track(tag);

    Track *t = calloc(1, sizeof(Track));
    if (!t) {
        taglib_file_free(tf);
        return NULL;
    }

    t->title  = eina_stringshare_add(title && title[0] ? title : path);
    t->artist = eina_stringshare_add(artist ? artist : "");
    t->album  = eina_stringshare_add(album ? album : "");
    t->path   = eina_stringshare_add(path);

    /* More efficient directory extraction (no temporary malloc) */
    const char *slash = strrchr(path, '/');
    if (slash) {
        size_t len = (size_t)(slash - path);
        t->dir = eina_stringshare_add_length(path, len);
    } else {
        t->dir = eina_stringshare_add("");
    }

    t->track_no = (int)track_no;

    if (!t->title || !t->artist || !t->album || !t->path || !t->dir) {
        track_free(t);
        taglib_tag_free_strings();
        taglib_file_free(tf);
        return NULL;
    }

    taglib_tag_free_strings();
    taglib_file_free(tf);
    return t;
}

/* ------------------------------------------------------------
 * Main-loop callback: add track to library + refresh UI
 * ------------------------------------------------------------ */
static void
_library_add_cb(void *data)
{
    Add_Job *aj = data;

    library_add_track(aj->ps->lib, aj->t);
    ui_refresh_schedule(aj->ps);

    free(aj);
}

/* ------------------------------------------------------------
 * Worker thread: parse metadata, then hand off to main loop
 * ------------------------------------------------------------ */
static void
_scan_worker(void *data, Ecore_Thread *thread)
{
    Worker_Job *job = data;
    if (!job || !job->path) {
        free(job);
        return;
    }

    eina_lock_take(&taglib_lock);
    Track *t = track_from_file(job->path);
    eina_lock_release(&taglib_lock);

    if (t) {
        Add_Job *aj = calloc(1, sizeof(Add_Job));
        if (aj) {
            aj->ps = job->ps;
            aj->t  = t;
            ecore_main_loop_thread_safe_call_async(_library_add_cb, aj);
        } else {
            track_free(t);
        }
    }

    free(job->path);
    free(job);
}

/* ------------------------------------------------------------
 * Eio filter: allow directories + audio files
 * ------------------------------------------------------------ */
static Eina_Bool
scan_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
    if (info->type == EINA_FILE_DIR)
        return EINA_TRUE;

    if (info->type != EINA_FILE_REG &&
        info->type != EINA_FILE_UNKNOWN &&
        info->type != EINA_FILE_LNK)
        return EINA_FALSE;

    const char *ext = strrchr(info->path, '.');
    if (!ext)
        return EINA_FALSE;

    ext++;  // skip the dot

    /* Supported audio formats */
    if (!strcasecmp(ext, "mp3")  ||
        !strcasecmp(ext, "flac") ||
        !strcasecmp(ext, "ogg")  ||
        !strcasecmp(ext, "wav")  ||
        !strcasecmp(ext, "m4a")  ||
        !strcasecmp(ext, "aac")  ||
        !strcasecmp(ext, "opus") ||
        !strcasecmp(ext, "aiff") ||
        !strcasecmp(ext, "aif")  ||
        !strcasecmp(ext, "wma")  ||
        !strcasecmp(ext, "alac") ||
        !strcasecmp(ext, "mp2")  ||
        !strcasecmp(ext, "mp1"))
    {
        return EINA_TRUE;
    }

    return EINA_FALSE;
}

/* ------------------------------------------------------------
 * Eio main callback: called for each file or directory
 * ------------------------------------------------------------ */
static void
scan_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
    Player_State *ps = data;
    if (!ps || !info || !info->path[0])
        return;

    if (info->type == EINA_FILE_DIR) {
        /* Optionally skip symlinked directories to avoid weird recursion */
        if (info->type == EINA_FILE_LNK)
            return;

        Eio_File *sub = eio_file_direct_ls(
            info->path,
            scan_filter_cb,
            scan_main_cb,
            scan_done_cb,
            scan_error_cb,
            ps
        );

        if (sub)
            scan_jobs++;

        return;
    }

    Worker_Job *job = calloc(1, sizeof(Worker_Job));
    if (!job)
        return;

    job->ps   = ps;
    job->path = strdup(info->path);
    if (!job->path) {
        free(job);
        return;
    }

    /* Use thread pool to avoid unbounded thread creation */
    Ecore_Thread *thr = ecore_thread_run(_scan_worker, NULL, NULL, job);
    if (!thr) {
        free(job->path);
        free(job);
    }
}

/* ------------------------------------------------------------
 * Start scanning
 * ------------------------------------------------------------ */
void
scanner_start(Player_State *ps, const char *path)
{
    if (!ps || !path || !path[0])
        return;

    static Eina_Bool lock_inited = EINA_FALSE;
    if (!lock_inited) {
        eina_lock_new(&taglib_lock);
        lock_inited = EINA_TRUE;
    }

    Eio_File *f = eio_file_direct_ls(
        path,
        scan_filter_cb,
        scan_main_cb,
        scan_done_cb,
        scan_error_cb,
        ps
    );

    if (f)
        scan_jobs++;
}

void
scanner_shutdown(void)
{
    if (ui_refresh_timer) {
        ecore_timer_del(ui_refresh_timer);
        ui_refresh_timer = NULL;
    }
}
void scanner_reset(void) {
    scan_jobs = 0;
}
