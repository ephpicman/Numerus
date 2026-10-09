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
    numerus_matrix *matrix = (numerus_matrix *) 1;

    /* Allow Matrix allocation, then fail the following Storage allocation. */
    allocations_before_failure = 1;
    assert(numerus_matrix_create_dense(2, 2, values, &matrix) ==
        NUMERUS_MATRIX_OUT_OF_MEMORY);
    assert(matrix == NULL);

    /* A failed allocation must not poison later operations. */
    assert(numerus_matrix_create_dense(2, 2, values, &matrix) ==
        NUMERUS_MATRIX_SUCCESS);
    assert(matrix != NULL);
    numerus_matrix_destroy(matrix);

    puts("Matrix allocation-failure tests passed.");
    return 0;
}
