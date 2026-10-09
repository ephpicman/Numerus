#include "../numerus_matrix.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MATRIX_SIZE 64
#define SMALL_SIZE 32
#define READ_ITERATIONS 10
#define OPERATION_ITERATIONS 3

void *__real_malloc(size_t size);
void *__real_calloc(size_t count, size_t size);
void __real_free(void *pointer);

static size_t allocation_calls;
static size_t allocation_bytes;
static volatile double benchmark_sink;

void *__wrap_malloc(size_t size)
{
    allocation_calls++;
    if (size <= SIZE_MAX - allocation_bytes) {
        allocation_bytes += size;
    } else {
        allocation_bytes = SIZE_MAX;
    }

    return __real_malloc(size);
}

void *__wrap_calloc(size_t count, size_t size)
{
    allocation_calls++;
    if (size == 0 || count <= SIZE_MAX / size) {
        size_t bytes = count * size;
        if (bytes <= SIZE_MAX - allocation_bytes) {
            allocation_bytes += bytes;
        } else {
            allocation_bytes = SIZE_MAX;
        }
    } else {
        allocation_bytes = SIZE_MAX;
    }

    return __real_calloc(count, size);
}

void __wrap_free(void *pointer)
{
    __real_free(pointer);
}

static void reset_allocation_stats(void)
{
    allocation_calls = 0;
    allocation_bytes = 0;
}

static void report_measurement(
    const char *name,
    size_t rows,
    size_t columns,
    size_t iterations,
    clock_t start,
    clock_t end
)
{
    size_t calls = allocation_calls;
    size_t bytes = allocation_bytes;
    double seconds = (double) (end - start) / (double) CLOCKS_PER_SEC;

    printf(
        "%-24s shape=%zux%zu iterations=%zu seconds=%.6f "
        "allocations=%zu allocated_bytes=%zu sink=%.6f\n",
        name,
        rows,
        columns,
        iterations,
        seconds,
        calls,
        bytes,
        (double) benchmark_sink
    );
}

static void benchmark_reads(
    const char *name,
    const numerus_matrix *matrix,
    size_t iterations
)
{
    size_t rows = numerus_matrix_rows(matrix);
    size_t columns = numerus_matrix_columns(matrix);
    size_t iteration;
    size_t row;
    size_t column;
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        for (row = 0; row < rows; row++) {
            for (column = 0; column < columns; column++) {
                double value;
                int status = numerus_matrix_get(
                    matrix, row, column, &value
                );

                if (status != NUMERUS_MATRIX_SUCCESS) {
                    fprintf(stderr, "read failed in benchmark '%s': %d\n", name, status);
                    exit(EXIT_FAILURE);
                }

                benchmark_sink += value;
            }
        }
    }

    end = clock();
    report_measurement(name, rows, columns, iterations, start, end);
}

static void benchmark_materialize(
    const numerus_matrix *source,
    size_t iterations
)
{
    size_t iteration;
    size_t rows = numerus_matrix_rows(source);
    size_t columns = numerus_matrix_columns(source);
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        numerus_matrix *result = NULL;
        double value;
        int status = numerus_matrix_materialize(source, &result);

        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "materialization benchmark failed: %d\n", status);
            exit(EXIT_FAILURE);
        }

        status = numerus_matrix_get(result, 0, 0, &value);
        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "materialized result read failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        benchmark_sink += value;
        numerus_matrix_destroy(result);
    }

    end = clock();
    report_measurement(
        "materialize view", rows, columns, iterations, start, end
    );
}

static void benchmark_multiplication(
    const char *name,
    const numerus_matrix *left,
    const numerus_matrix *right,
    size_t iterations
)
{
    size_t iteration;
    size_t rows = numerus_matrix_rows(left);
    size_t columns = numerus_matrix_columns(right);
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        numerus_matrix *result = NULL;
        double value;
        int status = numerus_matrix_multiply(left, right, &result);

        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "multiplication benchmark failed: %d\n", status);
            exit(EXIT_FAILURE);
        }

        status = numerus_matrix_get(result, 0, 0, &value);
        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "product read failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        benchmark_sink += value;
        numerus_matrix_destroy(result);
    }

    end = clock();
    report_measurement(
        name, rows, columns, iterations, start, end
    );
}

static void benchmark_inverse(
    const numerus_matrix *matrix,
    size_t iterations
)
{
    size_t iteration;
    size_t size = numerus_matrix_rows(matrix);
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        numerus_matrix *inverse = NULL;
        double value;
        int status = numerus_matrix_inverse(matrix, &inverse);

        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "inverse benchmark failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        status = numerus_matrix_get(inverse, 0, 0, &value);
        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "inverse result read failed: %d\n", status);
            numerus_matrix_destroy(inverse);
            exit(EXIT_FAILURE);
        }
        benchmark_sink += value;
        numerus_matrix_destroy(inverse);
    }

    end = clock();
    report_measurement("LU inverse", size, size, iterations, start, end);
}

