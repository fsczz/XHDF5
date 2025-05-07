/* src/rest_vol_config.h.  Generated from rest_vol_config.h.in by configure.  */
/* src/rest_vol_config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef RV_AC_APPLE_UNIVERSAL_BUILD */

/* Define to have the REST VOL print out debugging information. */
/* #undef RV_CONNECTOR_DEBUG */

/* Define to have cURL output debugging information. */
/* #undef RV_CURL_DEBUG */

/* Define to 1 if you have the <dirent.h> header file. */
#define RV_HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define RV_HAVE_DLFCN_H 1

/* Define to 1 if you have the <features.h> header file. */
#define RV_HAVE_FEATURES_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define RV_HAVE_INTTYPES_H 1

/* Define to 1 if you have the `dl' library (-ldl). */
#define RV_HAVE_LIBDL 1

/* Define to 1 if you have the `m' library (-lm). */
#define RV_HAVE_LIBM 1

/* Define to 1 if you have the <mach/mach_time.h> header file. */
/* #undef RV_HAVE_MACH_MACH_TIME_H */

/* Define to 1 if you have the <memory.h> header file. */
#define RV_HAVE_MEMORY_H 1

/* Define to 1 if you have the <setjmp.h> header file. */
#define RV_HAVE_SETJMP_H 1

/* Define to 1 if you have the `snprintf' function. */
#define RV_HAVE_SNPRINTF 1

/* Define to 1 if you have the <stdbool.h> header file. */
#define RV_HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stddef.h> header file. */
#define RV_HAVE_STDDEF_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define RV_HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define RV_HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define RV_HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define RV_HAVE_STRING_H 1

/* Define to 1 if you have the <sys/file.h> header file. */
#define RV_HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define RV_HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define RV_HAVE_SYS_RESOURCE_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define RV_HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define RV_HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define RV_HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define RV_HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define RV_HAVE_UNISTD_H 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define RV_LT_OBJDIR ".libs/"

/* Name of package */
#define RV_PACKAGE "rest-vol"

/* Define to the address where bug reports for this package should be sent. */
#define RV_PACKAGE_BUGREPORT "help@hdfgroup.org"

/* Define to the full name of this package. */
#define RV_PACKAGE_NAME "REST VOL"

/* Define to the full name and version of this package. */
#define RV_PACKAGE_STRING "REST VOL 1.0.0"

/* Define to the one symbol short name of this package. */
#define RV_PACKAGE_TARNAME "rest-vol"

/* Define to the home page for this package. */
#define RV_PACKAGE_URL ""

/* Define to the version of this package. */
#define RV_PACKAGE_VERSION "1.0.0"

/* The size of `bool', as computed by sizeof. */
#define RV_SIZEOF_BOOL 1

/* The size of `off_t', as computed by sizeof. */
#define RV_SIZEOF_OFF_T 8

/* Define to 1 if you have the ANSI C header files. */
#define RV_STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define RV_TIME_WITH_SYS_TIME 1

/* Define to have the REST VOL track memory usage. */
/* #undef RV_TRACK_MEM_USAGE */

/* Version number of package */
#define RV_VERSION "1.0.0"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef RV_const */

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef RV_off_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef RV_ptrdiff_t */

/* Define to `unsigned long' if <sys/types.h> does not define. */
/* #undef RV_size_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef RV_ssize_t */
