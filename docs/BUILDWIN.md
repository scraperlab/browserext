Compilation of BrowserExt in Windows
====================================

The compilation process is described for Visual Studio 2008:

1.  Install Qt4 library and set the environment variable QTDIR,
    pointing to a directory with Qt. For example, C:\Qt\4.8.3.

2.  Now it is necessary to compile a static library browserext-static.
    In the directory have a project for Visual Studio. The compiled library
    will be called phpbrowser_a.lib (or phpbrowser_a_debug.lib).

3.  Next to compile php, as described [here](https://wiki.php.net/internals/windows/stepbystepbuild),
    after copying directory phpextension in ext of php source
    and phpbrowser_a.lib in the directory with external libraries for php
    (deps\lib).

