#include "player.h"

void artist_image_prefetch_all(Player_State *ps);


/* Keep recursive Eio_File handles alive */
static Eina_List *scan_jobs = NULL;

/* Serialize TagLib access */
static Eina_Lock taglib_lock;

/* ------------------------------------------------------------
 * Worker job: passed to ecore_thread_run()
 * ------------------------------------------------------------ */
typedef struct _Worker_Job {
    Player_State *ps;
    char *path;
} Worker_Job;

typedef struct _Add_Job {
    Player_State *ps;
    Track        *t;
} Add_Job;

/* ------------------------------------------------------------
 * Done / Error callbacks
 * ------------------------------------------------------------ */
static void
scan_done_cb(void *data, Eio_File *handler)
{
    Player_State *ps = data;

    scan_jobs = eina_list_remove(scan_jobs, handler);

    if (!scan_jobs)
    {
        printf("SCAN COMPLETE: starting artist image prefetch\n");
        artist_image_prefetch_all(ps);
    }
}


static void
scan_error_cb(void *data, Eio_File *handler, int error)
{
    scan_jobs = eina_list_remove(scan_jobs, handler);
}

/* ------------------------------------------------------------
 * Extract metadata from a file using TagLib
 * ------------------------------------------------------------ */
static Track *
track_from_file(const char *path)
{
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
    t->title  = eina_stringshare_add(title && title[0] ? title : path);
    t->artist = eina_stringshare_add(artist ? artist : "");
    t->album  = eina_stringshare_add(album ? album : "");
    t->path   = eina_stringshare_add(path);

    /* Correct directory extraction */
    const char *slash = strrchr(path, '/');
    if (slash) {
        size_t len = slash - path;
        char *tmp = malloc(len + 1);
        memcpy(tmp, path, len);
        tmp[len] = '\0';

        t->dir = eina_stringshare_add(tmp);
        free(tmp);
    } else {
        t->dir = eina_stringshare_add("");
    }

    t->track_no = (int)track_no;

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
    ui_refresh_current(aj->ps);

    free(aj);
}

/* ------------------------------------------------------------
 * Worker thread: parse metadata, then hand off to main loop
 * ------------------------------------------------------------ */
static void
_scan_worker(void *data, Ecore_Thread *thread)
{
    Worker_Job *job = data;

    eina_lock_take(&taglib_lock);
    Track *t = track_from_file(job->path);
    eina_lock_release(&taglib_lock);

    if (t) {
        Add_Job *aj = calloc(1, sizeof(Add_Job));
        aj->ps = job->ps;
        aj->t  = t;

        ecore_main_loop_thread_safe_call_async(_library_add_cb, aj);
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
    if (info->type == EINA_FILE_DIR) {

        Eio_File *sub = eio_file_direct_ls(
            info->path,
            scan_filter_cb,
            scan_main_cb,
            scan_done_cb,
            scan_error_cb,
            data
        );

        if (sub)
            scan_jobs = eina_list_append(scan_jobs, sub);

        return;
    }

    Worker_Job *job = calloc(1, sizeof(Worker_Job));
    job->ps   = data;
    job->path = strdup(info->path);

    ecore_thread_run(_scan_worker, NULL, NULL, job);
}

/* ------------------------------------------------------------
 * Start scanning
 * ------------------------------------------------------------ */
void
scanner_start(Player_State *ps, const char *path)
{
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
        scan_jobs = eina_list_append(scan_jobs, f);
}
