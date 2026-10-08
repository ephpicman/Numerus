#include "../numerus_storage.h"

#include <assert.h>
#include <stdio.h>

static void test_basic_storages(void)
{
    const double dense[] = {1, 2, 3, 4, 5, 6};
    const double packed[] = {1, 2, 3, 4, 5, 6};
    const double diagonal[] = {10, 20, 30};
    numerus_storage *s = NULL;

    assert(numerus_storage_create_dense(2, 3, dense, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 6.0);
    }
    assert(numerus_storage_rows(s) == 2);
    assert(numerus_storage_columns(s) == 3);
    numerus_storage_destroy(s);

    assert(numerus_storage_create_upper_triangular(3, packed, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 0, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 3.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 1, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 4.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 2, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_lower_triangular(3, packed, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 2, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 4.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 0, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_diagonal(3, diagonal, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 2, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 30.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 0, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);
}

static void test_implicit_storages(void)
{
    numerus_storage *s = NULL;

    assert(numerus_storage_create_identity(3, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 0, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 1.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 0, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_constant(2, 3, 42, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 42.0);
        assert(sizeof(value) == sizeof(double));
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_zero(2, 3, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_scaled_identity(3, 7, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 1, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 7.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);
}

static void test_sparse(void)
{
    const numerus_storage_sparse_entry entries[] = {
        {5, 20},
        {1, 10}
    };
    numerus_storage *s = NULL;

    assert(numerus_storage_create_sparse(2, 3, -1, entries, 2, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 0, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 10.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 20.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 0, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == -1.0);
    }
    numerus_storage_destroy(s);
}

static void test_symmetric_and_banded(void)
{
    const double symmetric[] = {1, 2, 3, 4, 5, 6};
    const double banded[] = {
        0, 1, 2,
        3, 4, 5,
        6, 7, 0
    };
    numerus_storage *s = NULL;

    assert(numerus_storage_create_symmetric(3, symmetric, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 0, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 4.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 2, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 4.0);
    }
    numerus_storage_destroy(s);

    assert(numerus_storage_create_banded(3, 3, 1, 1, banded, &s) == 0);
    {
        double value;
        assert(numerus_storage_get(s, 0, 1, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 1.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 1, 0, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 3.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 1, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 5.0);
    }
    {
        double value;
        assert(numerus_storage_get(s, 0, 2, &value) == NUMERUS_STORAGE_SUCCESS);
        assert(value == 0.0);
    }
    numerus_storage_destroy(s);
}

static void test_bounds(void)
{
    numerus_storage *s = NULL;
    double value = 123.0;

    assert(numerus_storage_create_zero(2, 3, &s) == NUMERUS_STORAGE_SUCCESS);
    assert(numerus_storage_get(s, 2, 0, &value) == NUMERUS_STORAGE_OUT_OF_BOUNDS);
    assert(value == 123.0);
    assert(numerus_storage_get(s, 0, 3, &value) == NUMERUS_STORAGE_OUT_OF_BOUNDS);
    assert(numerus_storage_get(s, 0, 0, NULL) == NUMERUS_STORAGE_INVALID_ARGUMENT);
    assert(numerus_storage_get(NULL, 0, 0, &value) == NUMERUS_STORAGE_INVALID_ARGUMENT);
    assert(numerus_storage_get_unchecked(s, 1, 2) == 0.0);
    numerus_storage_destroy(s);
}

int main(void)
{
    test_basic_storages();
    test_implicit_storages();
    test_sparse();
    test_symmetric_and_banded();
    test_bounds();

    puts("Storage tests passed.");
    return 0;
}
