PHP_ARG_ENABLE([numerus],
  [whether to enable numerus support],
  [AS_HELP_STRING([--enable-numerus], [Enable numerus extension])],
  [no])

if test "$PHP_NUMERUS" != "no"; then
  PHP_NEW_EXTENSION([numerus], [numerus.c numerus_storage.c numerus_matrix.c], [$ext_shared])
fi
