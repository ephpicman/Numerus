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
    (void) context;

    if (row == 1) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = row;
    *parent_column = column;
    return NUMERUS_MATRIX_SUCCESS;
}

static void test_norm_values(void)
{
    const double values[] = {1, -2, 3, 4};
    numerus_matrix *root = NULL;
    double norm = 0.0;

    assert(numerus_matrix_create_dense(2, 2, values, &root) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_frobenius_norm(root, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(norm - sqrt(30.0)) < 1e-12);

    assert(numerus_matrix_one_norm(root, &norm) == NUMERUS_MATRIX_SUCCESS);
    assert(norm == 6.0);
    assert(numerus_matrix_infinity_norm(root, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(norm == 7.0);

    numerus_matrix_destroy(root);
}

static void test_frobenius_stability_and_nonfinite_values(void)
{
    const double large_values[] = {3e200, 4e200};
    const double small_values[] = {3e-200, 4e-200};
    const double nan_values[] = {NAN, 0, 0, 1};
    numerus_matrix *large = NULL;
    numerus_matrix *small = NULL;
    numerus_matrix *nan_matrix = NULL;
    double norm = 0.0;

    assert(numerus_matrix_create_dense(1, 2, large_values, &large) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_frobenius_norm(large, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(norm / 5e200 - 1.0) < 1e-14);

    assert(numerus_matrix_create_dense(1, 2, small_values, &small) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_frobenius_norm(small, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(norm / 5e-200 - 1.0) < 1e-14);

    assert(numerus_matrix_create_dense(2, 2, nan_values, &nan_matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_frobenius_norm(nan_matrix, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(isnan(norm));
    assert(numerus_matrix_one_norm(nan_matrix, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(isnan(norm));
    assert(numerus_matrix_infinity_norm(nan_matrix, &norm) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(isnan(norm));

    numerus_matrix_destroy(nan_matrix);
    numerus_matrix_destroy(small);
    numerus_matrix_destroy(large);
}

static void test_invalid_arguments_and_read_failure(void)
{
    const double values[] = {1, 2, 3, 4};
    numerus_matrix *root = NULL;
    numerus_matrix *failing = NULL;
    double norm = 5432.125;

    assert(numerus_matrix_frobenius_norm(NULL, &norm) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(norm == 5432.125);
    assert(numerus_matrix_one_norm(NULL, &norm) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(norm == 5432.125);
    assert(numerus_matrix_infinity_norm(NULL, NULL) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    assert(numerus_matrix_create_dense(2, 2, values, &root) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_from_parent_with_transforms(
        root, 2, 2, fail_on_row_one, NULL, NULL, &failing
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_frobenius_norm(failing, &norm) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(norm == 5432.125);
    assert(numerus_matrix_one_norm(failing, &norm) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(norm == 5432.125);
    assert(numerus_matrix_infinity_norm(failing, &norm) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(norm == 5432.125);

    numerus_matrix_destroy(failing);
    numerus_matrix_destroy(root);
}

int main(void)
{
    test_norm_values();
    test_frobenius_stability_and_nonfinite_values();
    test_invalid_arguments_and_read_failure();
    puts("Matrix norm tests passed.");
    return 0;
}
