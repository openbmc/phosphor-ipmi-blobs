AM_DEFAULT_SOURCE_EXT = .cpp

HANDLERS =

if ENABLE_EXAMPLE
HANDLERS += example/example.cpp
endif

libblobcmdsdir = ${libdir}/ipmid-providers
libblobcmds_LTLIBRARIES = libblobcmds.la
libblobcmds_la_SOURCES = main.cpp \
			 ipmi.cpp \
			 manager.cpp \
			 process.cpp \
			 crc.cpp \
			 utils.cpp \
			 $(HANDLERS)

libblobcmds_la_LDFLAGS = $(SYSTEMD_LIBS) \
			 $(PHOSPHOR_LOGGING_LIBS) \
                         -version-info 0:0:0 -shared

libblobcmds_la_CXXFLAGS = $(SYSTEMD_CFLAGS) \
			  $(PHOSPHOR_LOGGING_CFLAGS)

nobase_include_HEADERS = \
	blobs-ipmid/blobs.hpp \
	blobs-ipmid/manager.hpp

SUBDIRS = . test
