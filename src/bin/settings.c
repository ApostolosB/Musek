#include "player.h"
#include <Ecore_File.h>

#define SETTINGS_PATH ".config/musek/settings.conf"

static char *
_settings_file_path(void)
{
    const char *home = getenv("HOME");
    if (!home) return strdup("settings.conf");

    char buf[PATH_MAX];
    snprintf(buf, sizeof(buf), "%s/%s", home, SETTINGS_PATH);
    return strdup(buf);
}

void
settings_load(Settings *s)
{
    if (!s) return;

    char *path = _settings_file_path();
    FILE *f = fopen(path, "r");
    if (!f) {
        free(path);
        return;  // no settings yet → keep defaults
    }

    char line[PATH_MAX];
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = 0;  // strip newline
        free(s->music_folder);
        s->music_folder = strdup(line);
    }

    fclose(f);
    free(path);
}

void
settings_save(const Settings *s)
{
    if (!s || !s->music_folder) return;

    char *path = _settings_file_path();

    /* ensure directory exists */
    char dir[PATH_MAX];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        ecore_file_mkpath(dir);
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        free(path);
        return;
    }

    fprintf(f, "%s\n", s->music_folder);
    fclose(f);
    free(path);
}
