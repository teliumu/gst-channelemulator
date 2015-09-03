plugin_LTLIBRARIES = libgstchannelemulator.la

# sources used to compile this plug-in
libgstchannelemulator_la_SOURCES = gstchannelemulator.c gstchannelemulator.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstchannelemulator_la_CFLAGS = $(GST_CFLAGS)
libgstchannelemulator_la_LIBADD = $(GST_LIBS)
libgstchannelemulator_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstchannelemulator_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstchannelemulator.h
