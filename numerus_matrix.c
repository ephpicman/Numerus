#include "numerus_matrix.h"
#include "numerus_numeric.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    NUMERUS_MATRIX_FLAG_SQUARE = UINT32_C(1) << 0,
    NUMERUS_MATRIX_FLAG_ZERO = UINT32_C(1) << 1,
    NUMERUS_MATRIX_FLAG_DIAGONAL = UINT32_C(1) << 2,
    NUMERUS_MATRIX_FLAG_UPPER_TRIANGULAR = UINT32_C(1) << 3,
    NUMERUS_MATRIX_FLAG_LOWER_TRIANGULAR = UINT32_C(1) << 4,
    NUMERUS_MATRIX_FLAG_SYMMETRIC = UINT32_C(1) << 5,
    NUMERUS_MATRIX_FLAG_IDENTITY = UINT32_C(1) << 6,
    NUMERUS_MATRIX_FLAGS_ALL = (UINT32_C(1) << 7) - 1
};

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
    const void *transform_context;
    size_t transform_index1;
    size_t transform_index2;
    numerus_matrix_flags cached_flags;
    uint32_t flags_computed;
    int determinant_state;
    double cached_determinant;
};

static numerus_matrix_status identity_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
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
    const void *context
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
    const void *context
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
    const void *context
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
    const void *context
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

static numerus_matrix_status remove_row_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || view->parent == NULL ||
        parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = row >= view->transform_index1 ? row + 1 : row;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status remove_column_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || view->parent == NULL ||
        parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = row;
    *parent_column = column >= view->transform_index1 ? column + 1 : column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status swap_rows_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || view->parent == NULL ||
        parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (row == view->transform_index1) {
        row = view->transform_index2;
    } else if (row == view->transform_index2) {
        row = view->transform_index1;
    }

    *parent_row = row;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status swap_columns_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || view->parent == NULL ||
        parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (column == view->transform_index1) {
        column = view->transform_index2;
    } else if (column == view->transform_index2) {
        column = view->transform_index1;
    }

    *parent_row = row;
    *parent_column = column;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status rotate_90_clockwise_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
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
    const void *context
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
    const void *context
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
    (*matrix)->transform_index1 = 0;
    (*matrix)->transform_index2 = 0;
    (*matrix)->cached_flags = (numerus_matrix_flags) {0};
    (*matrix)->flags_computed = 0;
    (*matrix)->determinant_state = 0;
    (*matrix)->cached_determinant = 0.0;

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
    const void *context,
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

