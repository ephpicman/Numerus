#include "numerus_matrix_internal.h"
#include "numerus_size.h"

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

/** Create a lazy reshape view preserving row-major traversal order. */
int numerus_matrix_create_reshape(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_reshape_view(
        parent, rows, columns, matrix
    );
}

/** Flatten a Matrix into a single row in row-major order. */
int numerus_matrix_create_flatten(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    size_t element_count;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            numerus_matrix_rows(parent),
            numerus_matrix_columns(parent),
            &element_count
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    return numerus_matrix_create_reshape_view(
        parent, 1, element_count, matrix
    );
}
