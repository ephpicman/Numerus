#include "numerus_matrix.h"

#include <stdint.h>
#include <stdlib.h>

#ifndef NUMERUS_MATRIX_USE_LIBC_ALLOC
# include "php.h"
# include "Zend/zend_alloc.h"
# define numerus_matrix_alloc(size) emalloc(size)
# define numerus_matrix_free(ptr) efree(ptr)
#else
# define numerus_matrix_alloc(size) malloc(size)
# define numerus_matrix_free(ptr) free(ptr)
#endif

struct numerus_matrix {
    size_t rows;
    size_t columns;
    numerus_storage *storage;
    numerus_matrix *parent;
    numerus_matrix *parent2;
    numerus_matrix_join_type join_type;
    numerus_coordinate_transform_fn coordinate_transform;
    numerus_value_transform_fn value_transform;
    void *transform_context;
};

static numerus_matrix_status identity_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    (void) context;

    if (parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = row;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status passthrough_value_transform(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    void *context
)
{
    (void) row;
    (void) column;
    (void) context;

    if (result == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *result = parent_value;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status transpose_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    (void) context;

    if (parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = column;
    *parent_column = row;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status flip_rows_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    const numerus_matrix *parent = context;

    if (parent == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = numerus_matrix_rows(parent) - 1 - row;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status flip_columns_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    const numerus_matrix *parent = context;

    if (parent == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = row;
    *parent_column = numerus_matrix_columns(parent) - 1 - column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status rotate_90_clockwise_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    const numerus_matrix *parent = context;

    if (parent == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = numerus_matrix_rows(parent) - 1 - column;
    *parent_column = row;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status rotate_180_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    const numerus_matrix *parent = context;

    if (parent == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = numerus_matrix_rows(parent) - 1 - row;
    *parent_column = numerus_matrix_columns(parent) - 1 - column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status rotate_90_counterclockwise_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    void *context
)
{
    const numerus_matrix *parent = context;

    if (parent == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = column;
    *parent_column = numerus_matrix_columns(parent) - 1 - row;

    return NUMERUS_MATRIX_SUCCESS;
}

static int dimensions_valid(size_t rows, size_t columns)
{
    return rows != 0 && columns != 0;
}

static int dimensions_are_square(size_t rows, size_t columns)
{
    return rows == columns;
}

static numerus_matrix_status storage_status_to_matrix_status(int status)
{
    switch (status) {
        case NUMERUS_STORAGE_SUCCESS:
            return NUMERUS_MATRIX_SUCCESS;
        case NUMERUS_STORAGE_INVALID_ARGUMENT:
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        case NUMERUS_STORAGE_OVERFLOW:
            return NUMERUS_MATRIX_OVERFLOW;
        case NUMERUS_STORAGE_OUT_OF_MEMORY:
            return NUMERUS_MATRIX_OUT_OF_MEMORY;
        case NUMERUS_STORAGE_OUT_OF_BOUNDS:
            return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }

    return NUMERUS_MATRIX_INVALID_ARGUMENT;
}

static int allocate_matrix(numerus_matrix **matrix)
{
    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *matrix = numerus_matrix_alloc(sizeof(**matrix));
    if (*matrix == NULL) {
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }

    (*matrix)->rows = 0;
    (*matrix)->columns = 0;
    (*matrix)->storage = NULL;
    (*matrix)->parent = NULL;
    (*matrix)->parent2 = NULL;
    (*matrix)->join_type = NUMERUS_MATRIX_JOIN_HORIZONTAL;
    (*matrix)->coordinate_transform = NULL;
    (*matrix)->value_transform = NULL;
    (*matrix)->transform_context = NULL;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status create_storage(
    numerus_storage_kind kind,
    size_t rows,
    size_t columns,
    const numerus_matrix_data *data,
    numerus_storage **storage
)
{
    if (storage == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    switch (kind) {
        case NUMERUS_STORAGE_DENSE:
            return storage_status_to_matrix_status(numerus_storage_create_dense(
                rows,
                columns,
                data == NULL ? NULL : data->values,
                storage
            ));

        case NUMERUS_STORAGE_UPPER_TRIANGULAR:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_upper_triangular(
                rows,
                data == NULL ? NULL : data->values,
                storage
            ));

        case NUMERUS_STORAGE_LOWER_TRIANGULAR:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_lower_triangular(
                rows,
                data == NULL ? NULL : data->values,
                storage
            ));

        case NUMERUS_STORAGE_DIAGONAL:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_diagonal(
                rows,
                data == NULL ? NULL : data->values,
                storage
            ));

        case NUMERUS_STORAGE_IDENTITY:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_identity(rows, storage));

        case NUMERUS_STORAGE_CONSTANT:
            return storage_status_to_matrix_status(numerus_storage_create_constant(
                rows,
                columns,
                data == NULL ? 0.0 : data->value,
                storage
            ));

        case NUMERUS_STORAGE_ZERO:
            return storage_status_to_matrix_status(numerus_storage_create_zero(rows, columns, storage));

        case NUMERUS_STORAGE_SCALED_IDENTITY:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_scaled_identity(
                rows,
                data == NULL ? 0.0 : data->value,
                storage
            ));

        case NUMERUS_STORAGE_SPARSE:
            return storage_status_to_matrix_status(numerus_storage_create_sparse(
                rows,
                columns,
                data == NULL ? 0.0 : data->default_value,
                data == NULL ? NULL : data->entries,
                data == NULL ? 0 : data->count,
                storage
            ));

        case NUMERUS_STORAGE_SYMMETRIC:
            if (!dimensions_are_square(rows, columns)) {
                return NUMERUS_MATRIX_NOT_SQUARE;
            }
            return storage_status_to_matrix_status(numerus_storage_create_symmetric(
                rows,
                data == NULL ? NULL : data->values,
                storage
            ));

        case NUMERUS_STORAGE_BANDED:
            return storage_status_to_matrix_status(numerus_storage_create_banded(
                rows,
                columns,
                data == NULL ? 0 : data->lower_bandwidth,
                data == NULL ? 0 : data->upper_bandwidth,
                data == NULL ? NULL : data->values,
                storage
            ));
    }

    return NUMERUS_MATRIX_INVALID_ARGUMENT;
}

/**
 * Create a root Matrix by constructing and owning its immutable Storage.
 */
int numerus_matrix_create(
    numerus_storage_kind kind,
    size_t rows,
    size_t columns,
    const numerus_matrix_data *data,
    numerus_matrix **matrix
)
{
    numerus_storage *storage = NULL;
    numerus_matrix *result = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *matrix = NULL;

    if (!dimensions_valid(rows, columns)) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    status = create_storage(kind, rows, columns, data, &storage);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    status = allocate_matrix(&result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_storage_destroy(storage);
        return status;
    }

    result->rows = rows;
    result->columns = columns;
    result->storage = storage;
    result->parent = NULL;
    result->parent2 = NULL;
    result->coordinate_transform = NULL;
    result->value_transform = NULL;
    result->transform_context = NULL;
    *matrix = result;

    return NUMERUS_MATRIX_SUCCESS;
}

/**
 * Create a derived Matrix that delegates all reads to its parent.
 */
int numerus_matrix_create_from_parent(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        rows,
        columns,
        NULL,
        NULL,
        NULL,
        matrix
    );
}

int numerus_matrix_create_from_parent_with_transforms(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_coordinate_transform_fn coordinate_transform,
    numerus_value_transform_fn value_transform,
    void *context,
    numerus_matrix **matrix
)
{
    numerus_matrix *result;
    int status;

    if (parent == NULL || matrix == NULL || !dimensions_valid(rows, columns)) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *matrix = NULL;

    status = allocate_matrix(&result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    result->rows = rows;
    result->columns = columns;
    result->storage = NULL;
    result->parent = parent;
    result->parent2 = NULL;
    result->coordinate_transform = coordinate_transform == NULL
        ? identity_coordinate_transform
        : coordinate_transform;
    result->value_transform = value_transform == NULL
        ? passthrough_value_transform
        : value_transform;
    result->transform_context = context;
    *matrix = result;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_transpose(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_columns(parent),
        numerus_matrix_rows(parent),
        transpose_coordinate_transform,
        NULL,
        NULL,
        matrix
    );
}

int numerus_matrix_create_flip_rows(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        flip_rows_coordinate_transform,
        NULL,
        parent,
        matrix
    );
}

int numerus_matrix_create_flip_columns(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        flip_columns_coordinate_transform,
        NULL,
        parent,
        matrix
    );
}

int numerus_matrix_create_rotate_90_clockwise(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_columns(parent),
        numerus_matrix_rows(parent),
        rotate_90_clockwise_coordinate_transform,
        NULL,
        parent,
        matrix
    );
}

int numerus_matrix_create_rotate_180(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        rotate_180_coordinate_transform,
        NULL,
        parent,
        matrix
    );
}

int numerus_matrix_create_rotate_90_counterclockwise(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    if (parent == NULL || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    return numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_columns(parent),
        numerus_matrix_rows(parent),
        rotate_90_counterclockwise_coordinate_transform,
        NULL,
        parent,
        matrix
    );
}

static int numerus_matrix_create_join(
    numerus_matrix *parent,
    numerus_matrix *parent2,
    numerus_matrix_join_type join_type,
    numerus_matrix **matrix
)
{
    numerus_matrix *result = NULL;
    size_t rows;
    size_t columns;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || parent2 == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (join_type == NUMERUS_MATRIX_JOIN_HORIZONTAL) {
        if (numerus_matrix_rows(parent) != numerus_matrix_rows(parent2)) {
            return NUMERUS_MATRIX_DIMENSION_MISMATCH;
        }

        rows = numerus_matrix_rows(parent);
        if (numerus_matrix_columns(parent) > SIZE_MAX - numerus_matrix_columns(parent2)) {
            return NUMERUS_MATRIX_OVERFLOW;
        }
        columns = numerus_matrix_columns(parent) + numerus_matrix_columns(parent2);
    } else if (join_type == NUMERUS_MATRIX_JOIN_VERTICAL) {
        if (numerus_matrix_columns(parent) != numerus_matrix_columns(parent2)) {
            return NUMERUS_MATRIX_DIMENSION_MISMATCH;
        }

        columns = numerus_matrix_columns(parent);
        if (numerus_matrix_rows(parent) > SIZE_MAX - numerus_matrix_rows(parent2)) {
            return NUMERUS_MATRIX_OVERFLOW;
        }
        rows = numerus_matrix_rows(parent) + numerus_matrix_rows(parent2);
    } else {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    status = allocate_matrix(&result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    result->rows = rows;
    result->columns = columns;
    result->storage = NULL;
    result->parent = parent;
    result->parent2 = parent2;
    result->join_type = join_type;
    result->coordinate_transform = NULL;
    result->value_transform = NULL;
    result->transform_context = NULL;
    *matrix = result;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_join_horizontal(
    numerus_matrix *parent,
    numerus_matrix *parent2,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_join(
        parent, parent2, NUMERUS_MATRIX_JOIN_HORIZONTAL, matrix
    );
}

int numerus_matrix_create_join_vertical(
    numerus_matrix *parent,
    numerus_matrix *parent2,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_join(
        parent, parent2, NUMERUS_MATRIX_JOIN_VERTICAL, matrix
    );
}

int numerus_matrix_create_dense(
    size_t rows,
    size_t columns,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};

    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_DENSE,
        rows,
        columns,
        &data,
        matrix
    );
}

int numerus_matrix_create_identity(size_t size, numerus_matrix **matrix)
{
    return numerus_matrix_create(
        NUMERUS_STORAGE_IDENTITY,
        size,
        size,
        NULL,
        matrix
    );
}

int numerus_matrix_create_zero(
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create(
        NUMERUS_STORAGE_ZERO,
        rows,
        columns,
        NULL,
        matrix
    );
}

int numerus_matrix_create_zero_square(size_t size, numerus_matrix **matrix)
{
    return numerus_matrix_create_zero(size, size, matrix);
}

int numerus_matrix_create_diagonal(
    size_t size,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};

    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_DIAGONAL,
        size,
        size,
        &data,
        matrix
    );
}

