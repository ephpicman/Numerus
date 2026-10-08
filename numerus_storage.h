#ifndef NUMERUS_STORAGE_H
#define NUMERUS_STORAGE_H

#include <stddef.h>

typedef enum {
    NUMERUS_STORAGE_SUCCESS = 0,
    NUMERUS_STORAGE_INVALID_ARGUMENT,
    NUMERUS_STORAGE_OVERFLOW,
    NUMERUS_STORAGE_OUT_OF_MEMORY,
    NUMERUS_STORAGE_OUT_OF_BOUNDS
} numerus_storage_status;

typedef enum {
    NUMERUS_STORAGE_DENSE,
    NUMERUS_STORAGE_UPPER_TRIANGULAR,
    NUMERUS_STORAGE_LOWER_TRIANGULAR,
    NUMERUS_STORAGE_DIAGONAL,
    NUMERUS_STORAGE_IDENTITY,
    NUMERUS_STORAGE_CONSTANT,
    NUMERUS_STORAGE_ZERO,
    NUMERUS_STORAGE_SCALED_IDENTITY,
    NUMERUS_STORAGE_SPARSE,
    NUMERUS_STORAGE_SYMMETRIC,
    NUMERUS_STORAGE_BANDED
} numerus_storage_kind;

typedef struct numerus_storage numerus_storage;

typedef struct {
    size_t index;
    double value;
} numerus_storage_sparse_entry;

struct numerus_storage {
    numerus_storage_kind kind;
    size_t rows;
    size_t columns;

    union {
        struct { double *values; } dense;
        struct { double *values; } upper_triangular;
        struct { double *values; } lower_triangular;
        struct { double *values; } diagonal;
        struct { double value; } constant;
        struct { double value; } scaled_identity;
        struct {
            size_t count;
            numerus_storage_sparse_entry *entries;
            double default_value;
        } sparse;
        struct { double *values; } symmetric;
        struct {
            size_t lower_bandwidth;
            size_t upper_bandwidth;
            double *values;
        } banded;
    } data;
};

int numerus_storage_create_dense(size_t rows, size_t columns, const double *values, numerus_storage **storage);
int numerus_storage_create_upper_triangular(size_t size, const double *values, numerus_storage **storage);
int numerus_storage_create_lower_triangular(size_t size, const double *values, numerus_storage **storage);
int numerus_storage_create_diagonal(size_t size, const double *values, numerus_storage **storage);
int numerus_storage_create_identity(size_t size, numerus_storage **storage);
int numerus_storage_create_constant(size_t rows, size_t columns, double value, numerus_storage **storage);
int numerus_storage_create_zero(size_t rows, size_t columns, numerus_storage **storage);
int numerus_storage_create_scaled_identity(size_t size, double value, numerus_storage **storage);
int numerus_storage_create_sparse(size_t rows, size_t columns, double default_value, const numerus_storage_sparse_entry *entries, size_t count, numerus_storage **storage);
int numerus_storage_create_symmetric(size_t size, const double *values, numerus_storage **storage);
int numerus_storage_create_banded(size_t rows, size_t columns, size_t lower_bandwidth, size_t upper_bandwidth, const double *values, numerus_storage **storage);

numerus_storage_status numerus_storage_get(
    const numerus_storage *storage,
    size_t row,
    size_t column,
    double *value
);

double numerus_storage_get_unchecked(
    const numerus_storage *storage,
    size_t row,
    size_t column
);
void numerus_storage_destroy(numerus_storage *storage);

size_t numerus_storage_rows(const numerus_storage *storage);
size_t numerus_storage_columns(const numerus_storage *storage);
numerus_storage_kind numerus_storage_kind_of(const numerus_storage *storage);

#endif
