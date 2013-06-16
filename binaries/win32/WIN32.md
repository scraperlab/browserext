Installing extension in Windows
=============================== 

In this directory placed binaries for 32-bit version of Windows.
Also, here are the Qt library files, required for extension. Also
extension requires Microsoft Visual C++ 2008 Redistributable
Package (x86), which can be downloaded from Microsoft website.


Installation
------------

1.  Copy php_browserext.dll, the appropriate version of php 
    in the extensions directory. For example, it may be C:\php\ext.

2.  Copy all dll's from Qt directory to С:\windows\system32.

3.  Copy imageformats directory to directory with executable file
    of your web server, for example С:\apache\bin

4.  Download and install Microsoft Visual C++ 2008 Redistributable
    Package (x86).

5.  In the php.ini file, enable the extension by adding the line
    
    `extension=php_browserext.dll`
