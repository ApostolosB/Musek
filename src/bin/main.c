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

   /* Try to load saved settings */
   {
       const char *home = getenv("HOME");
       if (home) {
           char cfgdir[PATH_MAX];
           char cfgfile[PATH_MAX];

           /* Build ~/.config/musek */
           strlcpy(cfgdir, home, sizeof(cfgdir));
           strlcat(cfgdir, "/.config/musek", sizeof(cfgdir));

           /* Build ~/.config/musek/settings.conf */
           strlcpy(cfgfile, cfgdir, sizeof(cfgfile));
           strlcat(cfgfile, "/settings.conf", sizeof(cfgfile));

           printf("Loading settings from: %s\n", cfgfile);

           FILE *f = fopen(cfgfile, "r");
           if (f) {
               char line[PATH_MAX];
               if (fgets(line, sizeof(line), f)) {
                   line[strcspn(line, "\r\n")] = 0;  // strip newline
                   ps->settings->music_folder = strdup(line);

                   printf("Loaded music folder: %s\n",
                          ps->settings->music_folder);
               }
               fclose(f);
           } else {
               printf("Settings file not found.\n");
           }
       }
   }

   /* If still NULL, fall back to HOME */
   if (!ps->settings->music_folder) {
       const char *home = getenv("HOME");
       ps->settings->music_folder = strdup(home ? home : "./");
   }

   /* REMOVE THIS — it overwrote the loaded value
   const char *home = getenv("HOME");
   if (home)
      ps->settings->music_folder = strdup(home);
   else
      ps->settings->music_folder = strdup("./");
   */

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

   scanner_shutdown();
   artist_image_fetch_shutdown();

   if (ps->progress_timer) {
      ecore_timer_del(ps->progress_timer);
      ps->progress_timer = NULL;
   }

   library_free(ps->lib);

   free(ps->settings->music_folder);
   free(ps->settings);

   free(ps);

   eio_shutdown();
   eina_shutdown();

   return 0;
}
ELM_MAIN()
