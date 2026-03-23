#ifndef UI_H
#define UI_H

#include "player.h"

void ui_setup(Player_State *ps);
void ui_refresh_current(Player_State *ps);
void populate_current_album_tracklist(Player_State *ps);

#endif
