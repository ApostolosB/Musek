#include "player.h"
#include <Emotion.h>
#include "album.h"

void _album_thumb_path_get(const char *dir, char *out, size_t out_size);


void
ui_update_album_art(Player_State *ps, Track *t)
{
    if (!ps || !t || !t->dir) return;

    const char *dir = t->dir;
    char full[PATH_MAX];
    char *found = NULL;

    /* 1. Folder art */
    const char *candidates[] = {
        "cover.jpg", "cover.png",
        "Cover.jpg", "Cover.png",
        "folder.jpg", "folder.png",
        "Folder.jpg", "Folder.png",
        "album.jpg",  "album.png",
        "Album.jpg",  "Album.png",
        NULL
    };

    for (const char **p = candidates; *p; p++) {
        snprintf(full, sizeof(full), "%s/%s", dir, *p);
        if (ecore_file_exists(full)) {
            found = strdup(full);
            break;
        }
    }

/* 2. Embedded artwork via Emotion */
if (!found && ps->emotion && t->path) {
    Evas_Object *art = emotion_file_meta_artwork_get(
        ps->emotion, t->path, EMOTION_ARTWORK_PREVIEW_IMAGE);

    if (!art)
        art = emotion_file_meta_artwork_get(
            ps->emotion, t->path, EMOTION_ARTWORK_IMAGE);

    if (art) {
        /* Build the correct album thumbnail path */
        char thumb_path[PATH_MAX];
        _album_thumb_path_get(t->dir, thumb_path, sizeof(thumb_path));

        /* Save embedded artwork directly into album cache */
        evas_object_image_save(art, thumb_path, NULL, "quality=90");
        evas_object_del(art);

        /* Update player-side album art */
        elm_image_file_set(ps->album_art, NULL, NULL);
        elm_image_file_set(ps->album_art, thumb_path, NULL);

        return;
    }
}


    /* 3. Fallback */
    if (!found)
        found = strdup("data/noart.png");

    elm_image_file_set(ps->album_art, NULL, NULL);
    elm_image_file_set(ps->album_art, found, NULL);


    
    free(found);
}
