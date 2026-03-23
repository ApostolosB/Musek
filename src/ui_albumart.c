#include "player.h"
#include <Ecore_File.h>
#include <libgen.h>   // dirname()
#include <string.h>
#include <stdlib.h>

static const char *album_art_candidates[] = {
    "album.jpg",
    "Album.jpg",
    "album.png",
    "Album.png",
    "folder.jpg",
    "folder.png",
    "Folder.jpg",
    "Folder.png",
    "Cover.jpg",
    "cover.png",
    "NULL"
};

void
ui_update_album_art(Player_State *ps, Track *t)
{
    if (!ps || !t || !t->path) return;

    // 1. Get directory of the track
    char *path_copy = strdup(t->path);
    char *dir = dirname(path_copy);   // modifies path_copy

    printf("Track path: %s\n", t->path);
    printf("Album directory: %s\n", dir);

    // 2. Try each candidate file
    char fullpath[PATH_MAX];
    const char **p = album_art_candidates;
    const char *found = NULL;

    while (*p) {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, *p);
            printf("Checking: %s\n", fullpath);
        if (ecore_file_exists(fullpath)) {
                    printf("FOUND: %s\n", fullpath);
            found = strdup(fullpath);
            break;
        }
        p++;
    }

    free(path_copy);

    // 3. If nothing found, use fallback from source tree
    if (!found) {
        found = strdup("data/noart.png");
    }

    // 4. Update the UI image widget
    elm_image_file_set(ps->album_art, found, NULL);

    free((char*)found);
}