int numerus_matrix_create_constant(
    size_t rows,
    size_t columns,
    double value,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};

    data.value = value;
    return numerus_matrix_create(
        NUMERUS_STORAGE_CONSTANT,
        rows,
        columns,
        &data,
        matrix
    );
}

int numerus_matrix_create_scaled_identity(
    size_t size,
    double value,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};

    data.value = value;
    return numerus_matrix_create(
        NUMERUS_STORAGE_SCALED_IDENTITY,
        size,
        size,
        &data,
        matrix
    );
}

int numerus_matrix_create_upper_triangular(
    size_t size,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};
    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_UPPER_TRIANGULAR, size, size, &data, matrix
    );
}

int numerus_matrix_create_lower_triangular(
    size_t size,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};
    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_LOWER_TRIANGULAR, size, size, &data, matrix
    );
}

int numerus_matrix_create_sparse(
    size_t rows,
    size_t columns,
    double default_value,
    const numerus_storage_sparse_entry *entries,
    size_t count,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};
    data.default_value = default_value;
    data.entries = entries;
    data.count = count;
    return numerus_matrix_create(
        NUMERUS_STORAGE_SPARSE, rows, columns, &data, matrix
    );
}

int numerus_matrix_create_symmetric(
    size_t size,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};
    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_SYMMETRIC, size, size, &data, matrix
    );
}

