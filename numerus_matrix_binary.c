#include "numerus_matrix_internal.h"

/**
 * Create a lazy element-wise sum.
 *
 * Both parents are borrowed and must outlive the result.
 */
int numerus_matrix_create_add(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_binary_view(
        left,
        right,
        NUMERUS_MATRIX_BINARY_ADD,
        matrix
    );
}

/**
 * Create a lazy element-wise difference.
 *
 * Both parents are borrowed and must outlive the result.
 */
int numerus_matrix_create_subtract(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_binary_view(
        left,
        right,
        NUMERUS_MATRIX_BINARY_SUBTRACT,
        matrix
    );
}

/**
 * Create a lazy Hadamard (element-wise) product.
 *
 * Both parents are borrowed and must outlive the result.
 */
int numerus_matrix_create_hadamard_product(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_binary_view(
        left,
        right,
        NUMERUS_MATRIX_BINARY_HADAMARD,
        matrix
    );
}

/**
 * Create a lazy element-wise quotient.
 *
 * Division follows IEEE-754 double semantics, including infinities and NaN
 * for zero divisors. Both parents are borrowed and must outlive the result.
 */
int numerus_matrix_create_divide(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
)
{
    return numerus_matrix_create_binary_view(
        left,
        right,
        NUMERUS_MATRIX_BINARY_DIVIDE,
        matrix
    );
}
