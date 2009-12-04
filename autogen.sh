#! /bin/sh

libtoolize --automake
aclocal
autoconf
autoheader
automake -a --add-missing
./configure $@
exit
