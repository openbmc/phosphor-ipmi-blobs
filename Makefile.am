AM_DEFAULT_SOURCE_EXT = .cpp

libblobcmdsdir = ${libdir}/ipmid-providers
libblobcmds_LTLIBRARIES = libblobcmds.la
libblobcmds_la_SOURCES = \
	main.cpp \
	ipmi.cpp \
	manager.cpp \
	process.cpp \
	crc.cpp \
	utils.cpp

libblobcmds_la_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(LIBADD_DLOPEN) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs \
	-version-info 0:0:0 -shared

libblobcmds_la_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

nobase_include_HEADERS = \
	blobs-ipmid/blobs.hpp \
	blobs-ipmid/manager.hpp \
	blobs-ipmid/test/blob_mock.hpp \
	blobs-ipmid/test/manager_mock.hpp

# Install the blob handlers in ipmid-providers so you can leverage
# meta-phosphor/blob/master/classes/obmc-phosphor-ipmiprovider-symlink.bbclass
# Which will create a symlink in ${D}/${libdir}/blob-ipmid/ pointing to the
# library in ${D}/${libdir}/ipmid-providers/

# Always build but only installed if you add the item:
# BLOBIPMI_PROVIDER_LIBRARY += "libexample.so"
libblobexampledir = ${libdir}/ipmid-providers
libblobexample_LTLIBRARIES = libblobexample.la
libblobexample_la_SOURCES = example/example.cpp
libblobexample_la_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) \
	-version-info 0:0:0 -shared
libblobexample_la_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	-flto

SUBDIRS = . test
