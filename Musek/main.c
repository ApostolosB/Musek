#include "player.h"
#include <stdlib.h>

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
   else ps->settings->music_folder = strdup("./");

   Evas_Object *win = elm_win_util_standard_add("music-player", "Musek");
   elm_win_autodel_set(win, EINA_TRUE);
   ps->win = win;

   ui_setup(ps);

   /* Use settings folder for scanning */
   char path[1024];
   snprintf(path, sizeof(path), "%s", ps->settings->music_folder);

   scanner_start(ps, path);

   evas_object_resize(win, 1000, 600);
   evas_object_show(win);

   elm_run();

   library_free(ps->lib);
   free(ps);

   eio_shutdown();
   eina_shutdown();
   return 0;
}
ELM_MAIN()
