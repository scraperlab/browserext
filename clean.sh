#!/usr/bin/env bash

ROOTDIR="`pwd`"

cd "$ROOTDIR/browserext-static"
make clean
rm Makefile

cd "$ROOTDIR/browser"
make clean
rm Makefile

cd "$ROOTDIR/phpextension"
phpize --clean
