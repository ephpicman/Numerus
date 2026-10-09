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

numerus_matrix_status numerus_matrix_frobenius_norm(
    const numerus_matrix *matrix,
    double *norm
)
{
    double scale = 0.0;
    double sum_squares = 1.0;
    bool saw_nan = false;
    bool saw_infinity = false;
    size_t row;
    size_t column;

    if (matrix == NULL || norm == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    for (row = 0; row < numerus_matrix_rows(matrix); row++) {
        for (column = 0; column < numerus_matrix_columns(matrix); column++) {
            double value;
            double absolute_value;
            double ratio;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }

            absolute_value = fabs(value);
            if (isnan(absolute_value)) {
                saw_nan = true;
                continue;
            }
            if (isinf(absolute_value)) {
                saw_infinity = true;
                continue;
            }
            if (absolute_value == 0.0) {
                continue;
            }

            if (scale < absolute_value) {
                ratio = scale / absolute_value;
                sum_squares = 1.0 + sum_squares * ratio * ratio;
                scale = absolute_value;
            } else {
                ratio = absolute_value / scale;
                sum_squares += ratio * ratio;
            }
        }
    }

    if (saw_nan) {
        *norm = NAN;
    } else if (saw_infinity) {
        *norm = INFINITY;
    } else if (scale == 0.0) {
        *norm = 0.0;
    } else {
        *norm = scale * sqrt(sum_squares);
    }

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status matrix_induced_norm(
    const numerus_matrix *matrix,
    bool by_columns,
    double *norm
)
{
    double maximum_sum = 0.0;
    bool saw_nan = false;
    size_t outer_count;
    size_t inner_count;
    size_t outer;
    size_t inner;

    if (matrix == NULL || norm == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    outer_count = by_columns
        ? numerus_matrix_columns(matrix)
        : numerus_matrix_rows(matrix);
    inner_count = by_columns
        ? numerus_matrix_rows(matrix)
        : numerus_matrix_columns(matrix);

    for (outer = 0; outer < outer_count; outer++) {
        double sum = 0.0;

        for (inner = 0; inner < inner_count; inner++) {
            size_t row = by_columns ? inner : outer;
            size_t column = by_columns ? outer : inner;
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }

            if (isnan(value)) {
                saw_nan = true;
            }
            sum += fabs(value);
        }

        if (sum > maximum_sum) {
            maximum_sum = sum;
        }
    }

    *norm = saw_nan ? NAN : maximum_sum;
    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_one_norm(
    const numerus_matrix *matrix,
    double *norm
)
{
    return matrix_induced_norm(matrix, true, norm);
}

numerus_matrix_status numerus_matrix_infinity_norm(
    const numerus_matrix *matrix,
    double *norm
)
{
    return matrix_induced_norm(matrix, false, norm);
}
