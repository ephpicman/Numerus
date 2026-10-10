#ifndef NUMERUS_PROBABILITY_H
#define NUMERUS_PROBABILITY_H

#include "numerus_matrix.h"

/** Status values for reusable probability primitives. */
typedef enum {
    NUMERUS_PROBABILITY_SUCCESS = 0,
    NUMERUS_PROBABILITY_INVALID_ARGUMENT,
    NUMERUS_PROBABILITY_DIMENSION_MISMATCH,
    NUMERUS_PROBABILITY_NON_FINITE_INPUT,
    NUMERUS_PROBABILITY_NOT_POSITIVE_DEFINITE,
    NUMERUS_PROBABILITY_SIZE_OVERFLOW,
    NUMERUS_PROBABILITY_OUT_OF_MEMORY,
    NUMERUS_PROBABILITY_NUMERICAL_FAILURE
} numerus_probability_status;

/**
 * Compute the multivariate Gaussian log density for column-vector observation
 * and mean with a symmetric positive-definite covariance Matrix.
 *
 * Uses Cholesky factorization, a triangular solve and signed log determinant;
 * never forms the covariance inverse. Inputs are borrowed and unchanged.
 * The scalar output remains unchanged on every failure.
 */
numerus_probability_status numerus_multivariate_gaussian_log_density(
    const numerus_matrix *observation,
    const numerus_matrix *mean,
    const numerus_matrix *covariance,
    double *log_density
);

#endif
