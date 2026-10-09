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

/** Select rows using an owned copy of the requested indices. */
int numerus_matrix_create_select_rows(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_selection_view(
        parent,
        indices,
        count,
        NUMERUS_MATRIX_SELECTION_ROWS,
        matrix
    );
}

/** Select columns using an owned copy of the requested indices. */
int numerus_matrix_create_select_columns(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_selection_view(
        parent,
        indices,
        count,
        NUMERUS_MATRIX_SELECTION_COLUMNS,
        matrix
    );
}
