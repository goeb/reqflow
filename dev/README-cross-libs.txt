

# Platform for building Windows release from my Linux host
(Frederic Hoerni, 22 Nov 2016)

This README shortly explains how I have setup on my platform 
the cross-compiled libraries needed for Reqflow.

My development platform is a Linux Mint 32 bit host.

See also the script win32env in the same directory, that refers to these 
libraries for building Reqflow.

## Linux packages

	gcc-mingw-w64-i686
	win-iconv-mingw-w64-dev

## libpcre

LIBPCRE=$HOME/Downloads/pcre-8.34/.libs

Built with:

    $ ./configure --host=i586-mingw32msvc CFLAGS=-DPCRE_STATIC --enable-shared=no


## libxml2

LIBXML=$HOME/Downloads/libxml2-2.9.1

Built with:

    $ ./configure --host=i586-mingw32msvc --without-http --without-python --without-ftp


## libzip

LIBZIP=$HOME/Downloads/libzip-0.11.2

Built with:

    $ ./configure --host=i586-mingw32msvc LIBS=$HOME/Downloads/zlib-1.2.8/libz.a --with-zlib=$HOME/Downloads/zlib-1.2.8


## zlib

LIBZ=$HOME/Downloads/zlib-1.2.8


## poppler
LIBPOPPLER=$HOME/win32libs/poppler-0.24.5

Built with:

	./configure --host=i686-w64-mingw32 --prefix=/home/fred/win32libs/poppler-0.48.0 --disable-libopenjpeg --disable-libtiff --disable-libcurl --disable-libjpeg --disable-libpng --disable-splash-output --disable-cairo-output --disable-poppler-glib --disable-gtk-doc-html --disable-poppler-qt4 --disable-poppler-qt5 --disable-gtk-test --without-x FREETYPE_CFLAGS=-I/home/fred/win32libs/freetype-2.5.2/include FREETYPE_LIBS=-L/home/fred/win32libs/freetype-2.5.2/lib -lfreetype


## Inno Setup

Inno Setup installed on my Linux host via wine.


