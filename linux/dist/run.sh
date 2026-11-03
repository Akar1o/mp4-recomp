#!/bin/bash
# Metroid Prime 4 Beyond (Linux / Steam Deck): start the game from this folder. Extra arguments go to the game.
cd "$(dirname "$(readlink -f "$0")")"
if [ ! -d "Metroid Prime 4 Beyond_data/exefs" ] || [ ! -d "Metroid Prime 4 Beyond_data/romfs" ]; then
  echo "Put your own Metroid Prime 4 Beyond 2.1.0 files in 'Metroid Prime 4 Beyond_data/exefs' and 'Metroid Prime 4 Beyond_data/romfs' first (see README-SteamDeck.txt)." >&2
  exit 1
fi
exec "./Metroid Prime 4 Beyond" "$@"   # the runtime finds its _code.so, _data and .ini next to argv[0]
