#include "numerus_matrix_internal.h"

/** Public wrapper around the private checked slice-view constructor. */
int numerus_matrix_create_slice(
    numerus_matrix *parent,
    size_t row_start,
    size_t row_count,
    size_t column_start,
    size_t column_count,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_slice_view(
        parent,
        row_start,
        row_count,
        column_start,
        column_count,
        matrix
    );
}
