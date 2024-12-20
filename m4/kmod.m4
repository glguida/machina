AC_DEFUN([AC_KMOD_DIR],
	[
	AC_SUBST(m4_toupper($1)_SRCDIR, '$(SRCROOT)'/$2)
	AC_SUBST(m4_toupper($1)_BUILDDIR, '$(BUILDROOT)'/$2)
	AC_SUBST(KMOD_[]m4_toupper($1), ["include "'$(BUILDROOT)'"/mk/$1-kmod.mk"])
	AC_CONFIG_FILES([mk/$1-kmod.mk:$2/$1-kmod.mk.in])
])
