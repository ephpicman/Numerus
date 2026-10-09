PHP_ARG_ENABLE([numerus],
  [whether to enable numerus support],
  [AS_HELP_STRING([--enable-numerus], [Enable numerus extension])],
  [no])

if test "$PHP_NUMERUS" != "no"; then
  PHP_NEW_EXTENSION([numerus], [numerus.c numerus_storage.c numerus_matrix.c numerus_matrix_binary.c numerus_matrix_materialize.c numerus_matrix_multiply.c numerus_matrix_compare.c numerus_matrix_constructors.c numerus_matrix_power.c], [$ext_shared])
fi
