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

static void test_cached_analysis_lifetime(void)
{
    const double values[] = {4.0, 1.0, 1.0, 3.0};

    for (size_t iteration = 0; iteration < 32; iteration++) {
        numerus_matrix *matrix = NULL;
        numerus_matrix *transpose = NULL;
        numerus_matrix_flags first_flags, second_flags;
        double first_determinant = 0.0, second_determinant = 0.0;

        assert(numerus_matrix_create_dense(2, 2, values, &matrix) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_get_flags(matrix, &first_flags) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_get_flags(matrix, &second_flags) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(first_flags.identity == second_flags.identity);
        assert(first_flags.symmetric == second_flags.symmetric);

        assert(numerus_matrix_determinant(matrix, &first_determinant) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_determinant(matrix, &second_determinant) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(first_determinant == second_determinant);
        assert(first_determinant == 11.0);

        assert(numerus_matrix_create_transpose(matrix, &transpose) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_get_flags(transpose, &first_flags) ==
            NUMERUS_MATRIX_SUCCESS);

        numerus_matrix_destroy(transpose);
        numerus_matrix_destroy(matrix);
    }
}

int main(void)
{
    test_multiplication_transpose_identity();
    test_identity_and_zero_laws();
    test_cached_analysis_lifetime();
    puts("Matrix property tests passed.");
    return 0;
}
