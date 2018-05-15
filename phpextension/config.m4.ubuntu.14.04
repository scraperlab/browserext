PHP_ARG_ENABLE(browserext, Enable browserext support)

if test "$PHP_BROWSEREXT" = "yes"; then
   PHP_REQUIRE_CXX()
   
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/mkspecs/linux-g++-64)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtWebKitWidgets)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtWebKit)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtWidgets)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtGui)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtNetwork)
   PHP_ADD_INCLUDE(/usr/include/x86_64-linux-gnu/qt5/QtCore)
   PHP_ADD_INCLUDE([$srcdir/../browserext-static])

   PHP_SUBST(BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(stdc++, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5Gui, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5Core, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5Network, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5WebKit, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5WebKitWidgets, 1, BROWSEREXT_SHARED_LIBADD)
   PHP_ADD_LIBRARY(Qt5Widgets, 1, BROWSEREXT_SHARED_LIBADD)


   if test $PHP_DEBUG -eq 1; then
	PHPBROWSER_LIBDIR="$srcdir/../browserext-static"
   else
	PHPBROWSER_LIBDIR="$srcdir/../browserext-static"
   fi
   PHP_ADD_LIBRARY_WITH_PATH(phpbrowser, $PHPBROWSER_LIBDIR, BROWSEREXT_SHARED_LIBADD)

   MYCXXFLAGS="-w"
   if test $PHP_THREAD_SAFETY = "yes"; then
	MYCXXFLAGS="$MYCXXFLAGS -DZTS"
   fi
   PHP_NEW_EXTENSION(browserext, browserext.cpp, $ext_shared, , $MYCXXFLAGS)
fi