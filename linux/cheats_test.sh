#!/bin/bash
# Builds linux/cheats_test.c with runtime/cheats.c and runs it in a temporary folder; fails when a "want" line differs
set -e -o pipefail
source "$(dirname "$(readlink -f "$0")")/env.sh"
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
mkdir -p "$T/game_data/cheats"
gcc -O1 -w -I "$W/runtime" -o "$T/cheats_test" "$W/linux/cheats_test.c" "$W/runtime/cheats.c"
( cd "$T" && ./cheats_test ) | tee "$T/out.txt" | grep "want"
python3 -I - "$T/out.txt" <<'PY'
import re, sys
bad = 0
for ln in open(sys.argv[1]):
    m = re.search(r'[=:]\s*(.+?)\s*\(want (.+?)(,.*)?\)', ln)
    if m and m.group(1).strip() != m.group(2).strip():
        print('FAIL:', ln.strip()); bad = 1
print('cheats test: %s' % ('FAILED' if bad else 'all passed')); sys.exit(bad)
PY
