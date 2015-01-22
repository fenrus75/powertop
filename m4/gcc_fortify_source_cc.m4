dnl GCC_FORTIFY_SOURCE_CC
dnl checks -D_FORTIFY_SOURCE with the C++ compiler, if it exists then
dnl updates CXXCPP
AC_DEFUN([GCC_FORTIFY_SOURCE_CC],[
  AC_LANG_ASSERT([C++])
  AS_IF([test "X$CXX" != "X"], [
    AC_MSG_CHECKING([for FORTIFY_SOURCE support])
    fs_old_cxxcpp="$CXXCPP"
    fs_old_cxxflags="$CXXFLAGS"
    CXXCPP="$CXXCPP -D_FORTIFY_SOURCE=2"
    CXXFLAGS="$CXXFLAGS -Werror"
    AC_COMPILE_IFELSE([
      AC_LANG_PROGRAM([[]], [[
        int main(void) {
        #if !(__GNUC_PREREQ (4, 1) )
        #error No FORTIFY_SOURCE support
        #endif
          return 0;
        }
      ]], [
        AC_MSG_RESULT([yes])
      ], [
        AC_MSG_RESULT([no])
        CXXCPP="$fs_old_cxxcpp"
      ])
    ])
    CXXFLAGS="$fs_old_cxxflags"
  ])
])
