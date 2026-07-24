#!/usr/bin/env sh
set -eu

pack_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

find "$pack_root" \
    -type f \
    \( -iname 'LICENSE' -o -iname 'LICENSE.*' \
       -o -iname 'COPYING' -o -iname 'COPYING.*' \
       -o -iname 'NOTICE' -o -iname 'NOTICE.*' \) \
    -printf '%P\n' \
    | LC_ALL=C sort \
    > "$pack_root/LICENSE_FILE_INDEX.txt"

(
    cd "$pack_root"
    find . \
        -type f \
        ! -name 'SHA256SUMS.txt' \
        -print0 \
        | LC_ALL=C sort -z \
        | xargs -0 sha256sum
) > "$pack_root/SHA256SUMS.txt"
