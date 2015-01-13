/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Date of compilation */
#define BUILD_DATE "Tue 13 Jan 2015 14:29:35"

/* Build OS Particulars */
#define BUILD_OS_NAME "Linux 3.17.7-300.fc21.x86_64 x86_64"

/* Compiler Version */
#define COMPILER_VERSION "gcc version 4.9.2 20141101 (Red Hat 4.9.2-1) (GCC) "

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `backtrace' function. */
#define HAVE_BACKTRACE 1

/* Define to 1 if you have the `backtrace_symbols' function. */
#define HAVE_BACKTRACE_SYMBOLS 1

/* Define if daemon() is available */
#define HAVE_DAEMON 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
#define HAVE_EXECINFO_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `g15' library (-lg15). */
#define HAVE_LIBG15 1

/* Define to 1 if you have the `g15render' library (-lg15render). */
#define HAVE_LIBG15RENDER 1

/* Define to 1 if you have the <libg15.h> header file. */
#define HAVE_LIBG15_H 1

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `pthread' library (-lpthread). */
#define HAVE_LIBPTHREAD 1

/* Define to 1 if you have the <linux/input.h> header file. */
#define HAVE_LINUX_INPUT_H 1

/* Define to 1 if you have the <linux/uinput.h> header file. */
#define HAVE_LINUX_UINPUT_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the `strftime' function. */
#define HAVE_STRFTIME 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have <sys/wait.h> that is POSIX.1 compatible. */
#define HAVE_SYS_WAIT_H 1

/* Define if struct uinput_user_dev has id member. */
#define HAVE_UINPUT_USER_DEV_ID 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if libusb implementation blocks on read or write */
/* #undef LIBUSB_BLOCKS */

/* Target OS is Darwin */
/* #undef OSTYPE_DARWIN */

/* Target OS is Linux */
#define OSTYPE_LINUX 1

/* Target OS is unknown */
/* #undef OSTYPE_OTHER */

/* Target OS is Solaris */
/* #undef OSTYPE_SOLARIS */

/* Name of package */
#define PACKAGE "g15daemon"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "mlampard@users.sf.net"

/* Define to the full name of this package. */
#define PACKAGE_NAME "g15daemon"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "g15daemon 1.9.5.3"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "g15daemon"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.9.5.3"

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.9.5.3"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */
