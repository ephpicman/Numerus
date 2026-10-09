#include "../numerus_matrix.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static numerus_matrix_status fail_on_row_one(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    (void) column;
    (void) context;
    if (row == 1) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *parent_row = row;
    *parent_column = column;
    return NUMERUS_MATRIX_SUCCESS;
}

static void test_equal_weights_match_ordinary_least_squares(void)
{
    const double matrix_values[] = {
        1, 0,
        0, 1,
        1, 1
    };
    const double rhs_values[] = {1, 2, 4};
    const double weights[] = {1, 1, 1};
    numerus_matrix *matrix = NULL;
    numerus_matrix *rhs = NULL;
    numerus_matrix *ordinary = NULL;
    numerus_matrix *weighted = NULL;
    size_t row;

    assert(numerus_matrix_create_dense(3, 2, matrix_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_dense(3, 1, rhs_values, &rhs) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_least_squares(matrix, rhs, &ordinary) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, weights, 3, &weighted
    ) == NUMERUS_MATRIX_SUCCESS);

    for (row = 0; row < 2; row++) {
        double left;
        double right;
        assert(numerus_matrix_get(ordinary, row, 0, &left) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_get(weighted, row, 0, &right) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(fabs(left - right) < 1e-10);
    }

    numerus_matrix_destroy(weighted);
    numerus_matrix_destroy(ordinary);
    numerus_matrix_destroy(rhs);
    numerus_matrix_destroy(matrix);
}

static void test_zero_weight_excludes_observation(void)
{
    const double matrix_values[] = {
        1, 0,
        0, 1,
        1, 1
    };
    const double rhs_values[] = {1, 2, 100};
    const double weights[] = {1, 1, 0};
    numerus_matrix *matrix = NULL;
    numerus_matrix *rhs = NULL;
    numerus_matrix *solution = NULL;
    double value;

    assert(numerus_matrix_create_dense(3, 2, matrix_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_dense(3, 1, rhs_values, &rhs) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, weights, 3, &solution
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get(solution, 0, 0, &value) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(value - 1.0) < 1e-10);
    assert(numerus_matrix_get(solution, 1, 0, &value) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(value - 2.0) < 1e-10);

    numerus_matrix_destroy(solution);
    numerus_matrix_destroy(rhs);
    numerus_matrix_destroy(matrix);
}

static void test_validation_and_rank_failure(void)
{
    const double matrix_values[] = {1, 0, 1, 0, 1, 1};
    const double rhs_values[] = {1, 2, 3};
    const double bad_weights[] = {1, -1, 1};
    const double nan_weights[] = {1, NAN, 1};
    const double zero_weights[] = {1, 1, 0};
    numerus_matrix *matrix = NULL;
    numerus_matrix *rhs = NULL;
    numerus_matrix *solution = (void *) 1;

    assert(numerus_matrix_create_dense(3, 2, matrix_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_dense(3, 1, rhs_values, &rhs) ==
        NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, bad_weights, 3, &solution
    ) == NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(solution == NULL);

    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, nan_weights, 3, &solution
    ) == NUMERUS_MATRIX_NON_FINITE);
    assert(solution == NULL);

    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, zero_weights, 2, &solution
    ) == NUMERUS_MATRIX_DIMENSION_MISMATCH);
    assert(solution == NULL);

    assert(numerus_matrix_weighted_least_squares(
        matrix, rhs, zero_weights, 3, &solution
    ) == NUMERUS_MATRIX_RANK_DEFICIENT);
    assert(solution == NULL);

    numerus_matrix_destroy(rhs);
    numerus_matrix_destroy(matrix);
}

static void test_nonfinite_and_read_failures(void)
{
    const double matrix_values[] = {1, 0, 0, 1, 1, 1};
    const double rhs_values[] = {1, 2, 3};
    const double weights[] = {1, 1, 1};
    numerus_matrix *matrix = NULL;
    numerus_matrix *rhs = NULL;
    numerus_matrix *failing = NULL;
    numerus_matrix *solution = (void *) 1;

    assert(numerus_matrix_create_dense(3, 2, matrix_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_dense(3, 1, rhs_values, &rhs) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_from_parent_with_transforms(
        matrix, 3, 2, fail_on_row_one, NULL, NULL, &failing
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_weighted_least_squares(
        failing, rhs, weights, 3, &solution
    ) == NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(solution == NULL);

    numerus_matrix_destroy(failing);
    numerus_matrix_destroy(rhs);
    numerus_matrix_destroy(matrix);
}

int main(void)
{
    test_equal_weights_match_ordinary_least_squares();
    test_zero_weight_excludes_observation();
    test_validation_and_rank_failure();
    test_nonfinite_and_read_failures();
    puts("Matrix weighted least-squares tests passed.");
    return 0;
}
