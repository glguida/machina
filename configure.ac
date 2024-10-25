AC_INIT([MACHINA],[0.0])

AC_CONFIG_MACRO_DIRS([nux/m4])
AC_CONFIG_SUBDIRS(nux)
nux_dir=nux
libec_dir=nux/libec


AC_MK_DIR([${nux_dir}/mk])
AC_NUXROOT_DIR([${nux_dir}])
AC_LIBEC_DIR([${libec_dir}])

AC_LIB_DIR(libmachina,libmachina)

AC_ARG_VAR([ARCH], [The architecture to compile for: i386, amd64 or riscv64])

if test x"${ARCH}" == x ; then
   AC_MSG_ERROR([Please select a valid architecture with `./configure ARCH=<arch>`])
fi

AS_CASE([${ARCH}],
	[i386], [tool_prefix=i686-unknown-elf],
	[amd64], [tool_prefix=amd64-unknown-elf],
	[riscv64], [tool_prefix=riscv64-unknown-elf],
	[*], [AC_MSG_ERROR([Supported Architectures: i386, amd64, riscv64])])

AC_ARG_VAR([TOOLCHAIN],[toolchain prefix @<:@default: <ARCH>-elf@:>@])
if test x"${TOOLCHAIN}" != x ; then
   tool_prefix="${TOOLCHAIN}"
fi

AC_PROG_CC
AC_PROG_YACC
AC_PROG_LEX([])
# This is used by the mig script. Behaviour of CPP and GCC -E are
# different, so let's stick to a standard one.
AC_SUBST(CPP, "${CC} -E")

AC_CHECK_TOOL(TARGET_CC, ${tool_prefix}-gcc, :)
if test x"${TARGET_CC}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-gcc not found])
fi
AC_CHECK_TOOL(TARGET_LD, ${tool_prefix}-ld, :)
if test x"${TARGET_LD}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-ld not found])
fi
AC_CHECK_TOOL(TARGET_AR, ${tool_prefix}-ar, :)
if test x"${TARGET_AR}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-ar not found])
fi
AC_CHECK_TOOL(TARGET_OBJCOPY, ${tool_prefix}-objcopy, :)
if test x"${TARGET_OBJCOPY}" = 'x:' ; then
  AC_MSG_ERROR([${tool_prefix}-objcopy not found])
fi

machine=${ARCH}

AC_ARG_ENABLE([opt],
	AS_HELP_STRING([--disable-opt],[compile without compiler optimizations]))
AS_IF([test "x$enable_opt" != "xno"],
	[optflags='-O7 -g'],
	[optflags='-O0 -g'])

AC_ARG_ENABLE([werror],
	AS_HELP_STRING([--disable-werror],[compile ignoring compiler warnings]))
AS_IF([test "x$enable_relax" != "xno"],
	[wflags='-Werror'],
	[wflags=''])

AC_SUBST(MACHINE, ${machine})
AC_SUBST(CONFIGURE_FLAGS, ["${optflags} ${wflags} -DMCN_MACHINE=${machine} -I\$(SRCROOT)/include -I\$(BUILDROOT)/include -I\$(SRCROOT)/${nux_dir}/include -I\$(BUILDROOT)/${nux_dir}/include"])
AC_SUBST(APXH_SUBDIRS, [${apxh_subdirs}])

AC_CONFIG_FILES([tools/mig/mig:tools/mig/mig.in], [chmod +x tools/mig/mig])
AC_SUBST(MIG, '$(BUILDROOT)/tools/mig/mig')
AC_SUBST(MIGCOM, '$(BUILDROOT)/tools/mig/migcom')

AC_MK_CONFIG_FILES([
	Makefile
	libmachina/Makefile
	kern/Makefile
	tests/Makefile
	tests/libkstest/Makefile
	tests/libkstest/user/Makefile
	tests/libkstest/kern/Makefile
	tests/test/Makefile
	bootstrap/Makefile
	tools/mig/Makefile
])

AC_OUTPUT
