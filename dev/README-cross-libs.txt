

# Platform for building Windows release from my Linux host
(Frederic Hoerni, 30 Apr 2014)

This README shortly explains how I have setup on my platform 
the cross-compiled libraries needed for Reqflow.

My development platform is a Linux Mint 32 bit host.

See also the script win32env in the same directory, that refers to these 
libraries for building Reqflow.

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


## libiconv

LIBICONV=$HOME/win32libs/libiconv-1.14

Built with:
    $ ./configure --host i586-mingw32msvc --prefix=$HOME/win32libs/libiconv-1.14


## poppler
LIBPOPPLER=$HOME/win32libs/poppler-0.24.5

Built with:

    $ ./configure --host i586-mingw32msvc --prefix=$HOME/win32libs/poppler-0.24.5 --disable-libopenjpeg --disable-libtiff --disable-libcurl --disable-libjpeg --disable-libpng --disable-splash-output --disable-cairo-output --disable-poppler-glib --disable-gtk-doc-html --disable-poppler-qt4 --disable-poppler-qt5 --disable-gtk-test --without-x --with-libiconv-prefix=$HOME/win32libs/libiconv-1.14 CFLAGS=-I$HOME/win32libs/libiconv-1.14/include FREETYPE_CFLAGS=-I$HOME/win32libs/freetype-2.5.2/include FREETYPE_LIBS=-L$HOME/win32libs/freetype-2.5.2/lib -lfreetype CPPFLAGS=-D_WIN32_WINNT=0x0500 -I$HOME/win32libs/libiconv-1.14/include LDFLAGS=$HOME/win32libs/libiconv-1.14/lib/libiconv.a -L$HOME/win32libs/libiconv-1.14/lib -liconv


## Inno Setup

Inno Setup installed on my Linux host via wine.


