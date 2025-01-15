#!/usr/bin/env bash
set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ -z "$SCRIPT_DIR" ]
then
    echo "FAILURE to detect SCRIPT_DIR - aborting"
    exit 1
fi

BUILD_DIR=$SCRIPT_DIR/build_docs
INSTALL_DIR=$SCRIPT_DIR/../docs

rm -rf $BUILD_DIR
rm -rf $INSTALL_DIR

mkdir -p $BUILD_DIR
echo "**" > $BUILD_DIR/.gitignore

cmake -S $SCRIPT_DIR -B $BUILD_DIR -GNinja
cmake --build $BUILD_DIR
cmake --install $BUILD_DIR --prefix $INSTALL_DIR

# add a .nojekyll file to help GH-pages
touch $INSTALL_DIR/.nojekyll

echo "SUCCESS"
