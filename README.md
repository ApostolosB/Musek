A vibe coded music player written in EFL.

Compile with:
gcc main.c ui.c library.c playback.c scanner.c -o player \
  $(pkg-config --cflags --libs elementary emotion eio eina ecore) \
  -ltag_c
