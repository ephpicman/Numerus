#include "numerus_rng.h"

#include <stdlib.h>

#if defined(NUMERUS_RNG_TEST_ALLOCATOR)
void *numerus_rng_test_alloc(size_t size);
void numerus_rng_test_free(void *pointer);
# define numerus_rng_alloc(size) numerus_rng_test_alloc(size)
# define numerus_rng_free(pointer) numerus_rng_test_free(pointer)
#elif !defined(NUMERUS_RNG_USE_LIBC_ALLOC)
# include "php.h"
# include "Zend/zend_alloc.h"
# define numerus_rng_alloc(size) emalloc(size)
# define numerus_rng_free(pointer) efree(pointer)
#else
# define numerus_rng_alloc(size) malloc(size)
# define numerus_rng_free(pointer) free(pointer)
#endif

#define NUMERUS_PCG32_MULTIPLIER UINT64_C(6364136223846793005)

struct numerus_rng {
    uint64_t state;
    uint64_t increment;
};

/*
 * PCG-XSH-RR 64/32 reference transition and output permutation.
 * Unsigned uint64_t wraparound is part of the specified algorithm.
 */
static uint32_t numerus_rng_next_u32_unchecked(numerus_rng *rng)
{
    uint64_t old_state = rng->state;
    uint32_t xorshifted;
    uint32_t rotation;

    rng->state = old_state * NUMERUS_PCG32_MULTIPLIER + rng->increment;
    xorshifted = (uint32_t) (((old_state >> 18u) ^ old_state) >> 27u);
    rotation = (uint32_t) (old_state >> 59u);

    return (xorshifted >> rotation) |
        (xorshifted << ((0u - rotation) & 31u));
}

static void numerus_rng_seed_unchecked(
    numerus_rng *rng,
    uint64_t seed,
    uint64_t stream
)
{
    rng->state = 0u;
    rng->increment = (stream << 1u) | 1u;

    (void) numerus_rng_next_u32_unchecked(rng);
    rng->state += seed;
    (void) numerus_rng_next_u32_unchecked(rng);
}

numerus_rng_status numerus_rng_create(
    uint64_t seed,
    uint64_t stream,
    numerus_rng **rng
)
{
    numerus_rng *result;

    if (rng == NULL) {
        return NUMERUS_RNG_INVALID_ARGUMENT;
    }
    *rng = NULL;

    result = numerus_rng_alloc(sizeof(*result));
    if (result == NULL) {
        return NUMERUS_RNG_OUT_OF_MEMORY;
    }

    numerus_rng_seed_unchecked(result, seed, stream);
    *rng = result;
    return NUMERUS_RNG_SUCCESS;
}

numerus_rng_status numerus_rng_next_u32(
    numerus_rng *rng,
    uint32_t *value
)
{
    uint32_t result;

    if (rng == NULL || value == NULL) {
        return NUMERUS_RNG_INVALID_ARGUMENT;
    }

    result = numerus_rng_next_u32_unchecked(rng);
    *value = result;
    return NUMERUS_RNG_SUCCESS;
}

numerus_rng_status numerus_rng_clone(
    const numerus_rng *source,
    numerus_rng **clone
)
{
    numerus_rng *result;

    if (clone == NULL) {
        return NUMERUS_RNG_INVALID_ARGUMENT;
    }
    *clone = NULL;

    if (source == NULL) {
        return NUMERUS_RNG_INVALID_ARGUMENT;
    }

    result = numerus_rng_alloc(sizeof(*result));
    if (result == NULL) {
        return NUMERUS_RNG_OUT_OF_MEMORY;
    }

    result->state = source->state;
    result->increment = source->increment;
    *clone = result;
    return NUMERUS_RNG_SUCCESS;
}

void numerus_rng_destroy(numerus_rng *rng)
{
    if (rng != NULL) {
        numerus_rng_free(rng);
    }
}
