#ifndef _INCLUDE_BM_CONFIG_H
#define _INCLUDE_BM_CONFIG_H 1

/* include/bm/config.h. Generated automatically at end of configure. */
/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* "Link with gcov." */
/* #undef COVERAGE */

/* Enable debugger */
/* #undef DEBUG_ON */

/* Enable nanomsg event logger */
// #ifndef BM_ELOG_ON
// #define BM_ELOG_ON /**/
// #endif

/* Enable dynamic loading of modules */
#ifndef BM_ENABLE_MODULES
#define BM_ENABLE_MODULES /**/
#endif

/* Define to 1 if you have the <algorithm> header file. */
#ifndef BM_HAVE_ALGORITHM
#define BM_HAVE_ALGORITHM 1
#endif

/* Define to 1 if you have the <array> header file. */
#ifndef BM_HAVE_ARRAY
#define BM_HAVE_ARRAY 1
#endif

/* Define to 1 if you have the <cassert> header file. */
#ifndef BM_HAVE_CASSERT
#define BM_HAVE_CASSERT 1
#endif

/* Define to 1 if you have the <cmath> header file. */
#ifndef BM_HAVE_CMATH
#define BM_HAVE_CMATH 1
#endif

/* Define to 1 if you have the <cstdio> header file. */
#ifndef BM_HAVE_CSTDIO
#define BM_HAVE_CSTDIO 1
#endif

/* Define to 1 if you have the <ctime> header file. */
#ifndef BM_HAVE_CTIME
#define BM_HAVE_CTIME 1
#endif

/* define if the compiler supports basic C++11 syntax */
#ifndef BM_HAVE_CXX11
#define BM_HAVE_CXX11 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#ifndef BM_HAVE_DLFCN_H
#define BM_HAVE_DLFCN_H 1
#endif

/* Found dlopen */
#ifndef BM_HAVE_DLOPEN
#define BM_HAVE_DLOPEN /**/
#endif

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef BM_HAVE_INTTYPES_H
#define BM_HAVE_INTTYPES_H 1
#endif

/* Define to 1 if you have the `atomic' library (-latomic). */
/* #undef HAVE_LIBATOMIC */

/* Define to 1 if you have the `boost_thread' library (-lboost_thread). */
#ifndef BM_HAVE_LIBBOOST_THREAD
#define BM_HAVE_LIBBOOST_THREAD 1
#endif

/* Define to 1 if you have the `gcov' library (-lgcov). */
/* #undef HAVE_LIBGCOV */

/* Define to 1 if you have the `gmp' library (-lgmp). */
#ifndef BM_HAVE_LIBGMP
#define BM_HAVE_LIBGMP 1
#endif

/* Define to 1 if you have the `nanomsg' library (-lnanomsg).
- Mingyu NO nanomsg */
#ifndef BM_HAVE_LIBNANOMSG
#define BM_HAVE_LIBNANOMSG 0
#endif

/* Define to 1 if you have the `pcap' library (-lpcap). */
#ifndef BM_HAVE_LIBPCAP
#define BM_HAVE_LIBPCAP 1
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef BM_HAVE_MEMORY_H
#define BM_HAVE_MEMORY_H 1
#endif

/* Define to 1 if you have the <PI/p4info.h> header file. */
/* #undef HAVE_PI_P4INFO_H */

/* Define to 1 if you have the <PI/pi.h> header file. */
/* #undef HAVE_PI_PI_H */

/* Define to 1 if you have the <PI/target/pi_imp.h> header file. */
/* #undef HAVE_PI_TARGET_PI_IMP_H */

/* Define if you have POSIX threads libraries and header files. */
#ifndef BM_HAVE_PTHREAD
#define BM_HAVE_PTHREAD 1
#endif

/* Have PTHREAD_PRIO_INHERIT. */
#ifndef BM_HAVE_PTHREAD_PRIO_INHERIT
#define BM_HAVE_PTHREAD_PRIO_INHERIT 1
#endif

/* Define to 1 if you have the <queue> header file. */
#ifndef BM_HAVE_QUEUE
#define BM_HAVE_QUEUE 1
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef BM_HAVE_STDINT_H
#define BM_HAVE_STDINT_H 1
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef BM_HAVE_STDLIB_H
#define BM_HAVE_STDLIB_H 1
#endif