int numerus_matrix_create_remove_row(
    numerus_matrix *parent,
    size_t row,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (row >= numerus_matrix_rows(parent)) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }
    if (numerus_matrix_rows(parent) == 1) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent) - 1,
        numerus_matrix_columns(parent),
        remove_row_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_index1 = row;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_remove_column(
    numerus_matrix *parent,
    size_t column,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (column >= numerus_matrix_columns(parent)) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }
    if (numerus_matrix_columns(parent) == 1) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent) - 1,
        remove_column_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_index1 = column;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_swap_rows(
    numerus_matrix *parent,
    size_t row1,
    size_t row2,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (row1 >= numerus_matrix_rows(parent) ||
        row2 >= numerus_matrix_rows(parent)) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        swap_rows_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_index1 = row1;
    view->transform_index2 = row2;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_swap_columns(
    numerus_matrix *parent,
    size_t column1,
    size_t column2,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (column1 >= numerus_matrix_columns(parent) ||
        column2 >= numerus_matrix_columns(parent)) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        swap_columns_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_index1 = column1;
    view->transform_index2 = column2;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
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

        return NUMERUS_MATRIX_INVALID_ARGUMENT;
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


numerus_matrix_status numerus_matrix_get_flags(
    numerus_matrix *matrix,
    numerus_matrix_flags *flags
)
{
    numerus_matrix_flags result = {0};
    bool is_zero = true;
    bool is_diagonal = true;
    bool is_upper = true;
    bool is_lower = true;
    bool is_symmetric = true;
    bool is_identity = true;
    size_t row, column;

    if (matrix == NULL || flags == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if ((matrix->flags_computed & NUMERUS_MATRIX_FLAGS_ALL) ==
        NUMERUS_MATRIX_FLAGS_ALL) {
        *flags = matrix->cached_flags;
        return NUMERUS_MATRIX_SUCCESS;
    }

    result.square = matrix->rows == matrix->columns;

    for (row = 0; row < matrix->rows; row++) {
        for (column = 0; column < matrix->columns; column++) {
            double value;
            numerus_matrix_status status = numerus_matrix_get(
                matrix, row, column, &value
            );

            if (status != NUMERUS_MATRIX_SUCCESS) {
                return status;
            }
            if (!numerus_double_is_zero(value)) {
                is_zero = false;
            }
            if (row != column && !numerus_double_is_zero(value)) {
                is_diagonal = false;
            }
            if (row > column && !numerus_double_is_zero(value)) {
                is_upper = false;
            }
            if (row < column && !numerus_double_is_zero(value)) {
                is_lower = false;
            }
            if ((row == column && !numerus_double_is_one(value)) ||
                (row != column && !numerus_double_is_zero(value))) {
                is_identity = false;
            }
            /*
             * Compare each off-diagonal pair once during the shared scan.
             * The previous second pass fetched both values for every pair.
             */
            if (result.square && row < column && is_symmetric) {
                double transposed_value;
                status = numerus_matrix_get(
                    matrix, column, row, &transposed_value
                );
                if (status != NUMERUS_MATRIX_SUCCESS) {
                    return status;
                }
                if (!numerus_double_equals(value, transposed_value)) {
                    is_symmetric = false;
                }
            }
        }
    }

    result.zero = is_zero;
    if (result.square) {
        result.diagonal = is_diagonal;
        result.upper_triangular = is_upper;
        result.lower_triangular = is_lower;
        result.identity = is_identity;
        result.symmetric = is_symmetric;
    }

    matrix->cached_flags = result;
    matrix->flags_computed = NUMERUS_MATRIX_FLAGS_ALL;
    *flags = result;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status matrix_determinant_from_triangular(
    numerus_matrix *matrix,
    double *determinant
)
{
    double result = 1.0;
    size_t index;
    for (index = 0; index < matrix->rows; index++) {
        double diagonal_value;
        numerus_matrix_status status = numerus_matrix_get(
            matrix, index, index, &diagonal_value
        );
        if (status != NUMERUS_MATRIX_SUCCESS) return status;
        result *= diagonal_value;
    }
    *determinant = result;
    return NUMERUS_MATRIX_SUCCESS;
}

numerus_matrix_status numerus_matrix_determinant(
    numerus_matrix *matrix,
    double *determinant
)
{
    size_t size, element_count, row, column;
    double result = 1.0, *values;
    int sign = 1;
    numerus_matrix_status status;

    if (matrix == NULL || determinant == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (matrix->rows != matrix->columns) {
        return NUMERUS_MATRIX_NOT_SQUARE;
    }
    if (matrix->determinant_state == 2) {
        *determinant = matrix->cached_determinant;
        return NUMERUS_MATRIX_SUCCESS;
    }
    if (matrix->determinant_state == 1) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    matrix->determinant_state = 1;
    size = matrix->rows;
    if ((matrix->flags_computed &
        (NUMERUS_MATRIX_FLAG_ZERO | NUMERUS_MATRIX_FLAG_IDENTITY |
         NUMERUS_MATRIX_FLAG_UPPER_TRIANGULAR |
         NUMERUS_MATRIX_FLAG_LOWER_TRIANGULAR)) ==
        (NUMERUS_MATRIX_FLAG_ZERO | NUMERUS_MATRIX_FLAG_IDENTITY |
         NUMERUS_MATRIX_FLAG_UPPER_TRIANGULAR |
         NUMERUS_MATRIX_FLAG_LOWER_TRIANGULAR)) {
        if (matrix->cached_flags.zero) {
            result = 0.0;
            goto cache_result;
        }
        if (matrix->cached_flags.identity) {
            result = 1.0;
            goto cache_result;
        }
        if (matrix->cached_flags.upper_triangular ||
            matrix->cached_flags.lower_triangular) {
            status = matrix_determinant_from_triangular(matrix, &result);
            if (status != NUMERUS_MATRIX_SUCCESS) {
                matrix->determinant_state = 0;
                return status;
            }
            goto cache_result;
        }
    }

    if (size > SIZE_MAX / size) {
        matrix->determinant_state = 0;
        return NUMERUS_MATRIX_OVERFLOW;
    }
    element_count = size * size;
    if (element_count > SIZE_MAX / sizeof(*values)) {
        matrix->determinant_state = 0;
        return NUMERUS_MATRIX_OVERFLOW;
    }
    values = numerus_matrix_alloc(element_count * sizeof(*values));
    if (values == NULL) {
        matrix->determinant_state = 0;
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }

    for (row = 0; row < size; row++) {
        for (column = 0; column < size; column++) {
            status = numerus_matrix_get(
                matrix, row, column, &values[row * size + column]
            );
            if (status != NUMERUS_MATRIX_SUCCESS) {
                numerus_matrix_free(values);
                matrix->determinant_state = 0;
                return status;
            }
        }
    }

    for (column = 0; column < size; column++) {
        size_t pivot_row = column, candidate_row;
        double pivot_magnitude = values[column * size + column];
        if (pivot_magnitude < 0.0) pivot_magnitude = -pivot_magnitude;
        for (candidate_row = column + 1; candidate_row < size; candidate_row++) {
            double magnitude = values[candidate_row * size + column];
            if (magnitude < 0.0) magnitude = -magnitude;
            if (magnitude > pivot_magnitude) {
                pivot_magnitude = magnitude;
                pivot_row = candidate_row;
            }
        }
        if (values[pivot_row * size + column] == 0.0) {
            result = 0.0;
            break;
        }
        if (pivot_row != column) {
            size_t swap_column;
            for (swap_column = 0; swap_column < size; swap_column++) {
                double temporary = values[column * size + swap_column];
                values[column * size + swap_column] =
                    values[pivot_row * size + swap_column];
                values[pivot_row * size + swap_column] = temporary;
            }
            sign = -sign;
        }
        {
            double pivot = values[column * size + column];
            result *= pivot;
            for (row = column + 1; row < size; row++) {
                double factor = values[row * size + column] / pivot;
                for (candidate_row = column + 1;
                     candidate_row < size; candidate_row++) {
                    values[row * size + candidate_row] -=
                        factor * values[column * size + candidate_row];
                }
            }
        }
    }
    numerus_matrix_free(values);
    if (sign < 0) result = -result;

cache_result:
    matrix->cached_determinant = result;
    matrix->determinant_state = 2;
    *determinant = result;
    return NUMERUS_MATRIX_SUCCESS;
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
