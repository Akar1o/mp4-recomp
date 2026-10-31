#!/bin/bash
# Linux build: the H.264 decoder for the game's videos (NVDEC codec 3, runtime/host1x.c). Builds OpenH264 2.4.1 (BSD-2,
# the version of runtime/oh264/include) from its source release into runtime/oh264/lin/libdecoder.a + libcommon.a, the
# two static libraries the Windows build links from runtime/oh264/win. Without them the runtime builds with
# -DSWR_NO_H264 and videos stay black. The download is checked against its SHA-256 and kept in linux/appimage/cache.
# Run as your normal user: bash linux/build_openh264.sh (setup_and_build_runtime.sh runs it when the libraries are missing)
set -e -o pipefail
source "$(dirname "$(readlink -f "$0")")/env.sh"
need curl "curl"; need make "make"; need g++ "build-essential"
VER=2.4.1
URL=https://github.com/cisco/openh264/archive/refs/tags/v$VER.tar.gz
SHA=8ffbe944e74043d0d3fb53d4a2a14c94de71f58dbea6a06d0dc92369542958ea
C="$W/linux/appimage/cache"; OUT="$W/runtime/oh264/lin"; T="$C/openh264-$VER"
mkdir -p "$C" "$OUT"
f="$C/openh264-$VER.tar.gz"
if [ -f "$f" ] && ! echo "$SHA  $f" | sha256sum -c --quiet - 2>/dev/null; then rm -f "$f"; fi
[ -f "$f" ] || { echo "downloading OpenH264 $VER"; curl -fL --retry 3 -o "$f.part" "$URL" && mv "$f.part" "$f"; }
echo "$SHA  $f" | sha256sum -c --quiet - || { echo "error: openh264-$VER.tar.gz does not match its SHA-256" >&2; exit 1; }
rm -rf "$T"; tar -xzf "$f" -C "$C"
# x86-64 assembly with nasm when it is there (several times faster decoding), plain C otherwise
ASM=No; command -v nasm > /dev/null 2>&1 && ASM=Yes
make -C "$T" -j"$(nproc)" OS=linux ARCH=x86_64 USE_ASM=$ASM BUILDTYPE=Release CFLAGS_OPT="-O2 -fPIC" libdecoder.a libcommon.a > "$C/openh264-build.log" 2>&1 \
  || { tail -20 "$C/openh264-build.log" >&2; echo "error: the OpenH264 build failed (log: $C/openh264-build.log)" >&2; exit 1; }
cp -f "$T/libdecoder.a" "$T/libcommon.a" "$OUT/"
cp -f "$T/LICENSE" "$W/runtime/oh264/LICENSE"
rm -rf "$T"
echo "OpenH264 $VER (asm: $ASM): $OUT/libdecoder.a, $OUT/libcommon.a"
