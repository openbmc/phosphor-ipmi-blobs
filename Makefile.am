AM_DEFAULT_SOURCE_EXT = .cpp

libblobcmdsdir = ${libdir}/ipmid-providers
libblobcmds_LTLIBRARIES = libblobcmds.la
libblobcmds_la_SOURCES = main.cpp \
			 ipmi.cpp \
			 manager.cpp \
			 process.cpp \
			 crc.cpp \
			 utils.cpp \
			 internal.cpp

libblobcmds_la_LDFLAGS = $(SYSTEMD_LIBS) \
			 $(PHOSPHOR_LOGGING_LIBS) \
                         -version-info 0:0:0 -shared

libblobcmds_la_CXXFLAGS = $(SYSTEMD_CFLAGS) \
			  $(PHOSPHOR_LOGGING_CFLAGS)

nobase_include_HEADERS = \
	blobs-ipmid/blobs.hpp \
	blobs-ipmid/manager.hpp

# Always build but only installed if you add the item:
# BLOBIPMI_PROVIDER_LIBRARY += "libexample.so"
libexampledir = ${libdir}/blob-providers
libexample_LTLIBRARIES = libexample.la
libexample_la_SOURCES = example/example.cpp
libexample_la_LDFLAGS = $(PHOSPHOR_LOGGING_LIBS) \
			-version-info 0:0:0 -shared
libexample_la_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS) \
			 -flto

SUBDIRS = . test
