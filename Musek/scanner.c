#include "player.h"
#include <string.h>
#include <stdlib.h>
#include <Eio.h>
#include <Eina.h>

typedef struct _Scan_Ctx {
   Player_State *ps;
} Scan_Ctx;

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
   t->title    = strdup(title && title[0] ? title : path);
   t->artist   = strdup(artist ? artist : "");
   t->album    = strdup(album ? album : "");
   t->path     = strdup(path);
   t->track_no = (int)track_no;

   taglib_tag_free_strings();
   taglib_file_free(tf);
   return t;
}

static Eina_Bool
scan_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   /* Allow directories so recursion can happen */
   if (info->type == EINA_FILE_DIR)
      return EINA_TRUE;

   if (info->type != EINA_FILE_REG)
      return EINA_FALSE;

   const char *path = info->path;
   const char *ext = strrchr(path, '.');
   if (!ext) return EINA_FALSE;
   ext++;

   if (!strcasecmp(ext, "mp3") ||
       !strcasecmp(ext, "flac") ||
       !strcasecmp(ext, "ogg") ||
       !strcasecmp(ext, "wav") ||
       !strcasecmp(ext, "m4a"))
      return EINA_TRUE;

   return EINA_FALSE;
}

static void
scan_error_cb(void *data, Eio_File *handler, int error)
{
   /* Optional: log error */
}

static void
scan_done_cb(void *data, Eio_File *handler)
{
   (void)data;
   (void)handler;
}

static void
scan_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   /* Manual recursion because eio_file_direct_ls() is NOT recursive */
   if (info->type == EINA_FILE_DIR) {
      eio_file_direct_ls(
         info->path,
         scan_filter_cb,
         scan_main_cb,
         scan_done_cb,
         scan_error_cb,
         data
      );
      return;
   }

   Scan_Ctx *ctx = data;
   Player_State *ps = ctx->ps;

   Track *t = track_from_file(info->path);
   if (!t) return;

   library_add_track(ps->lib, t);
   ui_refresh_current(ps);
}

void
scanner_start(Player_State *ps, const char *path)
{
   Scan_Ctx *ctx = calloc(1, sizeof(Scan_Ctx));
   ctx->ps = ps;

   eio_file_direct_ls(
      path,
      scan_filter_cb,
      scan_main_cb,
      scan_done_cb,
      scan_error_cb,
      ctx
   );
}
