/**
 * @file numerus_matrix_internal.h
 * @brief Private helpers shared by Matrix implementation units.
 */
#ifndef NUMERUS_MATRIX_INTERNAL_H
#define NUMERUS_MATRIX_INTERNAL_H

#include "numerus_matrix.h"

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
