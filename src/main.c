#include "player.h"

EAPI_MAIN int
elm_main(int argc, char **argv)
{
   eina_init();
   eio_init();

   Player_State *ps = calloc(1, sizeof(Player_State));
   ps->lib = library_new();

   /* Initialize settings */
   ps->settings = calloc(1, sizeof(Settings));

   /* Default music folder */
   const char *home = getenv("HOME");
   if (home)
      ps->settings->music_folder = strdup(home);
   else
      ps->settings->music_folder = strdup("./");

   Evas_Object *win = elm_win_util_standard_add("music-player", "Musek");

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

   /*
    * IMPORTANT:
    * Do NOT call evas_object_del(ps->win) here.
    * elm_win_autodel_set() already deleted the window.
    * Calling it again causes:
    *   "Eo ID ... is not a valid object"
    */

   /* Now it is safe to free the library */
   library_free(ps->lib);

   /* Free Player_State */
   free(ps);

   /* Shutdown subsystems */
   eio_shutdown();
   eina_shutdown();

   return 0;
}
ELM_MAIN()
