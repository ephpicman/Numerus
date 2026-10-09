#include "numerus_matrix.h"
#include "numerus_size.h"

#include <math.h>
#include <stdlib.h>

static numerus_matrix_status matrix_reduce(
    const numerus_matrix *matrix,
    double *sum_result,
    double *minimum_result,
    double *maximum_result
)
{
    double sum = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
    bool has_finite_or_infinite_value = false;
    bool saw_nan = false;
    size_t row;
    size_t column;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }

            sum += value;
            if (isnan(value)) {
                saw_nan = true;
                continue;
            }

            if (!has_finite_or_infinite_value) {
                minimum = value;
                maximum = value;
                has_finite_or_infinite_value = true;
            } else {
                if (value < minimum) {
                    minimum = value;
                }
                if (value > maximum) {
                    maximum = value;
                }
            }
        }
    }

    if (sum_result != NULL) {
        *sum_result = sum;
    }
    if (minimum_result != NULL) {
        *minimum_result = saw_nan ? NAN : minimum;
    }
    if (maximum_result != NULL) {
        *maximum_result = saw_nan ? NAN : maximum;
    }

    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_trace(
    const numerus_matrix *matrix,
    double *trace
)
{
    double result = 0.0;
    size_t index;
    size_t size;

    if (matrix == NULL || trace == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (numerus_matrix_rows(matrix) != numerus_matrix_columns(matrix)) {
        return NUMERUS_MATRIX_NOT_SQUARE;
    }

    size = numerus_matrix_rows(matrix);
    for (index = 0; index < size; index++) {
        double value;
        numerus_matrix_status status = numerus_matrix_get(
            matrix, index, index, &value
        );

        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        result += value;
    }

    *trace = result;
    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_sum(
    const numerus_matrix *matrix,
    double *sum
)
{
    if (matrix == NULL || sum == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return matrix_reduce(matrix, sum, NULL, NULL);
}

numerus_matrix_status numerus_matrix_min(
    const numerus_matrix *matrix,
    double *minimum
)
{
    if (matrix == NULL || minimum == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return matrix_reduce(matrix, NULL, minimum, NULL);
}

numerus_matrix_status numerus_matrix_max(
    const numerus_matrix *matrix,
    double *maximum
)
{
    if (matrix == NULL || maximum == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return matrix_reduce(matrix, NULL, NULL, maximum);
}

numerus_matrix_status numerus_matrix_mean(
    const numerus_matrix *matrix,
    double *mean
)
{
    double sum;
    size_t element_count;
    numerus_matrix_status status;

    if (matrix == NULL || mean == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            numerus_matrix_rows(matrix),
            numerus_matrix_columns(matrix),
            &element_count
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    status = matrix_reduce(matrix, &sum, NULL, NULL);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    *mean = sum / (double) element_count;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status matrix_axis_aggregates(
    const numerus_matrix *matrix,
    bool aggregate_rows,
    bool calculate_mean,
    numerus_matrix **result
)
{
    size_t output_count;
    size_t reduction_count;
    size_t bytes;
    size_t outer;
    size_t inner;
    double *values;
    numerus_matrix_status status;

    if (result == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *result = NULL;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    output_count = aggregate_rows
        ? numerus_matrix_rows(matrix)
        : numerus_matrix_columns(matrix);
    reduction_count = aggregate_rows
        ? numerus_matrix_columns(matrix)
        : numerus_matrix_rows(matrix);

    if (!numerus_size_multiply(output_count, sizeof(*values), &bytes)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    values = malloc(bytes);
    if (values == NULL) {
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }

    for (outer = 0; outer < output_count; outer++) {
        double aggregate = 0.0;

        for (inner = 0; inner < reduction_count; inner++) {
            size_t row = aggregate_rows ? outer : inner;
            size_t column = aggregate_rows ? inner : outer;
            double value;

            status = numerus_matrix_get(matrix, row, column, &value);
            if (status != NUMERUS_MATRIX_SUCCESS) {
                free(values);
                return status;
            }

            aggregate += value;
        }

        values[outer] = calculate_mean
            ? aggregate / (double) reduction_count
            : aggregate;
    }

    status = numerus_matrix_create_dense(
        aggregate_rows ? output_count : 1,
        aggregate_rows ? 1 : output_count,
        values,
        result
    );
    free(values);

    return (numerus_matrix_status) status;
}

numerus_matrix_status numerus_matrix_row_sums(
    const numerus_matrix *matrix,
    numerus_matrix **sums
)
{
    return matrix_axis_aggregates(matrix, true, false, sums);
}

numerus_matrix_status numerus_matrix_column_sums(
    const numerus_matrix *matrix,
    numerus_matrix **sums
)
{
    return matrix_axis_aggregates(matrix, false, false, sums);
}

numerus_matrix_status numerus_matrix_row_means(
    const numerus_matrix *matrix,
    numerus_matrix **means
)
{
    return matrix_axis_aggregates(matrix, true, true, means);
}

numerus_matrix_status numerus_matrix_column_means(
    const numerus_matrix *matrix,
    numerus_matrix **means
)
{
    return matrix_axis_aggregates(matrix, false, true, means);
}


/*
 * Frobenius norm uses hypot() to avoid the avoidable overflow and underflow
 * of summing squared elements. NaN is deliberately propagated even when an
 * infinity is also present, consistently with the aggregate APIs.
 */
numerus_matrix_status numerus_matrix_norm_frobenius(
    const numerus_matrix *matrix,
    double *norm
)
{
    double result = 0.0;
    bool saw_nan = false;
    size_t row;
    size_t column;

    if (matrix == NULL || norm == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (isnan(value)) {
                saw_nan = true;
                continue;
            }
            result = hypot(result, value);
        }
    }

    *norm = saw_nan ? NAN : result;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status matrix_induced_norm(
    const numerus_matrix *matrix,
    bool reduce_columns,
    double *norm
)
{
    size_t outer_count;
    size_t inner_count;
    size_t outer;
    double maximum = 0.0;
    bool saw_nan = false;

    if (matrix == NULL || norm == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    outer_count = reduce_columns
        ? numerus_matrix_columns(matrix)
        : numerus_matrix_rows(matrix);
    inner_count = reduce_columns
        ? numerus_matrix_rows(matrix)
        : numerus_matrix_columns(matrix);

    for (outer = 0; outer < outer_count; outer++) {
        double sum = 0.0;
        size_t inner;

        for (inner = 0; inner < inner_count; inner++) {
            size_t row = reduce_columns ? inner : outer;
            size_t column = reduce_columns ? outer : inner;
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (isnan(value)) {
                saw_nan = true;
                continue;
            }
            sum += fabs(value);
        }

        if (sum > maximum) {
            maximum = sum;
        }
    }

    *norm = saw_nan ? NAN : maximum;
    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_norm_one(
    const numerus_matrix *matrix,
    double *norm
)
{
    return matrix_induced_norm(matrix, true, norm);
}

numerus_matrix_status numerus_matrix_norm_infinity(
    const numerus_matrix *matrix,
    double *norm
)
{
    return matrix_induced_norm(matrix, false, norm);
}


typedef enum {
    MATRIX_FINITE_PREDICATE_HAS_NAN,
    MATRIX_FINITE_PREDICATE_HAS_INFINITY,
    MATRIX_FINITE_PREDICATE_ALL_FINITE
} matrix_finite_predicate;

static numerus_matrix_status matrix_evaluate_finite_predicate(
    const numerus_matrix *matrix,
    matrix_finite_predicate predicate,
    bool *result
)
{
    size_t row;
    size_t column;

    if (matrix == NULL || result == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }

            if (predicate == MATRIX_FINITE_PREDICATE_HAS_NAN && isnan(value)) {
                *result = true;
                return NUMERUS_MATRIX_SUCCESS;
            }
            if (predicate == MATRIX_FINITE_PREDICATE_HAS_INFINITY && isinf(value)) {
                *result = true;
                return NUMERUS_MATRIX_SUCCESS;
            }
            if (predicate == MATRIX_FINITE_PREDICATE_ALL_FINITE && !isfinite(value)) {
                *result = false;
                return NUMERUS_MATRIX_SUCCESS;
            }
        }
    }

    *result = predicate == MATRIX_FINITE_PREDICATE_ALL_FINITE
        || false;
    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_has_nan(
    const numerus_matrix *matrix,
    bool *has_nan
)
{
    return matrix_evaluate_finite_predicate(
        matrix, MATRIX_FINITE_PREDICATE_HAS_NAN, has_nan
    );
}

numerus_matrix_status numerus_matrix_has_infinity(
    const numerus_matrix *matrix,
    bool *has_infinity
)
{
    return matrix_evaluate_finite_predicate(
        matrix, MATRIX_FINITE_PREDICATE_HAS_INFINITY, has_infinity
    );
}

numerus_matrix_status numerus_matrix_is_finite(
    const numerus_matrix *matrix,
    bool *is_finite
)
{
    return matrix_evaluate_finite_predicate(
        matrix, MATRIX_FINITE_PREDICATE_ALL_FINITE, is_finite
    );
}
