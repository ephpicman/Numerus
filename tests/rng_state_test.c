#include "../numerus_rng.h"

#include <assert.h>
#include <stdio.h>

static void test_reference_vector(void)
{
    static const uint32_t expected[] = {
        UINT32_C(0xa15c02b7),
        UINT32_C(0x7b47f409),
        UINT32_C(0xba1d3330),
        UINT32_C(0x83d2f293),
        UINT32_C(0xbfa4784b),
        UINT32_C(0xcbed606e),
        UINT32_C(0xbfc6a3ad),
        UINT32_C(0x812fff6d)
    };
    numerus_rng *rng = NULL;
    size_t index;

    assert(numerus_rng_create(UINT64_C(42), UINT64_C(54), &rng) ==
        NUMERUS_RNG_SUCCESS);
    assert(rng != NULL);

    for (index = 0; index < sizeof(expected) / sizeof(expected[0]); index++) {
        uint32_t value = 0;

        assert(numerus_rng_next_u32(rng, &value) == NUMERUS_RNG_SUCCESS);
        assert(value == expected[index]);
    }

    numerus_rng_destroy(rng);
}

static void test_replay_and_explicit_state(void)
{
    numerus_rng *first = NULL;
    numerus_rng *second = NULL;
    uint32_t first_value;
    uint32_t second_value;

    assert(numerus_rng_create(123, 456, &first) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_create(123, 456, &second) == NUMERUS_RNG_SUCCESS);

    assert(numerus_rng_next_u32(first, &first_value) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_next_u32(second, &second_value) == NUMERUS_RNG_SUCCESS);
    assert(first_value == second_value);

    numerus_rng_destroy(first);
    numerus_rng_destroy(second);
}

static void test_clone_copies_exact_state_but_not_storage(void)
{
    numerus_rng *source = NULL;
    numerus_rng *clone = NULL;
    uint32_t source_value;
    uint32_t clone_value;

    assert(numerus_rng_create(42, 54, &source) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_next_u32(source, &source_value) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_clone(source, &clone) == NUMERUS_RNG_SUCCESS);
    assert(clone != NULL);

    assert(numerus_rng_next_u32(source, &source_value) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_next_u32(source, &source_value) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_next_u32(clone, &clone_value) == NUMERUS_RNG_SUCCESS);
    assert(source_value != clone_value);

    numerus_rng_destroy(clone);
    numerus_rng_destroy(source);
}

static void test_invalid_arguments_preserve_output_and_state(void)
{
    numerus_rng *rng = NULL;
    uint32_t value = UINT32_C(0xdeadbeef);
    uint32_t next_value;

    assert(numerus_rng_create(42, 54, &rng) == NUMERUS_RNG_SUCCESS);
    assert(numerus_rng_next_u32(NULL, &value) ==
        NUMERUS_RNG_INVALID_ARGUMENT);
    assert(value == UINT32_C(0xdeadbeef));
    assert(numerus_rng_next_u32(rng, NULL) ==
        NUMERUS_RNG_INVALID_ARGUMENT);

    /* Invalid calls must not advance the state: the next output is vector[0]. */
    numerus_rng_destroy(NULL);
    assert(numerus_rng_next_u32(rng, &next_value) == NUMERUS_RNG_SUCCESS);
    assert(next_value == UINT32_C(0xa15c02b7));

    assert(numerus_rng_create(42, 54, NULL) ==
        NUMERUS_RNG_INVALID_ARGUMENT);
    {
        numerus_rng *clone = (void *) 1;
        assert(numerus_rng_clone(NULL, &clone) ==
            NUMERUS_RNG_INVALID_ARGUMENT);
        assert(clone == NULL);
    }
    assert(numerus_rng_clone(NULL, NULL) == NUMERUS_RNG_INVALID_ARGUMENT);
    numerus_rng_destroy(rng);
}

int main(void)
{
    test_reference_vector();
    test_replay_and_explicit_state();
    test_clone_copies_exact_state_but_not_storage();
    test_invalid_arguments_preserve_output_and_state();
    puts("RNG state tests passed.");
    return 0;
}
