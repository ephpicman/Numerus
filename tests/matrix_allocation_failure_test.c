/**
 * @file matrix_allocation_failure_test.c
 * @brief Native tests for allocation-failure injection, cleanup, and output-state guarantees.
 *
 * @details These native tests define regression coverage for the named Matrix
 * contract. Assertions are executable specifications: intentional behavior
 * changes should update these checks together with the corresponding API
 * documentation. This file is test support, not runtime code.
 */

#include "../numerus_matrix.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);

static long allocations_before_failure = -1;

static int should_fail_allocation(void)
{
    if (allocations_before_failure < 0) {
        return 0;
    }
    if (allocations_before_failure == 0) {
        allocations_before_failure = -1;
        return 1;
    }
    allocations_before_failure--;
    return 0;
}

void *__wrap_malloc(size_t size)
{
    return should_fail_allocation() ? NULL : __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    return should_fail_allocation() ? NULL : __real_calloc(count, size);
}

void __wrap_free(void *pointer)
{
    __real_free(pointer);
}

int main(void)
{
    const double values[] = {1.0, 2.0, 3.0, 4.0};
    long fail_after;
    int allocation_failure_observed = 0;
    int successful_creation_observed = 0;

    /*
     * Fail each allocation in turn until the operation succeeds. This makes
     * the test sensitive to added allocation points without hard-coding the
     * current allocation count.
     */
    for (fail_after = 0; fail_after < 16; fail_after++) {
        numerus_matrix *matrix = (numerus_matrix *) 1;
        numerus_matrix_status status;

        allocations_before_failure = fail_after;
        status = numerus_matrix_create_dense(2, 2, values, &matrix);

        if (status == NUMERUS_MATRIX_SUCCESS) {
            assert(matrix != NULL);
            numerus_matrix_destroy(matrix);
            successful_creation_observed = 1;
            break;
        }

        assert(status == NUMERUS_MATRIX_OUT_OF_MEMORY);
        assert(matrix == NULL);
        allocation_failure_observed = 1;
    }

    assert(allocation_failure_observed);
    assert(successful_creation_observed);

    /* A failure must not poison later operations. */
    {
        numerus_matrix *matrix = NULL;

        assert(numerus_matrix_create_dense(2, 2, values, &matrix) ==
            NUMERUS_MATRIX_SUCCESS);
        assert(matrix != NULL);
        numerus_matrix_destroy(matrix);
    }

    puts("Matrix allocation-failure tests passed.");
    return 0;
}