static void benchmark_condition_estimate(
    const numerus_matrix *matrix,
    size_t iterations
)
{
    size_t iteration;
    size_t size = numerus_matrix_rows(matrix);
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        double condition_estimate;
        int status = numerus_matrix_condition_estimate_one(
            matrix, &condition_estimate
        );

        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "condition estimate benchmark failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        benchmark_sink += condition_estimate;
    }

    end = clock();
    report_measurement(
        "condition estimate", size, size, iterations, start, end
    );
}

static void verify_inverse_residual(const numerus_matrix *matrix)
{
    numerus_matrix *inverse = NULL;
    size_t size = numerus_matrix_rows(matrix);
    size_t row;
    double maximum_residual = 0.0;
    int status = numerus_matrix_inverse(matrix, &inverse);

    if (status != NUMERUS_MATRIX_SUCCESS) {
        fprintf(stderr, "inverse residual check failed: %d\n", status);
        exit(EXIT_FAILURE);
    }

    for (row = 0; row < size; row++) {
        size_t column;

        for (column = 0; column < size; column++) {
            size_t k;
            double product = 0.0;
            double expected = row == column ? 1.0 : 0.0;

            for (k = 0; k < size; k++) {
                double left;
                double right;

                status = numerus_matrix_get(matrix, row, k, &left);
                if (status != NUMERUS_MATRIX_SUCCESS) goto failure;
                status = numerus_matrix_get(inverse, k, column, &right);
                if (status != NUMERUS_MATRIX_SUCCESS) goto failure;
                product += left * right;
            }

            if (fabs(product - expected) > maximum_residual) {
                maximum_residual = fabs(product - expected);
            }
        }
    }

    printf("LU inverse residual: max|A*A^-1-I|=%.6e\n", maximum_residual);
    numerus_matrix_destroy(inverse);
    return;

failure:
    fprintf(stderr, "inverse residual read failed: %d\n", status);
    numerus_matrix_destroy(inverse);
    exit(EXIT_FAILURE);
}

static void benchmark_scale_view_creation(
    numerus_matrix *parent,
    size_t iterations
)
{
    size_t iteration;
    size_t rows = numerus_matrix_rows(parent);
    size_t columns = numerus_matrix_columns(parent);
    clock_t start;
    clock_t end;

    reset_allocation_stats();
    start = clock();

    for (iteration = 0; iteration < iterations; iteration++) {
        numerus_matrix *view = NULL;
        int status = numerus_matrix_create_scale(parent, 2.0, &view);

        if (status != NUMERUS_MATRIX_SUCCESS) {
            fprintf(stderr, "scale view construction failed: %d\n", status);
            exit(EXIT_FAILURE);
        }
        numerus_matrix_destroy(view);
    }

    end = clock();
    report_measurement(
        "scale view creation", rows, columns, iterations, start, end
    );
}

