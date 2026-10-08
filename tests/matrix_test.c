#include "../numerus_matrix.h"

#include <assert.h>
#include <stdio.h>

static int matrix_value_equals(
    const numerus_matrix *matrix,
    size_t row,
    size_t column,
    double expected
)
{
    double actual = 0.0;

    return numerus_matrix_get_unchecked(
        matrix, row, column, &actual
    ) == NUMERUS_MATRIX_SUCCESS && actual == expected;
}

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
    assert(matrix_value_equals(matrix, 1, 1, 1.0));
    assert(matrix_value_equals(matrix, 1, 2, 0.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_zero(2, 3, &matrix) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 1, 2, 0.0));
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
        assert(matrix_value_equals(matrix, 2, 2, 30.0));
        assert(matrix_value_equals(matrix, 0, 2, 0.0));
        numerus_matrix_destroy(matrix);
    }

    assert(numerus_matrix_create_constant(
        2, 3, 42.0, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 1, 2, 42.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_scaled_identity(
        3, 7.0, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 2, 2, 7.0));
    assert(matrix_value_equals(matrix, 2, 1, 0.0));
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

    assert(numerus_matrix_create_upper_triangular(
        3, packed, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 2, 3.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_lower_triangular(
        3, packed, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 2, 0, 4.0));
    numerus_matrix_destroy(matrix);

    data.values = packed;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_UPPER_TRIANGULAR, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 2, 3.0));
    assert(matrix_value_equals(matrix, 2, 0, 0.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create(
        NUMERUS_STORAGE_LOWER_TRIANGULAR, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 2, 0, 4.0));
    assert(matrix_value_equals(matrix, 0, 2, 0.0));
    numerus_matrix_destroy(matrix);

    data.values = diagonal;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_DIAGONAL, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 2, 2, 30.0));
    numerus_matrix_destroy(matrix);

    data.value = 42.0;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_CONSTANT, 2, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 1, 42.0));
    numerus_matrix_destroy(matrix);

    data.default_value = -1.0;
    data.entries = entries;
    data.count = 2;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_SPARSE, 2, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 1, 10.0));
    assert(matrix_value_equals(matrix, 0, 0, -1.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_sparse(
        2, 3, -1.0, entries, 2, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 1, 10.0));
    numerus_matrix_destroy(matrix);

    data.values = banded;
    data.entries = NULL;
    data.count = 0;
    data.lower_bandwidth = 1;
    data.upper_bandwidth = 1;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_BANDED, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 1, 2, 5.0));
    assert(matrix_value_equals(matrix, 0, 2, 0.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_symmetric(
        3, packed, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 2, 4.0));
    numerus_matrix_destroy(matrix);

    assert(numerus_matrix_create_banded(
        3, 3, 1, 1, banded, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 1, 2, 5.0));
    numerus_matrix_destroy(matrix);

    data.values = packed;
    assert(numerus_matrix_create(
        NUMERUS_STORAGE_SYMMETRIC, 3, 3, &data, &matrix
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(matrix_value_equals(matrix, 0, 2, 4.0));
    assert(matrix_value_equals(matrix, 2, 0, 4.0));
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

        assert(matrix_value_equals(leaf, 1, 1, 4.0));
    }

    /* Child destruction must not destroy or mutate its parent. */
    numerus_matrix_destroy(leaf);
    assert(matrix_value_equals(middle, 1, 1, 4.0));

    numerus_matrix_destroy(middle);
    assert(matrix_value_equals(root, 0, 0, 1.0));

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

static numerus_matrix_status transpose_coordinates(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    (void) context;

    *parent_row = column;
    *parent_column = row;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status diagonal_index_values(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    void *context
)
{
    (void) context;

    *result = row == column ? (double) (row + 1) : parent_value;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status invalid_coordinates(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    (void) row;
    (void) column;
    (void) parent_row;
    (void) parent_column;
    (void) context;

    return NUMERUS_MATRIX_INVALID_ARGUMENT;
}

static numerus_matrix_status out_of_bounds_coordinates(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    (void) context;

    *parent_row = row + 2;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status invalid_value(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    void *context
)
{
    (void) row;
    (void) column;
    (void) parent_value;
    (void) result;
    (void) context;

    return NUMERUS_MATRIX_INVALID_ARGUMENT;
}

static void test_transpose_transform(void)
{
    const double values[] = {1, 2, 3, 4, 5, 6};
    numerus_matrix *parent = NULL;
    numerus_matrix *child = NULL;
    double value = 123.0;

    assert(numerus_matrix_create_dense(2, 3, values, &parent) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_from_parent_with_transforms(
        parent, 3, 2, transpose_coordinates, NULL, NULL, &child
    ) == NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_rows(child) == 3);
    assert(numerus_matrix_columns(child) == 2);
    assert(matrix_value_equals(child, 0, 0, 1.0));
    assert(matrix_value_equals(child, 0, 1, 4.0));
    assert(matrix_value_equals(child, 1, 0, 2.0));
    assert(matrix_value_equals(child, 1, 1, 5.0));
    assert(matrix_value_equals(child, 2, 0, 3.0));
    assert(matrix_value_equals(child, 2, 1, 6.0));

    assert(numerus_matrix_get(child, 3, 0, &value) ==
        NUMERUS_MATRIX_OUT_OF_BOUNDS);
    assert(value == 123.0);

    numerus_matrix_destroy(child);
    numerus_matrix_destroy(parent);
}

static void test_transpose_constructor(void)
{
    const double values[] = {1, 2, 3, 4, 5, 6};
    numerus_matrix *parent = NULL;
    numerus_matrix *transpose = NULL;
    numerus_matrix *double_transpose = NULL;
    numerus_matrix *invalid_output = NULL;

    assert(numerus_matrix_create_dense(2, 3, values, &parent) ==
        NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_create_transpose(parent, &transpose) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_rows(transpose) == 3);
    assert(numerus_matrix_columns(transpose) == 2);
    assert(numerus_matrix_storage_kind(transpose) == NUMERUS_STORAGE_DENSE);

    assert(matrix_value_equals(transpose, 0, 0, 1.0));
    assert(matrix_value_equals(transpose, 0, 1, 4.0));
    assert(matrix_value_equals(transpose, 1, 0, 2.0));
    assert(matrix_value_equals(transpose, 1, 1, 5.0));
    assert(matrix_value_equals(transpose, 2, 0, 3.0));
    assert(matrix_value_equals(transpose, 2, 1, 6.0));

    assert(numerus_matrix_create_transpose(transpose, &double_transpose) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_rows(double_transpose) == 2);
    assert(numerus_matrix_columns(double_transpose) == 3);

    for (size_t row = 0; row < 2; row++) {
        for (size_t column = 0; column < 3; column++) {
            assert(matrix_value_equals(
                double_transpose,
                row,
                column,
                values[row * 3 + column]
            ));
        }
    }

    assert(numerus_matrix_create_transpose(NULL, &invalid_output) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(numerus_matrix_create_transpose(parent, NULL) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);

    numerus_matrix_destroy(double_transpose);
    numerus_matrix_destroy(transpose);
    numerus_matrix_destroy(parent);
}

static void test_value_transform(void)
{
    numerus_matrix *parent = NULL;
    numerus_matrix *child = NULL;

    assert(numerus_matrix_create_identity(4, &parent) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_create_from_parent_with_transforms(
        parent, 4, 4, NULL, diagonal_index_values, NULL, &child
    ) == NUMERUS_MATRIX_SUCCESS);

    for (size_t i = 0; i < 4; i++) {
        assert(matrix_value_equals(child, i, i, (double) (i + 1)));
    }

    assert(matrix_value_equals(child, 0, 1, 0.0));
    assert(matrix_value_equals(child, 1, 3, 0.0));
    assert(matrix_value_equals(child, 3, 0, 0.0));

    numerus_matrix_destroy(child);
    numerus_matrix_destroy(parent);
}

static void test_transform_error_propagation(void)
{
    numerus_matrix *parent = NULL;
    numerus_matrix *child = NULL;
    double value = 123.0;

    assert(numerus_matrix_create_zero(2, 2, &parent) ==
        NUMERUS_MATRIX_SUCCESS);

    assert(numerus_matrix_create_from_parent_with_transforms(
        parent, 2, 2, invalid_coordinates, NULL, NULL, &child
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get(child, 0, 0, &value) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(value == 123.0);
    numerus_matrix_destroy(child);

    assert(numerus_matrix_create_from_parent_with_transforms(
        parent, 2, 2, out_of_bounds_coordinates, NULL, NULL, &child
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get(child, 0, 0, &value) ==
        NUMERUS_MATRIX_OUT_OF_BOUNDS);
    assert(value == 123.0);
    numerus_matrix_destroy(child);

    assert(numerus_matrix_create_from_parent_with_transforms(
        parent, 2, 2, NULL, invalid_value, NULL, &child
    ) == NUMERUS_MATRIX_SUCCESS);
    assert(numerus_matrix_get(child, 0, 0, &value) ==
        NUMERUS_MATRIX_INVALID_ARGUMENT);
    assert(value == 123.0);
    numerus_matrix_destroy(child);

    numerus_matrix_destroy(parent);
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
    test_transpose_transform();
    test_transpose_constructor();
    test_value_transform();
    test_transform_error_propagation();
    test_validation();

    puts("Matrix tests passed.");
    return 0;
}
