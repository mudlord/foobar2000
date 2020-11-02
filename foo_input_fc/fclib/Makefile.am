EXTRA_DIST = Config.h.in

DISTCLEANFILES = Config.h

fcdecincludedir = $(includedir)
fcdecinclude_HEADERS = fc14audiodecoder.h

lib_LTLIBRARIES = libfc14audiodecoder.la

libfc14audiodecoder_la_LDFLAGS = -version-info @LIBVERSION@

libfc14audiodecoder_la_SOURCES = \
fc14audiodecoder.h \
fc14audiodecoder.cpp \
Config.h \
Dump.h \
Dump.cpp \
FC.cpp \
FC_Data.cpp \
FC.h \
Paula.h \
Paula.cpp \
LamePaula.h \
LamePaulaVoice.cpp \
LamePaulaMixer.cpp \
MyEndian.h \
MyTypes.h \
SmartPtr.h

