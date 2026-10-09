#include "../numerus_matrix.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static double at(const numerus_matrix *matrix, size_t row, size_t column)
{
    double value = NAN;
    assert(numerus_matrix_get(matrix, row, column, &value) ==
        NUMERUS_MATRIX_SUCCESS);
    return value;
}

static void assert_matrix_close(
    const numerus_matrix *left,
    const numerus_matrix *right,
    double tolerance
)
{
    assert(numerus_matrix_rows(left) == numerus_matrix_rows(right));
    assert(numerus_matrix_columns(left) == numerus_matrix_columns(right));

    for (size_t row = 0; row < numerus_matrix_rows(left); row++) {
        for (size_t column = 0; column < numerus_matrix_columns(left); column++) {
            double a = at(left, row, column);
            double b = at(right, row, column);
            double scale = fmax(1.0, fmax(fabs(a), fabs(b)));
            assert(fabs(a - b) <= tolerance * scale);
        }
    }
}

static void test_multiplication_transpose_identity(void)
{
    const double a_values[] = {1.0, 2.0, -1.0, 3.0, 0.5, 4.0};
    const double b_values[] = {2.0, 1.0, 0.0, -3.0, 4.0, 2.0};
    numerus_matrix *a = NULL, *b = NULL;
    numerus_matrix *ab = NULL, *ab_t = NULL;
    numerus_matrix *a_t = NULL, *b_t = NULL, *bt_at = NULL;

    assert(numerus_matrix_create_dense(2, 3, a_values, &a) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_dense(3, 2, b_values, &b) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_multiply(a, b, &ab) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_transpose(ab, &ab_t) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_transpose(a, &a_t) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_transpose(b, &b_t) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_multiply(b_t, a_t, &bt_at) == NUMERUS_MATRIX_SUCCESS);

    assert_matrix_close(ab_t, bt_at, 1e-12);

    numerus_matrix_destroy(bt_at);
    numerus_matrix_destroy(b_t);
    numerus_matrix_destroy(a_t);
    numerus_matrix_destroy(ab_t);
    numerus_matrix_destroy(ab);
    numerus_matrix_destroy(b);
    numerus_matrix_destroy(a);
}

static void test_identity_and_zero_laws(void)
{
    const double values[] = {1.0, -2.0, 3.5, 4.0, 0.0, -6.0};
    numerus_matrix *a = NULL, *identity = NULL, *zero = NULL;
    numerus_matrix *ai = NULL, *az = NULL, *sum = NULL;

    assert(numerus_matrix_create_dense(2, 3, values, &a) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_identity(3, &identity) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_zero(3, 3, &zero) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_multiply(a, identity, &ai) == NUMERUS_MATRIX_SUCCESS);
    assert_matrix_close(a, ai, 0.0);

    assert(numerus_matrix_multiply(a, zero, &az) == NUMERUS_MATRIX_SUCCESS);
    for (size_t row = 0; row < 2; row++) {
        for (size_t column = 0; column < 3; column++) {
            assert(at(az, row, column) == 0.0);
        }
    }

    numerus_matrix_destroy(az);
    numerus_matrix_destroy(zero);
    numerus_matrix_destroy(identity);

    assert(numerus_matrix_create_zero(2, 3, &zero) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_add(a, zero, &sum) == NUMERUS_MATRIX_SUCCESS);
    assert_matrix_close(a, sum, 0.0);

    numerus_matrix_destroy(sum);
    numerus_matrix_destroy(zero);
    numerus_matrix_destroy(ai);
    numerus_matrix_destroy(a);
}

int main(void)
{
    test_multiplication_transpose_identity();
    test_identity_and_zero_laws();
    puts("Matrix property tests passed.");
    return 0;
}
