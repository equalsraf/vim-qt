#
# config.mk.in -- autoconf template for Vim on Unix		vim:ts=8:sw=8:
#
# DO NOT EDIT config.mk!!  It will be overwritten by configure.
# Edit Makefile and run "make" or run ./configure with other arguments.
#
# Configure does not edit the makefile directly. This method is not the
# standard use of GNU autoconf, but it has two advantages:
#   a) The user can override every choice made by configure.
#   b) Modifications to the makefile are not lost when configure is run.
#
# I hope this is worth being nonstandard. jw.



VIMNAME		= vim
EXNAME		= ex
VIEWNAME	= view

CC		= gcc
DEFS		= -DHAVE_CONFIG_H
CFLAGS		= -g -O2 -D_FORTIFY_SOURCE=1
CPPFLAGS	= 
srcdir		= .

LDFLAGS		=  -Wl,-E -Wl,-rpath,/usr/lib/perl5/5.12.3/x86_64-linux-thread-multi/CORE   -L/usr/local/lib
LIBS		= -lm -lelf -lnsl   -lncurses -lacl -lattr -lgpm
TAGPRG		= ctags

CPP		= gcc -E
CPP_MM		= M
DEPEND_CFLAGS_FILTER = | sed 's+-I */+-isystem /+g'
X_CFLAGS	=  
X_LIBS_DIR	=  
X_PRE_LIBS	=  -lSM -lICE -lXpm
X_EXTRA_LIBS	=  -lXdmcp -lSM -lICE
X_LIBS		= -lXt -lX11

LUA_LIBS	= 
LUA_SRC		= 
LUA_OBJ		= 
LUA_CFLAGS	= 
LUA_PRO		= 

MZSCHEME_LIBS	= 
MZSCHEME_SRC	= 
MZSCHEME_OBJ	= 
MZSCHEME_CFLAGS	= 
MZSCHEME_PRO	= 
MZSCHEME_EXTRA  = 
MZSCHEME_MZC	= 

PERL		= /usr/bin/perl
PERLLIB		= /usr/lib/perl5/5.12.3
PERL_LIBS	= -Wl,-E -Wl,-rpath,/usr/lib/perl5/5.12.3/x86_64-linux-thread-multi/CORE  -L/usr/local/lib64 -fstack-protector  -L/usr/lib/perl5/5.12.3/x86_64-linux-thread-multi/CORE -lperl -lm -ldl -lcrypt -lpthread
SHRPENV		= 
PERL_SRC	= auto/if_perl.c if_perlsfio.c
PERL_OBJ	= objects/if_perl.o objects/if_perlsfio.o
PERL_PRO	= if_perl.pro if_perlsfio.pro
PERL_CFLAGS	=  -D_REENTRANT -D_GNU_SOURCE -DPERL_USE_SAFE_PUTENV -DDEBUGGING  -fstack-protector -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64  -I/usr/lib/perl5/5.12.3/x86_64-linux-thread-multi/CORE 

PYTHON_SRC	= if_python.c
PYTHON_OBJ	= objects/if_python.o objects/py_config.o
PYTHON_CFLAGS	= -I/usr/include/python2.7 -pthread
PYTHON_LIBS	= -L/usr/lib64/python2.7/config -lpython2.7 -lpthread -ldl -lutil -lm -Xlinker -export-dynamic
PYTHON_CONFDIR	= /usr/lib64/python2.7/config
PYTHON_GETPATH_CFLAGS = -DPYTHONPATH='":/usr/local/lib/python2.7/site-packages/tweepy-1.7.1-py2.7.egg:/usr/local/lib/python2.7/site-packages/flickrapi-1.4.2-py2.7.egg:/usr/local/lib/python2.7/site-packages/configobj-4.7.2-py2.7.egg:/usr/local/lib/python2.7/site-packages/InkscapeSlide-1.0-py2.7.egg:/usr/local/lib/python2.7/site-packages/pyftpdlib-0.6.0-py2.7.egg:/usr/lib/python27.zip:/usr/lib64/python2.7:/usr/lib64/python2.7/plat-linux2:/usr/lib64/python2.7/lib-tk:/usr/lib64/python2.7/lib-old:/usr/lib64/python2.7/lib-dynload:/usr/lib64/python2.7/site-packages:/usr/lib64/python2.7/site-packages/PIL:/usr/local/lib64/python2.7/site-packages:/usr/local/lib/python2.7/site-packages:/usr/lib64/python2.7/site-packages/gtk-2.0:/usr/lib/python2.7/site-packages:/usr/lib/python2.7/site-packages/setuptools-0.6c11-py2.7.egg-info"' -DPREFIX='"/usr"' -DEXEC_PREFIX='"/usr"'

