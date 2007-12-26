dnl AC_C_RESTRICT
dnl Do nothing if the compiler accepts the restrict keyword.
dnl Otherwise define restrict to __restrict__ or __restrict if one of
dnl those work, otherwise define restrict to be empty.
AC_DEFUN([AC_C_RESTRICT],
    [AC_MSG_CHECKING([for restrict])
    ac_cv_c_restrict=no
    for ac_kw in restrict __restrict__ __restrict; do
	AC_TRY_COMPILE([],[char * $ac_kw p;],[ac_cv_c_restrict=$ac_kw; break])
    done
    AC_MSG_RESULT([$ac_cv_c_restrict])
    case $ac_cv_c_restrict in
	restrict) ;;
	no)	AC_DEFINE([restrict],,
		    [Define as `__restrict' if that's what the C compiler calls
		    it, or to nothing if it is not supported.]) ;;
	*)	AC_DEFINE_UNQUOTED([restrict],$ac_cv_c_restrict) ;;
    esac])

dnl AC_C_BUILTIN_EXPECT
dnl Check whether compiler understands __builtin_expect.
AC_DEFUN([AC_C_BUILTIN_EXPECT],
    [AC_CACHE_CHECK([for __builtin_expect],[ac_cv_builtin_expect],
	[cat > conftest.c <<EOF
#line __oline__ "configure"
int foo (int a)
{
    a = __builtin_expect (a, 10);
    return a == 10 ? 0 : 1;
}
EOF
	if AC_TRY_COMMAND([${CC-cc} $CFLAGS -nostdlib -nostartfiles
            -o conftest conftest.c -lgcc >&AC_FD_CC]); then
	    ac_cv_builtin_expect=yes
	else
	    ac_cv_builtin_expect=no
	fi
	rm -f conftest*])
    if test x"$ac_cv_builtin_expect" = x"yes"; then
	AC_DEFINE(HAVE_BUILTIN_EXPECT,,
	    [Define if you have the `__builtin_expect' function.])
    fi])

dnl AC_C_ALWAYS_INLINE
dnl Define inline to something appropriate, including the new always_inline
dnl attribute from gcc 3.1
AC_DEFUN([AC_C_ALWAYS_INLINE],
    [AC_C_INLINE
    if test x"$GCC" = x"yes" -a x"$ac_cv_c_inline" = x"inline"; then
	AC_MSG_CHECKING([for always_inline])
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -Wall -Werror"
	AC_TRY_COMPILE([],[__attribute__ ((__always_inline__)) void f (void);],
	    [ac_cv_always_inline=yes],[ac_cv_always_inline=no])
	CFLAGS="$SAVE_CFLAGS"
	AC_MSG_RESULT([$ac_cv_always_inline])
	if test x"$ac_cv_always_inline" = x"yes"; then
	    AC_DEFINE_UNQUOTED([inline],[__attribute__ ((__always_inline__))])
	fi
    fi])

dnl AC_C_ATTRIBUTE_ALIGNED
dnl define ATTRIBUTE_ALIGNED_MAX to the maximum alignment if this is supported
AC_DEFUN([AC_C_ATTRIBUTE_ALIGNED],
    [AC_CACHE_CHECK([__attribute__ ((aligned ())) support],
	[ac_cv_c_attribute_aligned],
	[ac_cv_c_attribute_aligned=0
	for ac_cv_c_attr_align_try in 2 4 8 16 32 64; do
	    AC_TRY_COMPILE([],
		[static char c __attribute__ ((aligned($ac_cv_c_attr_align_try))) = 0; return c;],
		[ac_cv_c_attribute_aligned=$ac_cv_c_attr_align_try])
	done])
    if test x"$ac_cv_c_attribute_aligned" != x"0"; then
	AC_DEFINE_UNQUOTED([ATTRIBUTE_ALIGNED_MAX],
	    [$ac_cv_c_attribute_aligned],[maximum supported data alignment])
    fi])

dnl AC_TRY_CFLAGS (CFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check if $CC supports a given set of cflags
AC_DEFUN([AC_TRY_CFLAGS],
    [AC_MSG_CHECKING([if $CC supports $1 flags])
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$1"
    AC_TRY_COMPILE([],[],[ac_cv_try_cflags_ok=yes],[ac_cv_try_cflags_ok=no])
    CFLAGS="$SAVE_CFLAGS"
    AC_MSG_RESULT([$ac_cv_try_cflags_ok])
    if test x"$ac_cv_try_cflags_ok" = x"yes"; then
	ifelse([$2],[],[:],[$2])
    else
	ifelse([$3],[],[:],[$3])
    fi])

dnl AC_LIBTOOL_NON_PIC ([ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check for nonbuggy libtool -prefer-non-pic
AC_DEFUN([AC_LIBTOOL_NON_PIC],
    [AC_MSG_CHECKING([if libtool supports -prefer-non-pic flag])
    mkdir ac_test_libtool; cd ac_test_libtool; ac_cv_libtool_non_pic=no
    echo "int g (int i); int f (int i) {return g (i);}" >f.c
    echo "int (* hook) (int) = 0; int g (int i) {if (hook) i = hook (i); return i + 1;}" >g.c
    ../libtool --mode=compile $CC $CFLAGS -prefer-non-pic \
		-c f.c >/dev/null 2>&1 && \
	../libtool --mode=compile $CC $CFLAGS -prefer-non-pic \
		-c g.c >/dev/null 2>&1 && \
	../libtool --mode=link $CC $CFLAGS -prefer-non-pic -o libfoo.la \
		-rpath / f.lo g.lo >/dev/null 2>&1 &&
	ac_cv_libtool_non_pic=yes
    cd ..; rm -fr ac_test_libtool; AC_MSG_RESULT([$ac_cv_libtool_non_pic])
    if test x"$ac_cv_libtool_non_pic" = x"yes"; then
	ifelse([$1],[],[:],[$1])
    else
	ifelse([$2],[],[:],[$2])
    fi])

dnl AC_CHECK_GENERATE_INTTYPES_H (INCLUDE-DIRECTORY)
dnl generate a default inttypes.h if the header file does not exist already
AC_DEFUN([AC_CHECK_GENERATE_INTTYPES],
    [rm -f $1/inttypes.h
    AC_CHECK_HEADER([inttypes.h],[],
	[AC_CHECK_SIZEOF([char])
	AC_CHECK_SIZEOF([short])
	AC_CHECK_SIZEOF([int])
	if test x"$ac_cv_sizeof_char" != x"1" -o \
	    x"$ac_cv_sizeof_short" != x"2" -o \
	    x"$ac_cv_sizeof_int" != x"4"; then
	    AC_MSG_ERROR([can not build a default inttypes.h])
	fi
	cat >$1/inttypes.h << EOF
/* default inttypes.h for people who do not have it on their system */

#ifndef _INTTYPES_H
#define _INTTYPES_H
#if (!defined __int8_t_defined) && (!defined __BIT_TYPES_DEFINED__)
#define __int8_t_defined
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
#ifdef ARCH_X86
typedef signed long long int64_t;
#endif
#endif
#if (!defined _LINUX_TYPES_H)
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#ifdef ARCH_X86
typedef unsigned long long uint64_t;
#endif
#endif
#endif
EOF
	])])
