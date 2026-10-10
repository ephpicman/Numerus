#!/bin/sh
set -eu

include_flags="-Isrc/core -Isrc/storage -Isrc/matrix -Isrc/statistics"
for header in \
    numerus_matrix_types.h \
    numerus_matrix_core.h \
    numerus_matrix_constructors.h \
    numerus_matrix_views.h \
    numerus_matrix_operations.h \
    numerus_matrix_linalg.h \
    numerus_matrix.h
do
    printf '#include "%s"\nint main(void) { return 0; }\n' "$header" |
        # Intentional word splitting supplies the individual -I arguments.
        cc $include_flags -std=c11 -Wall -Wextra -Wpedantic -Werror \
            -x c -fsyntax-only -
done
