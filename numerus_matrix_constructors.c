#include "numerus_matrix.h"

/** Create a row vector represented as a one-row dense Matrix. */
int numerus_matrix_create_row_vector(
    size_t length,
    const double *values,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_dense(1, length, values, matrix);
}

/** Create a column vector represented as a one-column dense Matrix. */
int numerus_matrix_create_column_vector(
    size_t length,
    const double *values,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_dense(length, 1, values, matrix);
}
