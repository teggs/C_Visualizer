#!/bin/sh

if hostname | egrep --quiet '(budgie|dropbear)' # Chris's Linux or macOS
then
    WXCONFIG="/usr/local/src/wxWidgets-3.1.4/wx-config"
else						# Yue's Linux
    WXCONFIG="/usr/bin/wx-config"
fi

set -v
date '+char *COMPILED = (char *)"%r %A %d %B %Y";' > compiled.cpp
c++	`$WXCONFIG --cxxflags` -fvisibility-inlines-hidden \
	-o foryue foryue.cpp gui.cpp compiled.cpp \
	`$WXCONFIG --libs`
rm -f compiled.cpp
ls -l foryue
