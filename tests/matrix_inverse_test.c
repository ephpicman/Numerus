#include "../numerus_matrix.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void assert_identity_product(
    const numerus_matrix *matrix,
    const numerus_matrix *inverse
)
{
    size_t size = numerus_matrix_rows(matrix);
    size_t row;
    size_t column;

    for (row = 0; row < size; row++) {
        for (column = 0; column < size; column++) {
            size_t k;
            double value = 0.0;

            for (k = 0; k < size; k++) {
                double left;
                double right;
                assert(numerus_matrix_get(matrix, row, k, &left) ==
                    NUMERUS_MATRIX_SUCCESS);
                assert(numerus_matrix_get(inverse, k, column, &right) ==
                    NUMERUS_MATRIX_SUCCESS);
                value += left * right;
            }
            assert(fabs(value - (row == column ? 1.0 : 0.0)) < 1e-9);
        }
    }
}

static void test_known_and_pivoted_inverses(void)
{
    const double values[] = {4, 7, 2, 6};
    const double pivot_values[] = {0, 1, 1, 0};
    numerus_matrix *matrix = NULL;
    numerus_matrix *inverse = NULL;

    assert(numerus_matrix_create_dense(2, 2, values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_rows(inverse) == 2);
    assert(numerus_matrix_columns(inverse) == 2);
    assert_identity_product(matrix, inverse);
    numerus_matrix_destroy(inverse);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_dense(2, 2, pivot_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_SUCCESS);
    assert_identity_product(matrix, inverse);
    numerus_matrix_destroy(inverse);
    numerus_matrix_destroy(matrix);
}

static void test_singularity_and_nonfinite_semantics(void)
{
    const double singular_values[] = {1, 2, 2, 4};
    const double near_singular_values[] = {1, 0, 0, 1e-12};
    const double nan_values[] = {1, NAN, 0, 1};
    numerus_matrix *matrix = NULL;
    numerus_matrix *inverse = (void *) 1;

    assert(numerus_matrix_create_dense(2, 2, singular_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_SINGULAR);
    assert(inverse == NULL);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_dense(
        2, 2, near_singular_values, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_SINGULAR);
    assert(inverse == NULL);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_dense(2, 2, nan_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_NON_FINITE);
    assert(inverse == NULL);
    numerus_matrix_destroy(matrix);
}

static void test_validation_and_scale(void)
{
    const double small_values[] = {1e-100, 0, 0, 2e-100};
    const double rectangular_values[] = {1, 2, 3, 4, 5, 6};
    numerus_matrix *matrix = NULL;
    numerus_matrix *inverse = (void *) 1;
    double value;

    assert(numerus_matrix_inverse(NULL, &inverse) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(inverse == NULL);
    assert(numerus_matrix_inverse(NULL, NULL) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    assert(numerus_matrix_create_dense(
        2, 3, rectangular_values, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_NOT_SQUARE);
    assert(inverse == NULL);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_dense(2, 2, small_values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_inverse(matrix, &inverse) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get(inverse, 0, 0, &value) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(value - 1e100) / 1e100 < 1e-12);
    assert(numerus_matrix_get(inverse, 1, 1, &value) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(fabs(value - 5e99) / 5e99 < 1e-12);
    assert_identity_product(matrix, inverse);
    numerus_matrix_destroy(inverse);
    numerus_matrix_destroy(matrix);
}

int main(void)
{
    test_known_and_pivoted_inverses();
    test_singularity_and_nonfinite_semantics();
    test_validation_and_scale();
    puts("Matrix inverse tests passed.");
    return 0;
}
