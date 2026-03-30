#include "player.h"

void populate_current_album_tracklist(Player_State *ps);
static void playback_length_changed_cb(void *data, Evas_Object *obj, void *event_info);

static Eina_Bool
progress_update_cb(void *data)
{
    Player_State *ps = data;
    double pos = emotion_object_position_get(ps->emotion);
    double len = emotion_object_play_length_get(ps->emotion);

    /* NEW: update duration label once when len becomes valid */
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
        /* Update slider position */
        elm_slider_value_set(ps->slider, pos / len);

        /* Update tooltip (current time) */
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
   if (!ps->album_mode || !ps->current_album_tracks) return;

   ps->current_index++;
   Track *next = eina_list_nth(ps->current_album_tracks, ps->current_index);
   if (!next) {
      ps->album_mode = EINA_FALSE;
      ps->current_album = NULL;
      ps->current_album_tracks = NULL;
      ps->current_index = 0;
      return;
   }

   populate_current_album_tracklist(ps);
   playback_track_start(ps, next);
}

void
playback_init(Player_State *ps)
{
   ecore_timer_add(0.1, progress_update_cb, ps);

   evas_object_smart_callback_add(ps->emotion, "playback_finished",
                                  playback_finished_cb, ps);

   /* NEW: update duration when Emotion knows it */
   evas_object_smart_callback_add(ps->emotion, "length_changed",
                                  playback_length_changed_cb, ps);
}


void
playback_play(Player_State *ps)
{
   emotion_object_play_set(ps->emotion, EINA_TRUE);
}

void
playback_pause(Player_State *ps)
{
   emotion_object_play_set(ps->emotion, EINA_FALSE);
}

void
playback_seek(Player_State *ps, double rel)
{
   double len = emotion_object_play_length_get(ps->emotion);
   if (len > 0.0)
      emotion_object_position_set(ps->emotion, rel * len);
}

static void
update_title(Player_State *ps, Track *t)
{
   char buf[512];
   snprintf(buf, sizeof(buf),
            "<b>%s - %s</b>",
            (t->artist && t->artist[0]) ? t->artist : "Unknown Artist",
            (t->album  && t->album[0])  ? t->album  : "Unknown Album");

   elm_object_text_set(ps->title_label, buf);
}

void
playback_track_start(Player_State *ps, Track *t)
{
   if (!ps || !t) return;

   /* Load file into Emotion */
   emotion_object_file_set(ps->emotion, t->path);
   emotion_object_play_set(ps->emotion, EINA_TRUE);
   ps->duration_set = EINA_FALSE;

   /* Update title + album art */
   update_title(ps, t);
   ui_update_album_art(ps, t);

   /* -----------------------------------------
      NEW: Update duration label (right of slider)
      ----------------------------------------- */
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
playback_album_start(Player_State *ps, const char *album)
{
   if (!ps || !album) return;

   Eina_List *tracks = eina_hash_find(ps->lib->album_tracks, album);
   if (!tracks) return;

   ps->album_mode = EINA_TRUE;
   ps->current_album = album;
   ps->current_album_tracks = tracks;
   ps->current_index = 0;

   Track *t = eina_list_nth(tracks, 0);
   if (t) playback_track_start(ps, t);
}

void
playback_set_volume(Player_State *ps, double vol)
{
   if (!ps || !ps->emotion) return;
   emotion_object_audio_volume_set(ps->emotion, vol);
}

/* ============================================================
   NEW: wrappers used by UI callbacks
   ============================================================ */

void
playback_resume(Player_State *ps)
{
   playback_play(ps);
}

void
playback_next(Player_State *ps)
{
   if (!ps || !ps->album_mode || !ps->current_album_tracks) return;

   ps->current_index++;
   Track *next = eina_list_nth(ps->current_album_tracks, ps->current_index);
   if (!next) {
      ps->album_mode = EINA_FALSE;
      ps->current_album = NULL;
      ps->current_album_tracks = NULL;
      ps->current_index = 0;
      return;
   }

   populate_current_album_tracklist(ps);
   playback_track_start(ps, next);
}

void
playback_prev(Player_State *ps)
{
   if (!ps || !ps->album_mode || !ps->current_album_tracks) return;

   if (ps->current_index <= 0)
      return;

   ps->current_index--;
   Track *prev = eina_list_nth(ps->current_album_tracks, ps->current_index);
   if (!prev) return;

   populate_current_album_tracklist(ps);
   playback_track_start(ps, prev);
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

