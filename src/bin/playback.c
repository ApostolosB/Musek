#include "player.h"

/* Forward declarations */
static Eina_Bool progress_update_cb(void *data);
static void playback_finished_cb(void *data, Evas_Object *obj, void *event_info);
static void playback_length_changed_cb(void *data, Evas_Object *obj, void *event_info);
void populate_current_album_tracklist(Player_State *ps);


/* ============================================================
   Playback-owned track helpers
   ============================================================ */

Track *
_track_clone(const Track *src)
{
    if (!src) return NULL;

    Track *t = calloc(1, sizeof(Track));
    if (!t) return NULL;

    t->title    = eina_stringshare_add(src->title    ? src->title    : "");
    t->artist   = eina_stringshare_add(src->artist   ? src->artist   : "");
    t->album    = eina_stringshare_add(src->album    ? src->album    : "");
    t->path     = eina_stringshare_add(src->path     ? src->path     : "");
    t->dir      = eina_stringshare_add(src->dir      ? src->dir      : "");
    t->track_no = src->track_no;

    return t;
}

void
_track_free_playback(Track *t)
{
    if (!t) return;

    if (t->title)  eina_stringshare_del(t->title);
    if (t->artist) eina_stringshare_del(t->artist);
    if (t->album)  eina_stringshare_del(t->album);
    if (t->path)   eina_stringshare_del(t->path);
    if (t->dir)    eina_stringshare_del(t->dir);

    free(t);
}

static void
_playback_clear_album(Player_State *ps)
{
    if (!ps) return;

    Track *t;
    Eina_List *l;

    EINA_LIST_FOREACH(ps->current_album_tracks, l, t)
        _track_free_playback(t);

    eina_list_free(ps->current_album_tracks);
    ps->current_album_tracks = NULL;

    ps->current_index = 0;
    ps->album_finished = EINA_FALSE;
    ps->album_mode = EINA_FALSE;

    if (ps->current_album) {
        eina_stringshare_del(ps->current_album);
        ps->current_album = NULL;
    }
}

/* ============================================================
   UI helpers
   ============================================================ */

static void
_update_title(Player_State *ps, Track *t)
{
    char buf[512];
    snprintf(buf, sizeof(buf),
             "<b>%s - %s</b>",
             (t->artist && t->artist[0]) ? t->artist : "Unknown Artist",
             (t->album  && t->album[0])  ? t->album  : "Unknown Album");

    elm_object_text_set(ps->title_label, buf);
}

/* ============================================================
   Core playback helper
   ============================================================ */

static void
_play_track(Player_State *ps, Track *t)
{
    if (!ps || !t || !ps->emotion) return;

    ps->current_track = t;
    ps->duration_set = EINA_FALSE;
    ps->needs_restart = EINA_FALSE;

    emotion_object_file_set(ps->emotion, t->path);
    emotion_object_position_set(ps->emotion, 0.0);
    emotion_object_play_set(ps->emotion, EINA_TRUE);

    _update_title(ps, t);
    ui_update_album_art(ps, t);
}

/* ============================================================
   Public API
   ============================================================ */

   void
playback_play_specific(Player_State *ps, Track *t)
{
    if (!ps || !t) return;
    _play_track(ps, t);
}


void
playback_init(Player_State *ps)
{
    if (!ps || !ps->emotion) return;

    ps->current_album_tracks = NULL;
    ps->current_index = 0;
    ps->album_mode = EINA_FALSE;
    ps->album_finished = EINA_FALSE;
    ps->current_track = NULL;
    ps->current_album = NULL;

    /* Restore old behavior: timer + callbacks */
    ps->progress_timer = ecore_timer_add(0.1, progress_update_cb, ps);

    evas_object_smart_callback_add(ps->emotion, "playback_finished",
                                   playback_finished_cb, ps);

    evas_object_smart_callback_add(ps->emotion, "length_changed",
                                   playback_length_changed_cb, ps);

    emotion_object_play_set(ps->emotion, EINA_FALSE);
}

void
playback_play(Player_State *ps)
{
    if (!ps || !ps->emotion) return;

    /* Restart album if finished */
    if (ps->album_finished && ps->current_album_tracks)
    {
        ps->album_finished = EINA_FALSE;
        ps->album_mode = EINA_TRUE;
        ps->current_index = 0;

        Track *first = eina_list_nth(ps->current_album_tracks, 0);
        if (first)
            _play_track(ps, first);
        

                populate_current_album_tracklist(ps);
        return;
    }

    /* Restart single track if needed */
    if (ps->needs_restart && ps->current_track)
    {
        emotion_object_file_set(ps->emotion, ps->current_track->path);
        emotion_object_position_set(ps->emotion, 0.0);
        ps->needs_restart = EINA_FALSE;
    }

    emotion_object_play_set(ps->emotion, EINA_TRUE);
}

