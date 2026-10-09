/**
 * @file numerus_matrix_internal.h
 * @brief Private helpers shared by Matrix implementation units.
 */
#ifndef NUMERUS_MATRIX_INTERNAL_H
#define NUMERUS_MATRIX_INTERNAL_H

#include "numerus_matrix.h"

typedef enum {
    NUMERUS_MATRIX_SELECTION_NONE = 0,
    NUMERUS_MATRIX_SELECTION_ROWS,
    NUMERUS_MATRIX_SELECTION_COLUMNS
} numerus_matrix_selection_axis;

typedef enum {
    NUMERUS_MATRIX_BINARY_NONE = 0,
    NUMERUS_MATRIX_BINARY_ADD,
    NUMERUS_MATRIX_BINARY_SUBTRACT,
    NUMERUS_MATRIX_BINARY_HADAMARD,
    NUMERUS_MATRIX_BINARY_DIVIDE
} numerus_matrix_binary_operation;

/**
 * Create a lazy two-parent element-wise node.
 *
 * This is an implementation helper, not part of the supported Matrix API.
 */
int numerus_matrix_create_binary_view(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix_binary_operation operation,
    numerus_matrix **matrix
);

/** Private checked constructor for a row-major reshape view. */
int numerus_matrix_create_reshape_view(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
);

/** Private checked constructor for a lazy index-selection view. */
int numerus_matrix_create_selection_view(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix_selection_axis axis,
    numerus_matrix **matrix
);

/** Private checked constructor for a constant-padded view. */
int numerus_matrix_create_padding_view(
    numerus_matrix *parent,
    size_t top,
    size_t bottom,
    size_t left,
    size_t right,
    double value,
    numerus_matrix **matrix
);

/** Private checked constructor for a tiled repetition view. */
int numerus_matrix_create_repeat_view(
    numerus_matrix *parent,
    size_t row_repetitions,
    size_t column_repetitions,
    numerus_matrix **matrix
);

/** Private checked constructor for a repeated block-diagonal view. */
int numerus_matrix_create_block_diagonal_view(
    numerus_matrix *parent,
    size_t repetitions,
    numerus_matrix **matrix
);

/** Private checked constructor for a general non-empty block grid. */
int numerus_matrix_create_block_grid_view(
    numerus_matrix *const *blocks,
    size_t block_row_count,
    size_t block_column_count,
    numerus_matrix **matrix
);

/** Private checked constructor for a full row/column permutation view. */
int numerus_matrix_create_permutation_view(
    numerus_matrix *parent,
    const size_t *permutation,
    size_t count,
    numerus_matrix_selection_axis axis,
    numerus_matrix **matrix
);

/** Private checked constructor for a row-vector diagonal extraction view. */
int numerus_matrix_create_diagonal_extract_view(
    numerus_matrix *parent,
    ptrdiff_t offset,
    numerus_matrix **matrix
);

/** Private checked constructor for a square diagonal view from a vector. */
int numerus_matrix_create_diagonal_from_vector_view(
    numerus_matrix *vector,
    ptrdiff_t offset,
    numerus_matrix **matrix
);

/** Private checked constructor for a lazy range slice. */
int numerus_matrix_create_slice_view(
    numerus_matrix *parent,
    size_t row_start,
    size_t row_count,
    size_t column_start,
    size_t column_count,
    numerus_matrix **matrix
);

#endif
