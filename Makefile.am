AM_DEFAULT_SOURCE_EXT = .cpp

libblobcmdsdir = ${libdir}/ipmid-providers
libblobcmds_LTLIBRARIES = libblobcmds.la
libblobcmds_la_SOURCES = main.cpp \
			 ipmi.cpp \
			 manager.cpp \
			 process.cpp \
			 crc.cpp

libblobcmds_la_LDFLAGS = $(SYSTEMD_LIBS) \
                         -version-info 0:0:0 -shared

libblobcmds_la_CXXFLAGS = $(SYSTEMD_CFLAGS)

SUBDIRS = . test
