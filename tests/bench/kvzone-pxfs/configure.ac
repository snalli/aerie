AC_INIT([KVZone], [0.9], [kvzone@nec-labs.com], [kvzone], [http://www.nec-labs.com])
AC_PREREQ([2.59])
AC_LANG([C++])

AM_INIT_AUTOMAKE([1.9 -Wall no-define foreign])
AC_CHECK_HEADERS([assert.h errno.h execinfo.h sched.h signal.h stdarg.h stdint.h stdio.h syscall.h sys/ioctl.h sys/sysinfo.h sys/time.h time.h unistd.h wchar.h string.h])
AC_CHECK_HEADERS([cstdlib cstring deque exception functional iomanip iosfwd iostream list map ostream queue set sstream string utility vector typeinfo tr1/unordered_set])

AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

save_cxxflags="$CXXFLAGS"
CXXFLAGS=-m64
AC_MSG_CHECKING([whether CC supports -m64])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([])],
    [AC_MSG_RESULT([yes])]
    [AM_CXXFLAGS="-m64 -DARCH64"],
    [AC_MSG_RESULT([no])]
)
CXXFLAGS="$save_cxxflags"
AC_SUBST([AM_CXXFLAGS])

AC_SEARCH_LIBS([clock_gettime],[rt],,AC_MSG_ERROR([clock_gettime is not available])) 

AX_BOOST_BASE([1.32])
AX_BOOST_DATE_TIME
AX_BOOST_FILESYSTEM
AX_BOOST_THREAD
AX_BOOST_SYSTEM
AC_CHECK_HEADERS([boost/bind.hpp boost/date_time/gregorian/gregorian_types.hpp boost/date_time/local_time/local_time.hpp boost/date_time/posix_time/posix_time_duration.hpp boost/date_time/posix_time/posix_time.hpp boost/date_time/posix_time/posix_time_types.hpp boost/filesystem/operations.hpp boost/filesystem/path.hpp boost/function.hpp boost/lexical_cast.hpp boost/none.hpp boost/optional.hpp boost/ptr_container/ptr_list.hpp boost/ptr_container/ptr_map.hpp boost/random.hpp boost/random/lagged_fibonacci.hpp boost/scoped_array.hpp boost/scoped_ptr.hpp boost/shared_ptr.hpp boost/thread/condition.hpp boost/thread/detail/lock.hpp boost/thread.hpp boost/thread/mutex.hpp boost/thread/thread.hpp boost/thread/xtime.hpp boost/version.hpp boost/limits.hpp])

PKG_CHECK_MODULES([TOKYOCABINET], [tokyocabinet >= 1.4])
PKG_CHECK_MODULES([SQLITE], [sqlite3 >= 3.3])

AX_BERKELEY_DB([4.3])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
