#ifndef NUMERUS_RNG_H
#define NUMERUS_RNG_H

#include <stdint.h>

/**
 * @brief Status values for the internal Numerus RNG API.
 */
typedef enum {
    NUMERUS_RNG_SUCCESS = 0,
    NUMERUS_RNG_INVALID_ARGUMENT,
    NUMERUS_RNG_OUT_OF_MEMORY
} numerus_rng_status;

/** Opaque mutable PCG-XSH-RR 64/32 state. */
typedef struct numerus_rng numerus_rng;

/**
 * Create a PCG32 state from explicit 64-bit seed and stream selector.
 *
 * Uses the reference PCG seeding procedure. The output handle is set to NULL
 * before allocation and remains NULL on failure.
 */
numerus_rng_status numerus_rng_create(
    uint64_t seed,
    uint64_t stream,
    numerus_rng **rng
);

/**
 * Advance one state transition and return the next exact 32-bit output.
 * On failure, the state and output value are unchanged.
 */
numerus_rng_status numerus_rng_next_u32(
    numerus_rng *rng,
    uint32_t *value
);

/**
 * Clone the exact state and stream increment. Advancing either object does
 * not change the other. The output handle is NULL on failure.
 */
numerus_rng_status numerus_rng_clone(
    const numerus_rng *source,
    numerus_rng **clone
);

/** Destroy one state; accepts NULL. */
void numerus_rng_destroy(numerus_rng *rng);

#endif
