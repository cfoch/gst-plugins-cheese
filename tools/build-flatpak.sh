#!/bin/bash
set -e
set -x
rm -rf var metadata export build

APP_ID="io.github.cfoch.gst_plugins_cheese"

SOURCE_DIR="$(git rev-parse --show-toplevel)"
MANIFEST_TEMPLATE_FILE="$SOURCE_DIR/flatpak/$APP_ID.json.in"
MANIFEST_FILE="$SOURCE_DIR/flatpak/$APP_ID.json"

BRANCH=${BRANCH:-master}
GIT_CLONE_BRANCH=${GIT_CLONE_BRANCH:-HEAD}
RUN_TESTS=${RUN_TESTS:-false}
REPO=${REPO:-repo}

pushd "$SOURCE_DIR"

sed \
  -e "s|@BRANCH@|${BRANCH}|g" \
  -e "s|@GIT_CLONE_BRANCH@|${GIT_CLONE_BRANCH}|g" \
  -e "s|\"@RUN_TESTS@\"|${RUN_TESTS}|g" \
  $MANIFEST_TEMPLATE_FILE \
  > $MANIFEST_FILE

flatpak-builder build --ccache -v $MANIFEST_FILE --keep-build-dirs --repo=${REPO}
flatpak build-bundle ${REPO} "$APP_ID.flatpak" $APP_ID ${BRANCH}

popd