PYTHON3_SRC	= 
PYTHON3_OBJ	= 
PYTHON3_CFLAGS	= 
PYTHON3_LIBS	= 
PYTHON3_CONFDIR	= 

TCL		= /usr/bin/tclsh8.5
TCL_SRC		= 
TCL_OBJ		= 
TCL_PRO		= 
TCL_CFLAGS	= 
TCL_LIBS	= 

HANGULIN_SRC	= 
HANGULIN_OBJ	= 

WORKSHOP_SRC	= 
WORKSHOP_OBJ	= 

NETBEANS_SRC	= netbeans.c
NETBEANS_OBJ	= objects/netbeans.o

RUBY		= /usr/bin/ruby
RUBY_SRC	= 
RUBY_OBJ	= 
RUBY_PRO	= 
RUBY_CFLAGS	= 
RUBY_LIBS	= 

SNIFF_SRC	= if_sniff.c
SNIFF_OBJ	= objects/if_sniff.o

AWK		= gawk

STRIP		= strip

EXEEXT		= 

COMPILEDBY	= 

INSTALLVIMDIFF	= installvimdiff
INSTALLGVIMDIFF	= installgvimdiff
INSTALL_LANGS	= install-languages
INSTALL_TOOL_LANGS	= install-tool-languages

### sed command to fix quotes while creating pathdef.c
QUOTESED        = sed -e 's/[\\"]/\\&/g' -e 's/\\"/"/' -e 's/\\";$$/";/'

### Line break character as octal number for "tr"
NL		= "\\012"

### Top directory for everything
prefix		= /usr

### Top directory for the binary
exec_prefix	= ${prefix}

### Prefix for location of data files
BINDIR		= ${exec_prefix}/bin

### For autoconf 2.60 and later (avoid a warning)
datarootdir	= ${prefix}/share

### Prefix for location of data files
DATADIR		= ${datarootdir}

### Prefix for location of man pages
MANDIR		= ${datarootdir}/man

### Do we have a GUI
GUI_INC_LOC	= 
GUI_LIB_LOC	= 
GUI_SRC		= $(QT_SRC)
GUI_OBJ		= $(QT_OBJ)
GUI_DEFS	= $(QT_DEFS)
GUI_IPATH	= $(QT_IPATH)
GUI_LIBS_DIR	= $(QT_LIBS_DIR)
GUI_LIBS1	= $(QT_LIBS1)
GUI_LIBS2	= $(QT_LIBS2)
GUI_INSTALL	= $(QT_INSTALL)
GUI_TARGETS	= $(QT_TARGETS)
GUI_MAN_TARGETS	= $(QT_MAN_TARGETS)
GUI_TESTTARGET	= $(QT_TESTTARGET)
GUI_TESTARG	= $(QT_TESTARG)
GUI_BUNDLE	= $(QT_BUNDLE)
NARROW_PROTO	= 
GUI_X_LIBS	= 
MOTIF_LIBNAME	= 
GTK_LIBNAME	= 

### Any OS dependent extra source and object file
OS_EXTRA_SRC	= 
OS_EXTRA_OBJ	= 

### If the *.po files are to be translated to *.mo files.
MAKEMO		= yes

# Make sure that "make first" will run "make all" once configure has done its
# work.  This is needed when using the Makefile in the top directory.
first: all
