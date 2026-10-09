#include "numerus_matrix.h"
#include "numerus_size.h"

#include <math.h>
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

static numerus_matrix_status matrix_matches_identity(
    const numerus_matrix *matrix,
    bool *matches
)
{
    size_t row;
    size_t column;

    if (matrix == NULL || matches == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (numerus_matrix_rows(matrix) != numerus_matrix_columns(matrix)) {
        *matches = false;
        return NUMERUS_MATRIX_SUCCESS;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            double expected = row == column ? 1.0 : 0.0;
            numerus_matrix_status status =
                numerus_matrix_get(matrix, row, column, &value);

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (value != expected || (value == 0.0 && signbit(value))) {
                *matches = false;
                return NUMERUS_MATRIX_SUCCESS;
            }
        }
    }

    *matches = true;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status matrix_matches_zero(
    const numerus_matrix *matrix,
    bool *matches
)
{
    size_t row;
    size_t column;

    if (matrix == NULL || matches == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            numerus_matrix_status status =
                numerus_matrix_get(matrix, row, column, &value);

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (value != 0.0 || signbit(value)) {
                *matches = false;
                return NUMERUS_MATRIX_SUCCESS;
            }
        }
    }

    *matches = true;
    return NUMERUS_MATRIX_SUCCESS;
}

/*
 * The current multiplication loop evaluates zero-times-value terms. Only
 * nonnegative finite values make identity/zero shortcuts preserve those
 * IEEE-754 results, including the sign of zero.
 */
static numerus_matrix_status matrix_is_finite_nonnegative(
    const numerus_matrix *matrix,
    bool *eligible
)
{
    size_t row;
    size_t column;

    if (matrix == NULL || eligible == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            numerus_matrix_status status =
                numerus_matrix_get(matrix, row, column, &value);

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (!isfinite(value) || value < 0.0 ||
                (value == 0.0 && signbit(value))) {
                *eligible = false;
                return NUMERUS_MATRIX_SUCCESS;
            }
        }
    }

    *eligible = true;
    return NUMERUS_MATRIX_SUCCESS;
}

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

    {
        bool matches;
        bool eligible;

        status = matrix_matches_identity(right, &matches);
        if (status != NUMERUS_MATRIX_SUCCESS) return status;
        if (matches) {
            status = matrix_is_finite_nonnegative(left, &eligible);
            if (status != NUMERUS_MATRIX_SUCCESS) return status;
            if (eligible) return numerus_matrix_materialize(left, matrix);
        }

        status = matrix_matches_identity(left, &matches);
        if (status != NUMERUS_MATRIX_SUCCESS) return status;
        if (matches) {
            status = matrix_is_finite_nonnegative(right, &eligible);
            if (status != NUMERUS_MATRIX_SUCCESS) return status;
            if (eligible) return numerus_matrix_materialize(right, matrix);
        }

        status = matrix_matches_zero(right, &matches);
        if (status != NUMERUS_MATRIX_SUCCESS) return status;
        if (matches) {
            status = matrix_is_finite_nonnegative(left, &eligible);
            if (status != NUMERUS_MATRIX_SUCCESS) return status;
            if (eligible) return numerus_matrix_create_zero(rows, columns, matrix);
        }

        status = matrix_matches_zero(left, &matches);
        if (status != NUMERUS_MATRIX_SUCCESS) return status;
        if (matches) {
            status = matrix_is_finite_nonnegative(right, &eligible);
            if (status != NUMERUS_MATRIX_SUCCESS) return status;
            if (eligible) return numerus_matrix_create_zero(rows, columns, matrix);
        }
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
