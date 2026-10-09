#include "numerus_matrix_internal.h"
#include "numerus_size.h"
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
    numerus_matrix_binary_operation binary_operation;
    numerus_coordinate_transform_fn coordinate_transform;
    numerus_value_transform_fn value_transform;
    const void *transform_context;
    size_t transform_index1;
    size_t transform_index2;
    numerus_matrix_flags cached_flags;
    uint32_t flags_computed;
    int determinant_state;
    double cached_determinant;
    double transform_scalar;
    size_t transform_row_offset;
    size_t transform_column_offset;
    size_t transform_parent_columns;
    size_t *selection_indices;
    numerus_matrix_selection_axis selection_axis;
    bool padding_view;
    size_t padding_top;
    size_t padding_left;
    double padding_value;
    bool repeat_view;
    bool block_diagonal_view;
    bool block_grid_view;
    numerus_matrix **block_matrices;
    size_t *block_row_offsets;
    size_t *block_column_offsets;
    size_t block_row_count;
    size_t block_column_count;
    bool diagonal_matrix_view;
    bool diagonal_offset_positive;
    size_t diagonal_offset_magnitude;
    size_t diagonal_vector_length;
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

static numerus_matrix_status scale_value_transform(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    const void *context
)
{
    const double *scalar = context;

    (void) row;
    (void) column;

    if (scalar == NULL || result == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *result = parent_value * *scalar;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status slice_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = view->transform_row_offset + row;
    *parent_column = view->transform_column_offset + column;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status diagonal_extract_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    (void) row;

    if (view == NULL || parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *parent_row = view->transform_row_offset + column;
    *parent_column = view->transform_column_offset + column;
    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status selection_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;

    if (view == NULL || view->selection_indices == NULL ||
        parent_row == NULL || parent_column == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (view->selection_axis == NUMERUS_MATRIX_SELECTION_ROWS) {
        *parent_row = view->selection_indices[row];
        *parent_column = column;
        return NUMERUS_MATRIX_SUCCESS;
    }

    if (view->selection_axis == NUMERUS_MATRIX_SELECTION_COLUMNS) {
        *parent_row = row;
        *parent_column = view->selection_indices[column];
        return NUMERUS_MATRIX_SUCCESS;
    }

    return NUMERUS_MATRIX_INVALID_ARGUMENT;
}

static numerus_matrix_status reshape_coordinate_transform(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
)
{
    const numerus_matrix *view = context;
    size_t linear_index;

    if (view == NULL || parent_row == NULL || parent_column == NULL ||
        view->transform_parent_columns == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    /*
     * Construction verifies rows * columns is representable. Coordinates
     * reaching this callback are within those dimensions, so this linear
     * index is bounded by the validated logical element count.
     */
    linear_index = row * view->columns + column;
    *parent_row = linear_index / view->transform_parent_columns;
    *parent_column = linear_index % view->transform_parent_columns;

    return NUMERUS_MATRIX_SUCCESS;
}

static numerus_matrix_status divide_scalar_value_transform(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    const void *context
)
{
    const double *divisor = context;

    (void) row;
    (void) column;

    if (divisor == NULL || result == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    *result = parent_value / *divisor;
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
    (*matrix)->binary_operation = NUMERUS_MATRIX_BINARY_NONE;
    (*matrix)->coordinate_transform = NULL;
    (*matrix)->value_transform = NULL;
    (*matrix)->transform_context = NULL;
    (*matrix)->transform_index1 = 0;
    (*matrix)->transform_index2 = 0;
    (*matrix)->cached_flags = (numerus_matrix_flags) {0};
    (*matrix)->flags_computed = 0;
    (*matrix)->determinant_state = 0;
    (*matrix)->cached_determinant = 0.0;
    (*matrix)->transform_scalar = 0.0;
    (*matrix)->transform_row_offset = 0;
    (*matrix)->transform_column_offset = 0;
    (*matrix)->transform_parent_columns = 0;
    (*matrix)->selection_indices = NULL;
    (*matrix)->selection_axis = NUMERUS_MATRIX_SELECTION_NONE;
    (*matrix)->padding_view = false;
    (*matrix)->padding_top = 0;
    (*matrix)->padding_left = 0;
    (*matrix)->padding_value = 0.0;
    (*matrix)->repeat_view = false;
    (*matrix)->block_diagonal_view = false;
    (*matrix)->block_grid_view = false;
    (*matrix)->block_matrices = NULL;
    (*matrix)->block_row_offsets = NULL;
    (*matrix)->block_column_offsets = NULL;
    (*matrix)->block_row_count = 0;
    (*matrix)->block_column_count = 0;
    (*matrix)->diagonal_matrix_view = false;
    (*matrix)->diagonal_offset_positive = true;
    (*matrix)->diagonal_offset_magnitude = 0;
    (*matrix)->diagonal_vector_length = 0;

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

int numerus_matrix_create_scale(
    numerus_matrix *parent,
    double scalar,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        NULL,
        scale_value_transform,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    /*
     * The callback context points into the stable, heap-allocated view.
     * This avoids borrowing a stack scalar or allocating a separate context.
     */
    view->transform_scalar = scalar;
    view->transform_context = &view->transform_scalar;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_negate(
    numerus_matrix *parent,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_scale(parent, -1.0, matrix);
}

int numerus_matrix_create_selection_view(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix_selection_axis axis,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t *owned_indices;
    size_t index_bytes;
    size_t parent_rows;
    size_t parent_columns;
    size_t index;
    size_t selected_limit;
    size_t view_rows;
    size_t view_columns;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || indices == NULL || count == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (axis != NUMERUS_MATRIX_SELECTION_ROWS &&
        axis != NUMERUS_MATRIX_SELECTION_COLUMNS) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (!numerus_size_multiply(count, sizeof(*owned_indices), &index_bytes)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    parent_rows = numerus_matrix_rows(parent);
    parent_columns = numerus_matrix_columns(parent);
    selected_limit = axis == NUMERUS_MATRIX_SELECTION_ROWS
        ? parent_rows
        : parent_columns;

    for (index = 0; index < count; index++) {
        if (indices[index] >= selected_limit) {
            return NUMERUS_MATRIX_OUT_OF_BOUNDS;
        }
    }

    owned_indices = numerus_matrix_alloc(index_bytes);
    if (owned_indices == NULL) {
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }
    for (index = 0; index < count; index++) {
        owned_indices[index] = indices[index];
    }

    view_rows = axis == NUMERUS_MATRIX_SELECTION_ROWS
        ? count
        : parent_rows;
    view_columns = axis == NUMERUS_MATRIX_SELECTION_COLUMNS
        ? count
        : parent_columns;

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        view_rows,
        view_columns,
        selection_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_free(owned_indices);
        return status;
    }

    view->selection_indices = owned_indices;
    view->selection_axis = axis;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_reshape_view(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t parent_count;
    size_t view_count;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || rows == 0 || columns == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            numerus_matrix_rows(parent),
            numerus_matrix_columns(parent),
            &parent_count
        ) ||
        !numerus_size_multiply(rows, columns, &view_count)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    if (parent_count != view_count) {
        return NUMERUS_MATRIX_DIMENSION_MISMATCH;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        rows,
        columns,
        reshape_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_parent_columns = numerus_matrix_columns(parent);
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_padding_view(
    numerus_matrix *parent,
    size_t top,
    size_t bottom,
    size_t left,
    size_t right,
    double value,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t rows_with_top;
    size_t rows;
    size_t columns_with_left;
    size_t columns;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_add(numerus_matrix_rows(parent), top, &rows_with_top) ||
        !numerus_size_add(rows_with_top, bottom, &rows) ||
        !numerus_size_add(numerus_matrix_columns(parent), left, &columns_with_left) ||
        !numerus_size_add(columns_with_left, right, &columns)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent, rows, columns, NULL, NULL, NULL, &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->padding_view = true;
    view->padding_top = top;
    view->padding_left = left;
    view->padding_value = value;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_repeat_view(
    numerus_matrix *parent,
    size_t row_repetitions,
    size_t column_repetitions,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t rows;
    size_t columns;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || row_repetitions == 0 || column_repetitions == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            numerus_matrix_rows(parent), row_repetitions, &rows
        ) ||
        !numerus_size_multiply(
            numerus_matrix_columns(parent), column_repetitions, &columns
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent, rows, columns, NULL, NULL, NULL, &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->repeat_view = true;
    *matrix = view;
    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_block_diagonal_view(
    numerus_matrix *parent,
    size_t repetitions,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t rows;
    size_t columns;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || repetitions == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            numerus_matrix_rows(parent), repetitions, &rows
        ) ||
        !numerus_size_multiply(
            numerus_matrix_columns(parent), repetitions, &columns
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent, rows, columns, NULL, NULL, NULL, &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->block_diagonal_view = true;
    *matrix = view;
    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_block_grid_view(
    numerus_matrix *const *blocks,
    size_t block_row_count,
    size_t block_column_count,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t block_count;
    size_t block_bytes;
    size_t row_offset_count;
    size_t row_offset_bytes;
    size_t column_offset_count;
    size_t column_offset_bytes;
    size_t total_rows = 0;
    size_t total_columns = 0;
    size_t row;
    size_t column;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (blocks == NULL || block_row_count == 0 || block_column_count == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (!numerus_size_multiply(
            block_row_count, block_column_count, &block_count
        ) ||
        !numerus_size_multiply(
            block_count, sizeof(*view->block_matrices), &block_bytes
        ) ||
        !numerus_size_add(block_row_count, 1, &row_offset_count) ||
        !numerus_size_multiply(
            row_offset_count, sizeof(*view->block_row_offsets), &row_offset_bytes
        ) ||
        !numerus_size_add(block_column_count, 1, &column_offset_count) ||
        !numerus_size_multiply(
            column_offset_count, sizeof(*view->block_column_offsets), &column_offset_bytes
        )) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    for (row = 0; row < block_row_count; row++) {
        size_t block_height;
        size_t next_total;

        if (blocks[row * block_column_count] == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        block_height = numerus_matrix_rows(blocks[row * block_column_count]);
        for (column = 1; column < block_column_count; column++) {
            numerus_matrix *block = blocks[row * block_column_count + column];

            if (block == NULL) {
                return NUMERUS_MATRIX_INVALID_ARGUMENT;
            }
            if (numerus_matrix_rows(block) != block_height) {
                return NUMERUS_MATRIX_DIMENSION_MISMATCH;
            }
        }

        if (!numerus_size_add(total_rows, block_height, &next_total)) {
            return NUMERUS_MATRIX_OVERFLOW;
        }
        total_rows = next_total;
    }

    for (column = 0; column < block_column_count; column++) {
        size_t block_width;
        size_t next_total;

        if (blocks[column] == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        block_width = numerus_matrix_columns(blocks[column]);
        for (row = 1; row < block_row_count; row++) {
            numerus_matrix *block = blocks[row * block_column_count + column];

            if (block == NULL) {
                return NUMERUS_MATRIX_INVALID_ARGUMENT;
            }
            if (numerus_matrix_columns(block) != block_width) {
                return NUMERUS_MATRIX_DIMENSION_MISMATCH;
            }
        }

        if (!numerus_size_add(total_columns, block_width, &next_total)) {
            return NUMERUS_MATRIX_OVERFLOW;
        }
        total_columns = next_total;
    }

    status = allocate_matrix(&view);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->block_matrices = numerus_matrix_alloc(block_bytes);
    view->block_row_offsets = numerus_matrix_alloc(row_offset_bytes);
    view->block_column_offsets = numerus_matrix_alloc(column_offset_bytes);

    if (view->block_matrices == NULL || view->block_row_offsets == NULL ||
        view->block_column_offsets == NULL) {
        numerus_matrix_free(view->block_matrices);
        numerus_matrix_free(view->block_row_offsets);
        numerus_matrix_free(view->block_column_offsets);
        numerus_matrix_free(view);
        return NUMERUS_MATRIX_OUT_OF_MEMORY;
    }

    for (row = 0; row < block_count; row++) {
        view->block_matrices[row] = blocks[row];
    }

    view->block_row_offsets[0] = 0;
    for (row = 0; row < block_row_count; row++) {
        view->block_row_offsets[row + 1] =
            view->block_row_offsets[row] +
            numerus_matrix_rows(blocks[row * block_column_count]);
    }

    view->block_column_offsets[0] = 0;
    for (column = 0; column < block_column_count; column++) {
        view->block_column_offsets[column + 1] =
            view->block_column_offsets[column] +
            numerus_matrix_columns(blocks[column]);
    }

    view->rows = total_rows;
    view->columns = total_columns;
    view->block_grid_view = true;
    view->block_row_count = block_row_count;
    view->block_column_count = block_column_count;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_diagonal_extract_view(
    numerus_matrix *parent,
    ptrdiff_t offset,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t parent_rows;
    size_t parent_columns;
    size_t row_offset = 0;
    size_t column_offset = 0;
    size_t length;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    parent_rows = numerus_matrix_rows(parent);
    parent_columns = numerus_matrix_columns(parent);

    if (offset >= 0) {
        column_offset = (size_t) offset;
        if (column_offset >= parent_columns) {
            return NUMERUS_MATRIX_OUT_OF_BOUNDS;
        }
        length = parent_columns - column_offset;
        if (length > parent_rows) {
            length = parent_rows;
        }
    } else {
        row_offset = (size_t) (-(offset + 1)) + 1;
        if (row_offset >= parent_rows) {
            return NUMERUS_MATRIX_OUT_OF_BOUNDS;
        }
        length = parent_rows - row_offset;
        if (length > parent_columns) {
            length = parent_columns;
        }
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent, 1, length, diagonal_extract_coordinate_transform,
        NULL, NULL, &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_row_offset = row_offset;
    view->transform_column_offset = column_offset;
    view->transform_context = view;
    *matrix = view;
    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_diagonal_from_vector_view(
    numerus_matrix *vector,
    ptrdiff_t offset,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t vector_length;
    size_t offset_magnitude;
    size_t size;
    size_t element_count;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (vector == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (numerus_matrix_rows(vector) == 1) {
        vector_length = numerus_matrix_columns(vector);
    } else if (numerus_matrix_columns(vector) == 1) {
        vector_length = numerus_matrix_rows(vector);
    } else {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    offset_magnitude = offset >= 0
        ? (size_t) offset
        : (size_t) (-(offset + 1)) + 1;

    if (!numerus_size_add(vector_length, offset_magnitude, &size) ||
        !numerus_size_multiply(size, size, &element_count)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        vector, size, size, NULL, NULL, NULL, &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->diagonal_matrix_view = true;
    view->diagonal_offset_positive = offset >= 0;
    view->diagonal_offset_magnitude = offset_magnitude;
    view->diagonal_vector_length = vector_length;
    *matrix = view;
    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_slice_view(
    numerus_matrix *parent,
    size_t row_start,
    size_t row_count,
    size_t column_start,
    size_t column_count,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    size_t parent_rows;
    size_t parent_columns;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL || row_count == 0 || column_count == 0) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    parent_rows = numerus_matrix_rows(parent);
    parent_columns = numerus_matrix_columns(parent);

    if (row_start > parent_rows || column_start > parent_columns) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }
    if (row_count > parent_rows - row_start ||
        column_count > parent_columns - column_start) {
        return NUMERUS_MATRIX_OUT_OF_BOUNDS;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        row_count,
        column_count,
        slice_coordinate_transform,
        NULL,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_row_offset = row_start;
    view->transform_column_offset = column_start;
    view->transform_context = view;
    *matrix = view;

    return NUMERUS_MATRIX_SUCCESS;
}

int numerus_matrix_create_divide_scalar(
    numerus_matrix *parent,
    double divisor,
    numerus_matrix **matrix
)
{
    numerus_matrix *view = NULL;
    numerus_matrix_status status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (parent == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (divisor == 0.0) {
        return NUMERUS_MATRIX_DIVISION_BY_ZERO;
    }

    status = numerus_matrix_create_from_parent_with_transforms(
        parent,
        numerus_matrix_rows(parent),
        numerus_matrix_columns(parent),
        NULL,
        divide_scalar_value_transform,
        NULL,
        &view
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    view->transform_scalar = divisor;
    view->transform_context = &view->transform_scalar;
    *matrix = view;

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

int numerus_matrix_create_binary_view(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix_binary_operation operation,
    numerus_matrix **matrix
)
{
    numerus_matrix *result = NULL;
    int status;

    if (matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    *matrix = NULL;

    if (left == NULL || right == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (operation != NUMERUS_MATRIX_BINARY_ADD &&
        operation != NUMERUS_MATRIX_BINARY_SUBTRACT &&
        operation != NUMERUS_MATRIX_BINARY_HADAMARD &&
        operation != NUMERUS_MATRIX_BINARY_DIVIDE) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }
    if (numerus_matrix_rows(left) != numerus_matrix_rows(right) ||
        numerus_matrix_columns(left) != numerus_matrix_columns(right)) {
        return NUMERUS_MATRIX_DIMENSION_MISMATCH;
    }

    status = allocate_matrix(&result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        return status;
    }

    result->rows = numerus_matrix_rows(left);
    result->columns = numerus_matrix_columns(left);
    result->storage = NULL;
    result->parent = left;
    result->parent2 = right;
    result->binary_operation = operation;
    result->coordinate_transform = NULL;
    result->value_transform = NULL;
    result->transform_context = NULL;
    *matrix = result;

    return NUMERUS_MATRIX_SUCCESS;
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
    result->binary_operation = NUMERUS_MATRIX_BINARY_NONE;
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

    if (matrix->padding_view) {
        size_t parent_row;
        size_t parent_column;

        if (matrix->parent == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        if (row < matrix->padding_top || column < matrix->padding_left) {
            *value = matrix->padding_value;
            return NUMERUS_MATRIX_SUCCESS;
        }

        parent_row = row - matrix->padding_top;
        parent_column = column - matrix->padding_left;

        if (parent_row >= numerus_matrix_rows(matrix->parent) ||
            parent_column >= numerus_matrix_columns(matrix->parent)) {
            *value = matrix->padding_value;
            return NUMERUS_MATRIX_SUCCESS;
        }

        return numerus_matrix_get(
            matrix->parent, parent_row, parent_column, value
        );
    }

    if (matrix->repeat_view) {
        size_t parent_rows;
        size_t parent_columns;

        if (matrix->parent == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        parent_rows = numerus_matrix_rows(matrix->parent);
        parent_columns = numerus_matrix_columns(matrix->parent);
        return numerus_matrix_get(
            matrix->parent, row % parent_rows, column % parent_columns, value
        );
    }

    if (matrix->block_diagonal_view) {
        size_t parent_rows;
        size_t parent_columns;
        size_t block_row;
        size_t block_column;

        if (matrix->parent == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        parent_rows = numerus_matrix_rows(matrix->parent);
        parent_columns = numerus_matrix_columns(matrix->parent);
        block_row = row / parent_rows;
        block_column = column / parent_columns;

        if (block_row != block_column) {
            *value = 0.0;
            return NUMERUS_MATRIX_SUCCESS;
        }

        return numerus_matrix_get(
            matrix->parent, row % parent_rows, column % parent_columns, value
        );
    }

    if (matrix->diagonal_matrix_view) {
        size_t diagonal_index;

        if (matrix->parent == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        if (matrix->diagonal_offset_positive) {
            if (column < row ||
                column - row != matrix->diagonal_offset_magnitude) {
                *value = 0.0;
                return NUMERUS_MATRIX_SUCCESS;
            }
            diagonal_index = row;
        } else {
            if (row < column ||
                row - column != matrix->diagonal_offset_magnitude) {
                *value = 0.0;
                return NUMERUS_MATRIX_SUCCESS;
            }
            diagonal_index = column;
        }

        if (diagonal_index >= matrix->diagonal_vector_length) {
            *value = 0.0;
            return NUMERUS_MATRIX_SUCCESS;
        }

        if (numerus_matrix_rows(matrix->parent) == 1) {
            return numerus_matrix_get(
                matrix->parent, 0, diagonal_index, value
            );
        }
        return numerus_matrix_get(
            matrix->parent, diagonal_index, 0, value
        );
    }

    if (matrix->block_grid_view) {
        size_t block_row = 0;
        size_t block_column = 0;
        size_t local_row;
        size_t local_column;
        numerus_matrix *block;

        if (matrix->block_matrices == NULL ||
            matrix->block_row_offsets == NULL ||
            matrix->block_column_offsets == NULL) {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        while (block_row + 1 < matrix->block_row_count &&
            row >= matrix->block_row_offsets[block_row + 1]) {
            block_row++;
        }
        while (block_column + 1 < matrix->block_column_count &&
            column >= matrix->block_column_offsets[block_column + 1]) {
            block_column++;
        }

        local_row = row - matrix->block_row_offsets[block_row];
        local_column = column - matrix->block_column_offsets[block_column];
        block = matrix->block_matrices[
            block_row * matrix->block_column_count + block_column
        ];

        return numerus_matrix_get(block, local_row, local_column, value);
    }

    if (matrix->parent2 != NULL &&
        matrix->binary_operation != NUMERUS_MATRIX_BINARY_NONE) {
        double left_value;
        double right_value;
        double result_value;
        numerus_matrix_status status;

        status = numerus_matrix_get(
            matrix->parent, row, column, &left_value
        );
        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        status = numerus_matrix_get(
            matrix->parent2, row, column, &right_value
        );
        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        if (matrix->binary_operation == NUMERUS_MATRIX_BINARY_ADD) {
            result_value = left_value + right_value;
        } else if (matrix->binary_operation == NUMERUS_MATRIX_BINARY_SUBTRACT) {
            result_value = left_value - right_value;
        } else if (matrix->binary_operation == NUMERUS_MATRIX_BINARY_HADAMARD) {
            result_value = left_value * right_value;
        } else if (matrix->binary_operation == NUMERUS_MATRIX_BINARY_DIVIDE) {
            result_value = left_value / right_value;
        } else {
            return NUMERUS_MATRIX_INVALID_ARGUMENT;
        }

        *value = result_value;
        return NUMERUS_MATRIX_SUCCESS;
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
    size_t size, element_count, allocation_size, row, column;
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

    if (!numerus_size_multiply(size, size, &element_count) ||
        !numerus_size_multiply(element_count, sizeof(*values), &allocation_size)) {
        matrix->determinant_state = 0;
        return NUMERUS_MATRIX_OVERFLOW;
    }
    values = numerus_matrix_alloc(allocation_size);
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

    numerus_matrix_free(matrix->selection_indices);
    numerus_matrix_free(matrix->block_matrices);
    numerus_matrix_free(matrix->block_row_offsets);
    numerus_matrix_free(matrix->block_column_offsets);
    numerus_matrix_free(matrix);
}
