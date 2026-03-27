#ifndef ARTIST_IMAGE_FETCH_H
#define ARTIST_IMAGE_FETCH_H

#include <Eina.h>

typedef void (*Artist_Image_Fetch_Done_Cb)(const char *thumb_path, void *data);

Eina_Bool artist_image_prefetch_is_running(void);

/**
 * Initialize artist image fetcher.
 * Call once at startup (after Ecore/Ecore_Con/Efreet init).
 */
void artist_image_fetch_init(void);

/**
 * Shutdown artist image fetcher.
 * Call at shutdown to free handlers/resources.
 */
void artist_image_fetch_shutdown(void);

/**
 * Get the expected thumbnail path for an artist.
 * Returned string must be freed by caller.
 */
char *artist_image_thumb_path_get(const char *artist);

/**
 * Asynchronously fetch an artist image from Google Images, generate a 256x256
 * thumbnail in ~/.cache/musek/artist_thumbs, and call cb when ready.
 *
 * If the thumbnail already exists, cb is called immediately.
 *
 * @param artist    Artist name (UTF-8).
 * @param cb        Callback called when thumbnail is ready (or failed).
 *                  thumb_path is NULL on failure.
 * @param data      User data passed to cb.
 */
void artist_image_fetch(const char *artist,
                        Artist_Image_Fetch_Done_Cb cb,
                        void *data);

#endif
