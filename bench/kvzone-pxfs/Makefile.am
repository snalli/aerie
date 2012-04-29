AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}
AM_CXXFLAGS = -I$(top_srcdir)/src/include -Wall -Wunused-parameter -Werror -Wno-non-virtual-dtor -pthread -fno-strict-aliasing -O3 -g0 -DNDEBUG @BOOST_CPPFLAGS@ @AM_CXXFLAGS@
LDADD = @BOOST_THREAD_LIB@ @BOOST_DATE_TIME_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_SYSTEM_LIB@

bin_PROGRAMS = kvzone simpleClient
noinst_LTLIBRARIES = libutil.la libkeyValueStore.la libnoopKVAdapter.la libmemMapAdapter.la libtokyoCabinetAdapter.la libberkeleyDBAdapter.la libsqliteAdapter.la

kvzone_SOURCES = src/main/kvzone/kvzone.cpp
kvzone_LDADD = libutil.la libkeyValueStore.la libnoopKVAdapter.la libmemMapAdapter.la libtokyoCabinetAdapter.la libberkeleyDBAdapter.la libsqliteAdapter.la ${LDADD}

simpleClient_SOURCES = src/main/simpleClient/simpleClient.cpp
simpleClient_LDADD = libutil.la libkeyValueStore.la libnoopKVAdapter.la libmemMapAdapter.la libtokyoCabinetAdapter.la libberkeleyDBAdapter.la libsqliteAdapter.la ${LDADD}

libutil_la_SOURCES = src/main/util/boostTime.cpp \
src/main/util/debug.cpp \
src/main/util/exceptions.cpp \
src/main/util/hrFastTime.cpp \
src/main/util/hrFastTimeThread.cpp \
src/main/util/misc.cpp \
src/main/util/string.cpp \
src/main/util/completion.cpp \
src/main/util/tsc.cpp \
src/main/util/tscExtrapolator.cpp

libkeyValueStore_la_SOURCES = src/main/keyValueStore/blockingMultiQueue.cpp \
src/main/keyValueStore/kvStoreQueue.cpp \
src/main/keyValueStore/localKeyValueStore.cpp \
src/main/keyValueStore/syncKeyValueStore.cpp \
src/main/keyValueStore/workersQueue.cpp

libmemMapAdapter_la_SOURCES = src/main/keyValueStore/adapters/memMap/memMap.cpp
libnoopKVAdapter_la_SOURCES = src/main/keyValueStore/adapters/noop/noopAdapter.cpp
libsqliteAdapter_la_SOURCES = src/main/keyValueStore/adapters/sqlite/sqliteAdapter.cpp
libsqliteAdapter_la_LIBADD = libutil.la @SQLITE_LIBS@
libsqliteAdapter_la_CXXFLAGS = @SQLITE_CFLAGS@ $(AM_CXXFLAGS)
libtokyoCabinetAdapter_la_SOURCES = src/main/keyValueStore/adapters/tokyoCabinet/tokyoCabinetAdapter.cpp
libtokyoCabinetAdapter_la_LIBADD = @TOKYOCABINET_LIBS@
libtokyoCabinetAdapter_la_CXXFLAGS = @TOKYOCABINET_CFLAGS@ $(AM_CXXFLAGS)
libberkeleyDBAdapter_la_SOURCES = src/main/keyValueStore/adapters/berkeleyDB/cds/berkeleyDBAdapter.cpp
libberkeleyDBAdapter_la_LIBADD = @DB_LIBS@

dist_noinst_SCRIPTS = autogen.sh