void
playback_pause(Player_State *ps)
{
    if (!ps || !ps->emotion) return;
    emotion_object_play_set(ps->emotion, EINA_FALSE);
}

void
playback_seek(Player_State *ps, double rel)
{
    if (!ps || !ps->emotion) return;

    double len = emotion_object_play_length_get(ps->emotion);
    if (len > 0.0)
        emotion_object_position_set(ps->emotion, rel * len);
}

void
playback_set_volume(Player_State *ps, double vol)
{
    if (!ps || !ps->emotion) return;
    emotion_object_audio_volume_set(ps->emotion, vol);
}

void
playback_track_start(Player_State *ps, Track *t)
{
    if (!ps || !t) return;

    _playback_clear_album(ps);

    ps->album_mode = EINA_FALSE;
    ps->album_finished = EINA_FALSE;

    _play_track(ps, t);
}

void
playback_album_start(Player_State *ps, const char *album_dir)
{
    if (!ps || !ps->lib || !album_dir) return;

    _playback_clear_album(ps);

    /* Look up album by directory */
    Eina_List *orig = eina_hash_find(ps->lib->album_tracks, album_dir);
    if (!orig) return;

    Track *t;
    Eina_List *l;

    ps->current_album_tracks = NULL;

    /* Clone tracks */
    EINA_LIST_FOREACH(orig, l, t) {
        Track *copy = _track_clone(t);
        if (copy)
            ps->current_album_tracks =
                eina_list_append(ps->current_album_tracks, copy);
    }

    if (!ps->current_album_tracks)
        return;

    ps->album_mode = EINA_TRUE;
    ps->album_finished = EINA_FALSE;
    ps->current_index = 0;

    /* Store album identity (directory) */
    ps->current_album = eina_stringshare_add(album_dir);

    /* Start first track */
    Track *first = eina_list_nth(ps->current_album_tracks, 0);
    if (first)
        _play_track(ps, first);

    /* Update playlist highlight */
    populate_current_album_tracklist(ps);
}


void
playback_next(Player_State *ps)
{
    if (!ps || !ps->album_mode || !ps->current_album_tracks)
        return;

    int count = eina_list_count(ps->current_album_tracks);
    if (ps->current_index >= count - 1)
        return;

    ps->current_index++;

    Track *next = eina_list_nth(ps->current_album_tracks, ps->current_index);
    if (next){
        _play_track(ps, next);
        populate_current_album_tracklist(ps);
    }
}

void
playback_prev(Player_State *ps)
{
    if (!ps || !ps->album_mode || !ps->current_album_tracks)
        return;

    if (ps->current_index <= 0)
        return;

    ps->current_index--;

    Track *prev = eina_list_nth(ps->current_album_tracks, ps->current_index);
    if (prev){
    _play_track(ps, prev);
    populate_current_album_tracklist(ps);
}
}

/* ============================================================
   Emotion callbacks
   ============================================================ */

static Eina_Bool
progress_update_cb(void *data)
{
    Player_State *ps = data;
    double pos = emotion_object_position_get(ps->emotion);
    double len = emotion_object_play_length_get(ps->emotion);

    if (!ps->duration_set && len > 0.1)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d",
                 (int)(len / 60),
                 (int)((int)len % 60));

        elm_object_text_set(ps->lbl_time_total, buf);
        ps->duration_set = EINA_TRUE;
    }

    if (len > 0.0)
    {
        elm_slider_value_set(ps->slider, pos / len);

        if (ps->slider_indicator)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d:%02d",
                     (int)(pos / 60),
                     (int)((int)pos % 60));

            elm_object_tooltip_text_set(ps->slider_indicator, buf);
        }
    }

    return EINA_TRUE;
}

static void
playback_finished_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    /* Album mode: advance to next track */
    if (ps->album_mode && ps->current_album_tracks)
    {
        ps->current_index++;
        Track *next = eina_list_nth(ps->current_album_tracks, ps->current_index);

        if (!next)
        {
            /* Album finished */
            ps->album_finished = EINA_TRUE;
            ps->album_mode = EINA_FALSE;
            return;
        }

        /* Play next track WITHOUT clearing album mode */
        _play_track(ps, next);

        /* Update playlist highlight */
        populate_current_album_tracklist(ps);
        return;
    }

    /* Single-track mode */
    emotion_object_play_set(ps->emotion, EINA_FALSE);
    emotion_object_file_set(ps->emotion, NULL);
    ps->needs_restart = EINA_TRUE;
}


static void
playback_length_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
    Player_State *ps = data;

    double len = emotion_object_play_length_get(ps->emotion);
    if (len > 0.0)
    {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:%02d",
                 (int)(len / 60),
                 (int)((int)len % 60));

        elm_object_text_set(ps->lbl_time_total, buf);
    }
}
void
playback_resume(Player_State *ps)
{
    playback_play(ps);
}

