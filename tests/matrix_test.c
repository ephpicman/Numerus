#include "../numerus_matrix.h"

#include <assert.h>
#include <stdio.h>

static void test_factory_and_dimensions(void)
{
    const double values[] = {1, 2, 3, 4, 5, 6};
    numerus_matrix_data data = {0};
    numerus_matrix *matrix = NULL;

    data.values = values;

    assert(numerus_matrix_create(
        NUMERUS_STORAGE_DENSE, 2, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_rows(matrix) == 2);
    assert(numerus_matrix_columns(matrix) == 3);
    assert(numerus_matrix_storage_kind(matrix) == NUMERUS_STORAGE_DENSE);

    {
        double value = 0;
        assert(numerus_matrix_get(matrix, 1, 2, &value) == NUMERUS_MATRIX_SUCCESS);
        assert(value == 6.0);
    }

    numerus_matrix_destroy(matrix);
}

static void test_convenience_constructors(void)
{
    numerus_matrix *matrix = NULL;

    assert(numerus_matrix_create_identity(3, &matrix) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_storage_kind(matrix) == NUMERUS_STORAGE_IDENTITY);
    assert(numerus_matrix_get_unchecked(matrix, 1, 1) == 1.0);
    assert(numerus_matrix_get_unchecked(matrix, 1, 2) == 0.0);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_zero(2, 3, &matrix) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 1, 2) == 0.0);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_zero_square(4, &matrix) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_rows(matrix) == 4);
    assert(numerus_matrix_columns(matrix) == 4);
    numerus_matrix_destroy(matrix);

    {
        const double diagonal[] = {10, 20, 30};

        assert(numerus_matrix_create_diagonal(
            3, diagonal, &matrix
        ) == NUMERUS_MATRIX_SUCCESS);
        assert(numerus_matrix_get_unchecked(matrix, 2, 2) == 30.0);
        assert(numerus_matrix_get_unchecked(matrix, 0, 2) == 0.0);
        numerus_matrix_destroy(matrix);
    }

    assert(numerus_matrix_create_constant(
        2, 3, 42.0, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 1, 2) == 42.0);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_scaled_identity(
        3, 7.0, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 2, 2) == 7.0);
    assert(numerus_matrix_get_unchecked(matrix, 2, 1) == 0.0);
    numerus_matrix_destroy(matrix);
}

static void test_all_storage_kinds(void)
{
    const double packed[] = {1, 2, 3, 4, 5, 6};
    const double diagonal[] = {10, 20, 30};
    const double banded[] = {
        0, 1, 2,
        3, 4, 5,
        6, 7, 0
    };
    const numerus_storage_sparse_entry entries[] = {
        {5, 20},
        {1, 10}
    };
    numerus_matrix_data data = {0};
    numerus_matrix *matrix = NULL;

    data.values = packed;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_UPPER_TRIANGULAR, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 0, 2) == 3.0);
    assert(numerus_matrix_get_unchecked(matrix, 2, 0) == 0.0);
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create(
        NUMERUS_STORAGE_LOWER_TRIANGULAR, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 2, 0) == 4.0);
    assert(numerus_matrix_get_unchecked(matrix, 0, 2) == 0.0);
    numerus_matrix_destroy(matrix);

    data.values = diagonal;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_DIAGONAL, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 2, 2) == 30.0);
    numerus_matrix_destroy(matrix);

    data.value = 42.0;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_CONSTANT, 2, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 0, 1) == 42.0);
    numerus_matrix_destroy(matrix);

    data.default_value = -1.0;
    data.entries = entries;
    data.count = 2;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_SPARSE, 2, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 0, 1) == 10.0);
    assert(numerus_matrix_get_unchecked(matrix, 0, 0) == -1.0);
    numerus_matrix_destroy(matrix);

    data.values = banded;
    data.entries = NULL;
    data.count = 0;
    data.lower_bandwidth = 1;
    data.upper_bandwidth = 1;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_BANDED, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 1, 2) == 5.0);
    assert(numerus_matrix_get_unchecked(matrix, 0, 2) == 0.0);
    numerus_matrix_destroy(matrix);

    data.values = packed;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_SYMMETRIC, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get_unchecked(matrix, 0, 2) == 4.0);
    assert(numerus_matrix_get_unchecked(matrix, 2, 0) == 4.0);
    numerus_matrix_destroy(matrix);
}

static void test_parent_chain(void)
{
    const double values[] = {1, 2, 3, 4};
    numerus_matrix *root = NULL;
    numerus_matrix *middle = NULL;
    numerus_matrix *leaf = NULL;

    assert(numerus_matrix_create_dense(
        2, 2, values, &root
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_create_from_parent(
        root, 2, 2, &middle
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_create_from_parent(
        middle, 2, 2, &leaf
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_storage_kind(leaf) == NUMERUS_STORAGE_DENSE);
    assert(numerus_matrix_rows(leaf) == 2);
    assert(numerus_matrix_columns(leaf) == 2);

    {
        double value = 0;

        assert(numerus_matrix_get(leaf, 0, 1, &value) == NUMERUS_MATRIX_SUCCESS);
        assert(value == 2.0);

        assert(numerus_matrix_get(leaf, 1, 0, &value) == NUMERUS_MATRIX_SUCCESS);
        assert(value == 3.0);

        assert(numerus_matrix_get_unchecked(leaf, 1, 1) == 4.0);
    }

    /* Child destruction must not destroy or mutate its parent. */
    numerus_matrix_destroy(leaf);
    assert(numerus_matrix_get_unchecked(middle, 1, 1) == 4.0);

    numerus_matrix_destroy(middle);
    assert(numerus_matrix_get_unchecked(root, 0, 0) == 1.0);

    numerus_matrix_destroy(root);

    /* A derived Matrix can expose different dimensions from its parent. */
    assert(numerus_matrix_create_dense(2, 3, (const double[]){
        1, 2, 3, 4, 5, 6
    }, &root) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_from_parent(
        root, 3, 2, &leaf
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_rows(leaf) == 3);
    assert(numerus_matrix_columns(leaf) == 2);
    numerus_matrix_destroy(leaf);
    numerus_matrix_destroy(root);
}

static void test_validation(void)
{
    numerus_matrix *matrix = NULL;
    double value = 123.0;

    assert(numerus_matrix_create_zero(0, 3, &matrix) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(numerus_matrix_create_zero(3, 0, &matrix) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    assert(numerus_matrix_create(
        NUMERUS_STORAGE_IDENTITY, 2, 3, NULL, &matrix
    ) == NUMERUS_MATRIX_NOT_SQUARE);

    assert(numerus_matrix_create_from_parent(NULL, 2, 2, &matrix) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    assert(numerus_matrix_create_from_parent(
        NULL, 2, 2, NULL
    ) == NUMERUS_MATRIX_INVALID_ARGUMENT);

    assert(numerus_matrix_create_zero(2, 2, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_get(matrix, 2, 0, &value) ==
        NUMERUS_MATRIX_OUT_OF_BOUNDS);
    assert(value == 123.0);

    assert(numerus_matrix_get(matrix, 0, 2, &value) ==
        NUMERUS_MATRIX_OUT_OF_BOUNDS);
    assert(numerus_matrix_get(matrix, 0, 0, NULL) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(numerus_matrix_get(NULL, 0, 0, &value) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    numerus_matrix_destroy(matrix);
}

int main(void)
{
    test_factory_and_dimensions();
    test_convenience_constructors();
    test_all_storage_kinds();
    test_parent_chain();
    test_validation();

    puts("Matrix tests passed.");
    return 0;
}
