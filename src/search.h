#ifndef SEARCH_H
#define SEARCH_H

#include "player.h"

void search_filter_artists(Player_State *ps, const char *q);
void search_filter_albums(Player_State *ps, const char *q);
void search_filter_tracks(Player_State *ps, const char *q);

#endif