/* Define to 1 if you have the <string> header file. */
#ifndef BM_HAVE_STRING
#define BM_HAVE_STRING 1
#endif

/* Define to 1 if you have the <strings.h> header file. */
#ifndef BM_HAVE_STRINGS_H
#define BM_HAVE_STRINGS_H 1
#endif

/* Define to 1 if you have the <string.h> header file. */
#ifndef BM_HAVE_STRING_H
#define BM_HAVE_STRING_H 1
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
#ifndef BM_HAVE_SYS_STAT_H
#define BM_HAVE_SYS_STAT_H 1
#endif

/* Define to 1 if you have the <sys/types.h> header file. */
#ifndef BM_HAVE_SYS_TYPES_H
#define BM_HAVE_SYS_TYPES_H 1
#endif

/* Found Thrift stdcxx wrapper */
/* #undef HAVE_THRIFT_STDCXX_H */

/* Define to 1 if you have the <tuple> header file. */
#ifndef BM_HAVE_TUPLE
#define BM_HAVE_TUPLE 1
#endif

/* Define to 1 if you have the <unistd.h> header file. */
#ifndef BM_HAVE_UNISTD_H
#define BM_HAVE_UNISTD_H 1
#endif

/* Define to 1 if you have the <unordered_map> header file. */
#ifndef BM_HAVE_UNORDERED_MAP
#define BM_HAVE_UNORDERED_MAP 1
#endif

/* Define to 1 if you have the <utility> header file. */
#ifndef BM_HAVE_UTILITY
#define BM_HAVE_UTILITY 1
#endif

/* Define to 1 if you have the <vector> header file. */
#ifndef BM_HAVE_VECTOR
#define BM_HAVE_VECTOR 1
#endif

/* Enable compile-time macro for debug logging */
// #ifndef BM_LOG_DEBUG_ON
// #define BM_LOG_DEBUG_ON /**/
// #endif

/* Enable compile-time macro for trace logging */
// #ifndef BM_LOG_TRACE_ON
// #define BM_LOG_TRACE_ON /**/
// #endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#ifndef BM_LT_OBJDIR
#define BM_LT_OBJDIR ".libs/"
#endif

/* Enable Nanomsg support */
// #ifndef BM_NANOMSG_ON
// #define BM_NANOMSG_ON /**/
// #endif

/* Name of package */
#ifndef BM_PACKAGE
#define BM_PACKAGE "bm"
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef BM_PACKAGE_BUGREPORT
#define BM_PACKAGE_BUGREPORT "mingyu.ma@tu-dresden.de"
#endif

/* Define to the full name of this package. */
#ifndef BM_PACKAGE_NAME
#define BM_PACKAGE_NAME "bm"
#endif

/* Define to the full name and version of this package. */
#ifndef BM_PACKAGE_STRING
#define BM_PACKAGE_STRING "bm 1.15.0-ba36c264"
#endif

/* Define to the one symbol short name of this package. */
#ifndef BM_PACKAGE_TARNAME
#define BM_PACKAGE_TARNAME "bm"
#endif

/* Define to the home page for this package. */
#ifndef BM_PACKAGE_URL
#define BM_PACKAGE_URL ""
#endif

/* Define to the version of this package. */
#ifndef BM_PACKAGE_VERSION
#define BM_PACKAGE_VERSION "1.15.0-ba36c264"
#endif

/* Define to necessary symbol if this constant uses a non-standard name on
   your system. */
/* #undef PTHREAD_CREATE_JOINABLE */

/* Define to 1 if you have the ANSI C header files. */
#ifndef BM_STDC_HEADERS
#define BM_STDC_HEADERS 1
#endif

/* Enable Thrift support */
#ifndef BM_THRIFT_ON
#define BM_THRIFT_ON /**/
#endif

/* Thrift version string extracted from thrift/config.h */
#ifndef BM_THRIFT_VERSION
#define BM_THRIFT_VERSION 1300
#endif

/* Version number of package */
#ifndef BM_VERSION
#define BM_VERSION "1.15.0-ba36c264"
#endif

/* Implement stacks as per P4_16 spec */
#ifndef BM_WP4_16_STACKS
#define BM_WP4_16_STACKS /**/
#endif

/* Define for Solaris 2.5.1 so the uint64_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT64_T */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 64 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint64_t */

/* once: _INCLUDE_BM_CONFIG_H */
#endif

