#!/bin/sh

set -eu

OGG_VERSION=1.3.6
OGG_SHA256=83e6704730683d004d20e21b8f7f55dcb3383cdf84c0daedf30bde175f774638
VORBIS_COMMIT=1b75110b5a2754ba1931d82dd83cb822b266a21d
VORBIS_REPOSITORY=https://github.com/xiph/vorbis.git

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUTPUT_FILE="$SCRIPT_DIR/minivorbis.h"
TEMP_OUTPUT="$SCRIPT_DIR/minivorbis.h.tmp"
WORK_DIR=

cleanup()
{
    rm -f "$TEMP_OUTPUT" "$TEMP_OUTPUT.next"

    if [ -n "$WORK_DIR" ]
    then
        rm -rf "$WORK_DIR"
    fi
}

trap cleanup EXIT HUP INT TERM

verify_sha256()
{
    expected=$1
    file=$2

    if command -v sha256sum >/dev/null 2>&1
    then
        actual=$(sha256sum "$file" | awk '{print $1}')
    else
        actual=$(shasum -a 256 "$file" | awk '{print $1}')
    fi

    if [ "$actual" != "$expected" ]
    then
        echo "SHA-256 mismatch for $file" >&2
        exit 1
    fi
}

if [ "$#" -eq 0 ]
then
    WORK_DIR=$(mktemp -d "${TMPDIR:-/tmp}/geargrafx-minivorbis.XXXXXX")
    OGG_ARCHIVE="$WORK_DIR/libogg-$OGG_VERSION.tar.gz"
    VORBIS_DIR="$WORK_DIR/vorbis-$VORBIS_COMMIT"

    curl --fail --location --output "$OGG_ARCHIVE" \
        "https://downloads.xiph.org/releases/ogg/libogg-$OGG_VERSION.tar.gz"
    verify_sha256 "$OGG_SHA256" "$OGG_ARCHIVE"

    tar -xzf "$OGG_ARCHIVE" -C "$WORK_DIR"
    OGG_DIR="$WORK_DIR/libogg-$OGG_VERSION"

    git init --quiet "$VORBIS_DIR"
    git -C "$VORBIS_DIR" fetch --quiet --depth 1 "$VORBIS_REPOSITORY" "$VORBIS_COMMIT"
    git -C "$VORBIS_DIR" checkout --quiet --detach FETCH_HEAD

    if [ "$(git -C "$VORBIS_DIR" rev-parse HEAD)" != "$VORBIS_COMMIT" ]
    then
        echo "Unexpected xiph/vorbis commit" >&2
        exit 1
    fi
elif [ "$#" -eq 2 ]
then
    OGG_DIR=$1
    VORBIS_DIR=$2
else
    echo "Usage: $0 [libogg-$OGG_VERSION-source-dir xiph-vorbis-$VORBIS_COMMIT-source-dir]" >&2
    exit 1
fi

if [ ! -f "$OGG_DIR/src/bitwise.c" ] || [ ! -f "$VORBIS_DIR/lib/vorbisfile.c" ]
then
    echo "Invalid libogg or libvorbis source directory" >&2
    exit 1
fi

transform_output()
{
    expression=$1
    sed "$expression" "$TEMP_OUTPUT" > "$TEMP_OUTPUT.next"
    mv "$TEMP_OUTPUT.next" "$TEMP_OUTPUT"
}

cat > "$TEMP_OUTPUT" <<EOF
/*
  minivorbis.h -- libvorbis decoder in a single header
  Project URL: https://github.com/edubart/minivorbis

  This is libogg $OGG_VERSION + xiph/vorbis $VORBIS_COMMIT contained in a single header
  to be bundled in C/C++ applications with ease for decoding OGG sound files.
  Ogg Vorbis is a open general-purpose compressed audio format
  for mid to high quality audio and music at fixed and variable bitrates.

  Do the following in *one* C file to implement Ogg and Vorbis:
    #define OGG_IMPL
    #define VORBIS_IMPL

  Optionally provide the following defines:
    OV_EXCLUDE_STATIC_CALLBACKS     - exclude the default static callbacks

  Note that almost no modification was made in the Ogg/Vorbis implementation code,
  thus there are some C variable names that may collide with your code,
  therefore it is best to declare the implementation in dedicated C file.

  LICENSE
    BSD-like License, same as libogg and libvorbis, see end of file.
*/
EOF

