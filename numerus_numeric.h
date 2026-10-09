/**
 * @file numerus_numeric.h
 * @brief Shared floating-point comparison helpers for Numerus.
 */
#ifndef NUMERUS_NUMERIC_H
#define NUMERUS_NUMERIC_H

#include <stdbool.h>
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

#endif
