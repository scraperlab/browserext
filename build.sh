#!/usr/bin/env bash

ROOTDIR="`pwd`"

cd "$ROOTDIR/browserext-static"
qmake
make

cd "$ROOTDIR/browser"
qmake
make

cd "$ROOTDIR/phpextension"
phpize
./configure
make
#sudo make install
#sudo service apache2 restart
