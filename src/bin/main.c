#include "player.h"
#include "artist_image_fetch.h"

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   if (!eina_init()) return 1;
   if (!eio_init()) {
      eina_shutdown();
      return 1;
   }

   artist_image_fetch_init();

   Player_State *ps = calloc(1, sizeof(Player_State));
   if (!ps) {
      eio_shutdown();
      eina_shutdown();
      return 1;
   }
   ps->lib = library_new();
   if (!ps->lib) {
      free(ps);
      eio_shutdown();
      eina_shutdown();
      return 1;
   }

   /* Initialize settings */
   ps->settings = calloc(1, sizeof(Settings));
   if (!ps->settings) {
      library_free(ps->lib);
      free(ps);
      eio_shutdown();
      eina_shutdown();
      return 1;
   }

   /* Default music folder */
   const char *home = getenv("HOME");
   if (home)
      ps->settings->music_folder = strdup(home);
   else
      ps->settings->music_folder = strdup("./");
   if (!ps->settings->music_folder) {
      free(ps->settings);
      library_free(ps->lib);
      free(ps);
      eio_shutdown();
      eina_shutdown();
      return 1;
   }

   Evas_Object *win = elm_win_util_standard_add("music-player", "Musek");
   if (!win) {
      free(ps->settings->music_folder);
      free(ps->settings);
      library_free(ps->lib);
      free(ps);
      eio_shutdown();
      eina_shutdown();
      return 1;
   }

   /* Let EFL delete the window automatically when closed */
   elm_win_autodel_set(win, EINA_TRUE);

   ps->win = win;

   ui_setup(ps);

   /* Use settings folder for scanning */
   char path[1024];
   snprintf(path, sizeof(path), "%s", ps->settings->music_folder);

   scanner_start(ps, path);

   evas_object_resize(win, 1000, 600);
   evas_object_show(win);

   /* Main loop */
   elm_run();

   scanner_shutdown();
   artist_image_fetch_shutdown();

   if (ps->progress_timer) {
      ecore_timer_del(ps->progress_timer);
      ps->progress_timer = NULL;
   }

   /*
    * IMPORTANT:
    * Do NOT call evas_object_del(ps->win) here.
    * elm_win_autodel_set() already deleted the window.
    * Calling it again causes:
    *   "Eo ID ... is not a valid object"
    */

   /* Now it is safe to free the library */
   library_free(ps->lib);

   free(ps->settings->music_folder);
   free(ps->settings);

   /* Free Player_State */
   free(ps);

   /* Shutdown subsystems */
   eio_shutdown();
   eina_shutdown();

   return 0;
}
ELM_MAIN()