int main(void)
{
    static double dense_values[MATRIX_SIZE * MATRIX_SIZE];
    static double diagonal_values[MATRIX_SIZE];
    static double upper_values[MATRIX_SIZE * (MATRIX_SIZE + 1) / 2];
    static numerus_storage_sparse_entry sparse_entries[MATRIX_SIZE];
    static double small_values[SMALL_SIZE * SMALL_SIZE];
    static double ill_conditioned_values[SMALL_SIZE * SMALL_SIZE];

    numerus_matrix *dense = NULL;
    numerus_matrix *diagonal = NULL;
    numerus_matrix *upper = NULL;
    numerus_matrix *sparse = NULL;
    numerus_matrix *identity = NULL;
    numerus_matrix *zero = NULL;
    numerus_matrix *transpose = NULL;
    numerus_matrix *joined = NULL;
    numerus_matrix *scaled = NULL;
    numerus_matrix *sum = NULL;
    numerus_matrix *small = NULL;
    numerus_matrix *ill_conditioned = NULL;
    size_t row;
    size_t column;
    size_t packed_index = 0;
    int status;

    printf("Numerus Matrix benchmark baseline\n");
    printf("compiler=%s matrix_size=%d small_size=%d\n", __VERSION__,
        MATRIX_SIZE, SMALL_SIZE);
    printf("timing uses clock(); allocation metrics wrap malloc/calloc in the standalone test build\n");

    for (row = 0; row < MATRIX_SIZE; row++) {
        diagonal_values[row] = (double) (row + 1);
        sparse_entries[row].index = row * MATRIX_SIZE + row;
        sparse_entries[row].value = (double) (row + 1);

        for (column = row; column < MATRIX_SIZE; column++) {
            upper_values[packed_index++] =
                (double) ((row + column) % 11 + 1);
        }

        for (column = 0; column < MATRIX_SIZE; column++) {
            dense_values[row * MATRIX_SIZE + column] =
                (double) ((row + column) % 13 + 1) / 4.0;
        }
    }

    for (row = 0; row < SMALL_SIZE; row++) {
        for (column = 0; column < SMALL_SIZE; column++) {
            small_values[row * SMALL_SIZE + column] = row == column
                ? (double) (SMALL_SIZE + row + 1)
                : (double) ((row + column) % 13 + 1) / 16.0;
        }
    }

    for (row = 0; row < SMALL_SIZE; row++) {
        for (column = 0; column < SMALL_SIZE; column++) {
            ill_conditioned_values[row * SMALL_SIZE + column] =
                row == column
                    ? (row == SMALL_SIZE - 1 ? 1e-7 : 1.0)
                    : 0.0;
        }
    }

    status = numerus_matrix_create_dense(
        MATRIX_SIZE, MATRIX_SIZE, dense_values, &dense
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_diagonal(
        MATRIX_SIZE, diagonal_values, &diagonal
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_upper_triangular(
        MATRIX_SIZE, upper_values, &upper
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_sparse(
        MATRIX_SIZE, MATRIX_SIZE, 0.0,
        sparse_entries, MATRIX_SIZE, &sparse
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_identity(MATRIX_SIZE, &identity);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_zero(MATRIX_SIZE, MATRIX_SIZE, &zero);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_transpose(dense, &transpose);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_join_horizontal(dense, transpose, &joined);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_scale(dense, 2.0, &scaled);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_add(dense, transpose, &sum);
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;
    status = numerus_matrix_create_dense(
        SMALL_SIZE, SMALL_SIZE, small_values, &small
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;

    status = numerus_matrix_create_dense(
        SMALL_SIZE, SMALL_SIZE, ill_conditioned_values, &ill_conditioned
    );
    if (status != NUMERUS_MATRIX_SUCCESS) goto fail;

    benchmark_reads("dense read", dense, READ_ITERATIONS);
    benchmark_reads("diagonal read", diagonal, READ_ITERATIONS);
    benchmark_reads("triangular read", upper, READ_ITERATIONS);
    benchmark_reads("sparse read", sparse, READ_ITERATIONS);
    benchmark_reads("transpose view read", transpose, READ_ITERATIONS);
    benchmark_reads("joined view read", joined, READ_ITERATIONS);
    benchmark_reads("scaled view read", scaled, READ_ITERATIONS);
    benchmark_reads("binary view read", sum, READ_ITERATIONS);
    benchmark_scale_view_creation(dense, 1000);
    benchmark_materialize(transpose, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x dense", dense, dense, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x identity", dense, identity, OPERATION_ITERATIONS);
    benchmark_multiplication("identity x dense", identity, dense, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x zero", dense, zero, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x diagonal", dense, diagonal, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x triangular", dense, upper, OPERATION_ITERATIONS);
    benchmark_multiplication("dense x sparse", dense, sparse, OPERATION_ITERATIONS);
    benchmark_multiplication("small dense multiply", small, small, OPERATION_ITERATIONS);
    benchmark_inverse(small, OPERATION_ITERATIONS);
    benchmark_condition_estimate(small, OPERATION_ITERATIONS);
    verify_inverse_residual(small);

    benchmark_inverse(ill_conditioned, OPERATION_ITERATIONS);
    benchmark_condition_estimate(ill_conditioned, OPERATION_ITERATIONS);
    verify_inverse_residual(ill_conditioned);

    numerus_matrix_destroy(ill_conditioned);
    numerus_matrix_destroy(small);
    numerus_matrix_destroy(sum);
    numerus_matrix_destroy(scaled);
    numerus_matrix_destroy(joined);
    numerus_matrix_destroy(transpose);
    numerus_matrix_destroy(zero);
    numerus_matrix_destroy(identity);
    numerus_matrix_destroy(sparse);
    numerus_matrix_destroy(upper);
    numerus_matrix_destroy(diagonal);
    numerus_matrix_destroy(dense);

    return EXIT_SUCCESS;

fail:
    fprintf(stderr, "benchmark setup failed: %d\n", status);
    numerus_matrix_destroy(ill_conditioned);
    numerus_matrix_destroy(small);
    numerus_matrix_destroy(sum);
    numerus_matrix_destroy(scaled);
    numerus_matrix_destroy(joined);
    numerus_matrix_destroy(transpose);
    numerus_matrix_destroy(sparse);
    numerus_matrix_destroy(upper);
    numerus_matrix_destroy(diagonal);
    numerus_matrix_destroy(dense);
    return EXIT_FAILURE;
}
