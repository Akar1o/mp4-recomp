Metroid Prime 4 Beyond - recompiled for PC (Linux / Steam Deck build)
=============================================================

This package has the program and the compiled game code only. No Nintendo files are included: you need your own
dumped copy of Metroid Prime 4 Beyond 2.1.0.

1. Put your game files next to the program:
     Metroid Prime 4 Beyond_data/exefs/   the ExeFS of 2.1.0 (main, main.npdm, rtld, sdk, ...)
     Metroid Prime 4 Beyond_data/romfs/   the RomFS with the 2.1.0 update merged in (system, packs, ...)
2. Start it with ./run.sh (on a Steam Deck: add run.sh to Steam as a non-Steam game, in Desktop mode).

Needs: an OpenGL 4.3 driver (Mesa or NVIDIA), SDL2 (controllers and audio) and zlib, which SteamOS and most
desktop distributions already have. Under Wayland the game runs through XWayland.

Settings: "Metroid Prime 4 Beyond.ini" (rewritten with every option explained on the first start). In game: F1 or the
Home / Guide button opens the menu. Saves: Metroid Prime 4 Beyond_data/save. Problems: attach "Metroid Prime 4 Beyond.log".

Metroid Prime 4 Beyond is (c) Nintendo. This project is not affiliated with or endorsed by Nintendo.
