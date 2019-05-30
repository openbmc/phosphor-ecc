bin_PROGRAMS = ecc_main

ecc_main_SOURCES = ecc_main.cpp
ecc_main_LDADD = $(SDBUSPLUS_LIBS) libecc.la
ecc_main_CXXFLAGS = $(SDBUSPLUS_CFLAGS)

noinst_LTLIBRARIES = libecc.la
libecc_la_LDFLAGS = -static
libecc_la_LIBADD = \
	-lstdc++fs \
	$(SDBUSPLUS_LIBS) \
	$(SDEVENTPLUS_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)

libecc_la_CXXFLAGS = \
	$(SDBUSPLUS_CFLAGS) \
	$(SDEVENTPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

libecc_la_SOURCES = \
	ecc_main.cpp \
	ecc_manager.cpp

eccdir = $(sysconfdir)/ecc
ecc_DATA = maxlog.conf