int numerus_matrix_create_banded(
    size_t rows,
    size_t columns,
    size_t lower_bandwidth,
    size_t upper_bandwidth,
    const double *values,
    numerus_matrix **matrix
)
{
    numerus_matrix_data data = {0};
    data.lower_bandwidth = lower_bandwidth;
    data.upper_bandwidth = upper_bandwidth;
    data.values = values;
    return numerus_matrix_create(
        NUMERUS_STORAGE_BANDED, rows, columns, &data, matrix
    );
}

/**
 * Read a Matrix element without validating this Matrix's own coordinates.
 *
 * Parent access remains checked because a coordinate transform can map a
 * valid child coordinate outside the parent's logical dimensions.
 */
numerus_matrix_status numerus_matrix_get_unchecked(
    const numerus_matrix *matrix,
    size_t row,
    size_t column,
    double *value
)
{
    if (matrix == NULL || value == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (matrix->parent2 != NULL) {
        numerus_matrix_status status;

        if (matrix->join_type == NUMERUS_MATRIX_JOIN_HORIZONTAL) {
            if (column < numerus_matrix_columns(matrix->parent)) {
                return numerus_matrix_get(
                    matrix->parent, row, column, value
                );
            }

            return numerus_matrix_get(
                matrix->parent2,
                row,
                column - numerus_matrix_columns(matrix->parent),
                value
            );
        }

        if (matrix->join_type == NUMERUS_MATRIX_JOIN_VERTICAL) {
            if (row < numerus_matrix_rows(matrix->parent)) {
                return numerus_matrix_get(
                    matrix->parent, row, column, value
                );
            }

            return numerus_matrix_get(
                matrix->parent2,
                row - numerus_matrix_rows(matrix->parent),
                column,
                value
            );
        }

        status = NUMERUS_MATRIX_INVALID_ARGUMENT;
        return status;
    }

    if (matrix->parent != NULL) {
        size_t parent_row;
        size_t parent_column;
        double parent_value;
        double transformed_value;
        numerus_matrix_status status;

        status = matrix->coordinate_transform(
            row,
            column,
            &parent_row,
            &parent_column,
            matrix->transform_context
        );
        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        status = numerus_matrix_get(
            matrix->parent,
            parent_row,
            parent_column,
            &parent_value
        );
        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        status = matrix->value_transform(
            row,
            column,
            parent_value,
            &transformed_value,
            matrix->transform_context
        );
        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        *value = transformed_value;
        return NUMERUS_MATRIX_SUCCESS;
    }

    *value = numerus_storage_get_unchecked(matrix->storage, row, column);

    return NUMERUS_MATRIX_SUCCESS;
}

/**
 * Validate Matrix coordinates and read the logical value.
 */
numerus_matrix_status numerus_matrix_get(
    const numerus_matrix *matrix,
    size_t row,
    size_t column,
    double *value
)
{
    if (matrix == NULL || value == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (row >= matrix->rows || column >= matrix->columns) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }

    return numerus_matrix_get_unchecked(matrix, row, column, value);
}

size_t numerus_matrix_rows(const numerus_matrix *matrix)
{
    return matrix == NULL ? 0 : matrix->rows;
}

size_t numerus_matrix_columns(const numerus_matrix *matrix)
{
    return matrix == NULL ? 0 : matrix->columns;
}

numerus_storage_kind numerus_matrix_storage_kind(
    const numerus_matrix *matrix
)
{
    if (matrix == NULL) {
        return NUMERUS_STORAGE_ZERO;
    }

    if (matrix->parent != NULL) {
        return numerus_matrix_storage_kind(matrix->parent);
    }

    return numerus_storage_kind_of(matrix->storage);
}

/**
 * Release a Matrix. Parent relationships are non-owning.
 */
void numerus_matrix_destroy(numerus_matrix *matrix)
{
    if (matrix == NULL) {
        return;
    }

    if (matrix->parent == NULL) {
        numerus_storage_destroy(matrix->storage);
    }

    numerus_matrix_free(matrix);
}
