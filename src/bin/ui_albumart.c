#include "player.h"

void
ui_update_album_art(Player_State *ps, Track *t)
{
    if (!ps || !t || !t->dir) return;

    const char *dir = t->dir;
    char full[PATH_MAX];
    const char *found = NULL;

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

    if (!found)
        found = strdup("data/noart.png");

    /* FIX: clear previous image first */
    elm_image_file_set(ps->album_art, NULL, NULL);

    /* Now set the new one */
    elm_image_file_set(ps->album_art, found, NULL);

    free((char*)found);
}

