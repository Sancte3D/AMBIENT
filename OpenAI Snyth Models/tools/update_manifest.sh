#!/bin/sh
set -eu

find firmware tools tests docs -type f -print0 \
    | sort -z \
    | xargs -0 sha256sum > SOURCE_MANIFEST.sha256
sha256sum README.md CMakeLists.txt Makefile LICENSE-PROPRIETARY.md \
    >> SOURCE_MANIFEST.sha256
find audio/previews ui/previews -type f -print0 \
    | sort -z \
    | xargs -0 sha256sum > ASSET_MANIFEST.sha256
echo "updated SOURCE_MANIFEST.sha256 and ASSET_MANIFEST.sha256"
