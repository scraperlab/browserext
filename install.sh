#!/usr/bin/env bash

ROOTDIR="`pwd`"

cd "$ROOTDIR/phpextension"
make install
#service apache2 restart
