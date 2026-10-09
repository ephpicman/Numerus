#include "numerus_matrix.h"
#include "numerus_size.h"

#include <stdlib.h>

#ifndef NUMERUS_MATRIX_USE_LIBC_ALLOC
# include "php.h"
# include "Zend/zend_alloc.h"
# define numerus_multiply_alloc(size) emalloc(size)
# define numerus_multiply_free(ptr) efree(ptr)
#else
# define numerus_multiply_alloc(size) malloc(size)
# define numerus_multiply_free(ptr) free(ptr)
#endif

/**
 * Compute a materialized matrix product using the standard triple loop.
 *
 * The result is independent dense Storage. Parent read errors are propagated,
 * and no partial result is published. Dot products accumulate in double
 * precision using the natural left-to-right order.
 */
int numerus_matrix_multiply(
    const numerus_matrix *left,
    const numerus_matrix *right,
    numerus_matrix **matrix
)
{
    size_t rows;
    size_t inner_dimension;
    size_t columns;
    size_t element_count;
    size_t allocation_size;
    size_t row;
    size_t column;
    double *values;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (left == NULL || right == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    rows = numerus_matrix_rows(left);
    inner_dimension = numerus_matrix_columns(left);
    columns = numerus_matrix_columns(right);

    if (inner_dimension != numerus_matrix_rows(right)) {
        return NUMERUS_MATRIX_DIMENSION_MISMATCH;
    }

    if (rows == 0 || inner_dimension == 0 || columns == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(rows, columns, &element_count) ||
        !numerus_size_multiply(
            element_count, sizeof(*values), &allocation_size
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    values = numerus_multiply_alloc(allocation_size);
    if (values == NULL) {
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }

    for (row = 0; row < rows; row++) {
        for (column = 0; column < columns; column++) {
            size_t inner;
            double sum = 0.0;

            for (inner = 0; inner < inner_dimension; inner++) {
                double left_value;
                double right_value;

                status = numerus_matrix_get(
                    left, row, inner, &left_value
                );
                if (status != NUMERUS_MATRIX_SUCCESS) {
                    numerus_multiply_free(values);
                    return status;
                }

                status = numerus_matrix_get(
                    right, inner, column, &right_value
                );
                if (status != NUMERUS_MATRIX_SUCCESS) {
                    numerus_multiply_free(values);
                    return status;
                }

                sum += left_value * right_value;
            }

            values[row * columns + column] = sum;
        }
    }

    status = numerus_matrix_create_dense(rows, columns, values, matrix);
    numerus_multiply_free(values);

    return status;
}
