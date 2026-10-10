/**
 * @file numerus_numeric.h
 * @brief Shared floating-point comparison helpers for Numerus.
 */
#ifndef NUMERUS_NUMERIC_H
#define NUMERUS_NUMERIC_H

#include <stdbool.h>
#include <stddef.h>
#include <math.h>

/**
 * Default absolute/relative tolerance used for approximate comparisons.
 *
 * This is an application-level tolerance, not the machine epsilon of double.
 */
#define NUMERUS_EPSILON 1e-9

/**
 * Compare two finite values with a combined absolute/relative tolerance.
 *
 * Exact equality is handled first so equal infinities compare equal; NaN
 * compares unequal to every value, including itself.
 */
static inline bool numerus_double_equals(double left, double right)
{
    double left_magnitude;
    double right_magnitude;
    double scale;

    if (left == right) {
        return true;
    }

    if (!isfinite(left) || !isfinite(right)) {
        return false;
    }

    left_magnitude = fabs(left);
    right_magnitude = fabs(right);
    scale = left_magnitude > right_magnitude
        ? left_magnitude
        : right_magnitude;

    if (scale < 1.0) {
        scale = 1.0;
    }

    return fabs(left - right) <= NUMERUS_EPSILON * scale;
}

/** Return whether a value is within NUMERUS_EPSILON of zero. */
static inline bool numerus_double_is_zero(double value)
{
    return isfinite(value) && fabs(value) <= NUMERUS_EPSILON;
}

/** Return whether a value is within NUMERUS_EPSILON of one. */
static inline bool numerus_double_is_one(double value)
{
    return numerus_double_equals(value, 1.0);
}


/** Status values for stable scalar and log-domain operations. */
typedef enum {
    NUMERUS_NUMERIC_SUCCESS = 0,
    NUMERUS_NUMERIC_INVALID_ARGUMENT,
    NUMERUS_NUMERIC_DOMAIN_ERROR,
    NUMERUS_NUMERIC_NON_FINITE,
    NUMERUS_NUMERIC_NUMERICAL_FAILURE
} numerus_numeric_status;

/** Compute a stable sigmoid for a finite input; output is unchanged on failure. */
numerus_numeric_status numerus_numeric_sigmoid(double input, double *result);

/** Compute log(p / (1-p)) for finite 0 < p < 1. */
numerus_numeric_status numerus_numeric_logit(double probability, double *result);

/** Compute log(1 + exp(x)) without avoidable overflow. */
numerus_numeric_status numerus_numeric_softplus(double input, double *result);

/** Compute log(sigmoid(x)) without taking log of a rounded sigmoid result. */
numerus_numeric_status numerus_numeric_log_sigmoid(double input, double *result);

/**
 * Compute log(sum(exp(values[i]))) for a non-empty array of finite values.
 * Uses max-shifting and does not allocate. Output is unchanged on failure.
 */
numerus_numeric_status numerus_numeric_log_sum_exp(
    const double *values,
    size_t count,
    double *result
);


#endif