cat "$OGG_DIR/include/ogg/os_types.h" >> "$TEMP_OUTPUT"
cat "$OGG_DIR/include/ogg/ogg.h" >> "$TEMP_OUTPUT"
transform_output "s@# *include <ogg/config_types.h>@/* config_types.h */\\
#include <stdint.h>\\
typedef int16_t ogg_int16_t;\\
typedef uint16_t ogg_uint16_t;\\
typedef int32_t ogg_int32_t;\\
typedef uint32_t ogg_uint32_t;\\
typedef int64_t ogg_int64_t;\\
typedef uint64_t ogg_uint64_t;@"

cat "$VORBIS_DIR/include/vorbis/codec.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/include/vorbis/vorbisfile.h" >> "$TEMP_OUTPUT"

cat >> "$TEMP_OUTPUT" <<EOF
#ifdef OGG_IMPL
#ifdef __cplusplus
extern "C" {
#endif
EOF
cat "$OGG_DIR/src/bitwise.c" >> "$TEMP_OUTPUT"
cat "$OGG_DIR/src/framing.c" >> "$TEMP_OUTPUT"
transform_output "/#include \"crctable.h\"/r $OGG_DIR/src/crctable.h"
cat >> "$TEMP_OUTPUT" <<EOF
#ifdef __cplusplus
}
#endif
#endif /* OGG_IMPL */
#ifdef VORBIS_IMPL
#ifdef __cplusplus
extern "C" {
#endif
EOF

cat "$VORBIS_DIR/lib/misc.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/os.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/mdct.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/envelope.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/codebook.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/smallft.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/codec_internal.h" >> "$TEMP_OUTPUT"
transform_output "/#include \"psy.h\"/r $VORBIS_DIR/lib/psy.h"
transform_output "/#include \"backends.h\"/r $VORBIS_DIR/lib/backends.h"
transform_output "/#include \"bitrate.h\"/r $VORBIS_DIR/lib/bitrate.h"
transform_output "/#include \"highlevel.h\"/r $VORBIS_DIR/lib/highlevel.h"
cat "$VORBIS_DIR/lib/lookup_data.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lookup.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lpc.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lsp.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/masking.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/registry.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/scales.h" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/window.h" >> "$TEMP_OUTPUT"

cat "$VORBIS_DIR/lib/mdct.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/smallft.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/block.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/envelope.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/window.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lsp.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lpc.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/analysis.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/synthesis.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/psy.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/info.c" >> "$TEMP_OUTPUT"
transform_output 's/FLOOR1_fromdB_LOOKUP/_FLOOR1_fromdB_LOOKUP/'
cat "$VORBIS_DIR/lib/floor1.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/floor0.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/res0.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/mapping0.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/registry.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/codebook.c" >> "$TEMP_OUTPUT"
transform_output 's/bitreverse/_bitreverse/'
cat "$VORBIS_DIR/lib/sharedbook.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/lookup.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/bitrate.c" >> "$TEMP_OUTPUT"
cat "$VORBIS_DIR/lib/vorbisfile.c" >> "$TEMP_OUTPUT"

cat >> "$TEMP_OUTPUT" <<EOF
#ifdef __cplusplus
}
#endif
#endif /* VORBIS_IMPL */
/*
Copyright (c) 2002-2020 Xiph.org Foundation
Copyright (c) 2020 Eduardo Bart (https://github.com/edubart)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

- Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

- Neither the name of the Xiph.org Foundation nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
EOF

transform_output 's/#include "\([^"]*\)"/\/\*#include "\1"\*\//'
transform_output 's/# *include <ogg\/\([^>]*\)>/\/\*#include <ogg\/\1>\*\//'

mv "$TEMP_OUTPUT" "$OUTPUT_FILE"
trap - EXIT HUP INT TERM
cleanup
