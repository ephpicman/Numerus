/**
 * @file numerus_size.h
 * @brief Checked size_t arithmetic shared by Storage and Matrix.
 *
 * Helpers leave the output parameter unchanged when the operation would
 * overflow or the output pointer is NULL.
 */
#ifndef NUMERUS_SIZE_H
#define NUMERUS_SIZE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline bool numerus_size_add(
    size_t left,
    size_t right,
    size_t *result
)
{
    if (result == NULL || right > SIZE_MAX - left) {
        return false;
    }

    *result = left + right;
    return true;
}

static inline bool numerus_size_multiply(
    size_t left,
    size_t right,
    size_t *result
)
{
    if (result == NULL || (left != 0 && right > SIZE_MAX / left)) {
        return false;
    }

    *result = left * right;
    return true;
}

/** Compute n * (n + 1) / 2 without overflowing an intermediate value. */
static inline bool numerus_size_triangular_count(size_t n, size_t *result)
{
    size_t left;
    size_t right;

    if (n % 2 == 0) {
        left = n / 2;
        if (!numerus_size_add(n, 1, &right)) {
            return false;
        }
    } else {
        left = n;
        right = n / 2 + 1;
    }

    return numerus_size_multiply(left, right, result);
}

#endif
