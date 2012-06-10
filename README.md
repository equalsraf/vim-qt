This is README for the Qt GUI - if you want the VIM README check README.txt

# Compiling VIM with the Qt gui

## Building on Linux/BSD systems

For the most part you can build it as you would build vim with support for another gui, just issue the following commands:

    $ ./configure --enable-gui=qt
    $ make  
    $ sudo make install

All the regular configure options apply. Just don’t forget, you need libqt and libqt4-devel now. Usually you'll want to pass in something like:

    ./configure --prefix=/usr/ --with-features=huge --enable-gui=qt


If configure is unable to find Qt, try passing in the Qt base dir as follows:

    ./configure --enable-gui=qt --with-qt-dir=/usr/lib/qt4


## Building on Windows

As of commit c0956732b437 we have some initial support to build vim-Qt in windows. You can build vim-Qt for windows provided that you have Qt, CMake and an adequate compiler(Mingw or Visual Studio).

Naturally you need to have Qt installed. The best way to do this depends on your setup - check the Qt website for more details and keep in mind that you need to match your Qt library version with your compiler.


### Visual Studio 2010

From the VS console:

    $ cmake -G NMake Makefiles PATH_TO_QTVIM\src
    $ nmake

This will generate a binary, qvim.exe. Keep the runtime folder in the same folder as qvim.exe, otherwise vim will not have access to menus and configuration files.


### MinGW

Make sure all mingw tools are in your path (i.e. gcc, mingw32-make), and then from your console:

    $ cmake -G MinGW Makefiles PATH_TO_QTVIM\src
    $ mingw32-make



