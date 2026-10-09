#include "numerus_matrix.h"
#include "numerus_size.h"

#include <math.h>
#include <stdlib.h>

#ifndef NUMERUS_MATRIX_USE_LIBC_ALLOC
# include "php.h"
# include "Zend/zend_alloc.h"
# define matrix_qr_alloc(size) emalloc(size)
# define matrix_qr_free(pointer) efree(pointer)
#else
# define matrix_qr_alloc(size) malloc(size)
# define matrix_qr_free(pointer) free(pointer)
#endif

numerus_matrix_status numerus_matrix_qr_decompose(
    const numerus_matrix *matrix,
    numerus_matrix **q,
    numerus_matrix **r
)
{
    size_t rows;
    size_t columns;
    size_t rank_bound;
    size_t work_count;
    size_t q_count;
    size_t r_count;
    size_t work_bytes;
    size_t tau_bytes;
    size_t q_bytes;
    size_t r_bytes;
    size_t row;
    size_t column;
    size_t step;
    double *work = NULL;
    double *tau = NULL;
    double *q_values = NULL;
    double *r_values = NULL;
    numerus_matrix_status status = NUMERUS_MATRIX_SUCCESS;

    if (q != NULL) {
        *q = NULL;
    }
    if (r != NULL && r != q) {
        *r = NULL;
    }
    if (q == NULL || r == NULL || q == r || matrix == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    rows = numerus_matrix_rows(matrix);
    columns = numerus_matrix_columns(matrix);
    rank_bound = rows < columns ? rows : columns;

    if (!numerus_size_multiply(rows, columns, &work_count) ||
        !numerus_size_multiply(work_count, sizeof(*work), &work_bytes) ||
        !numerus_size_multiply(rows, rank_bound, &q_count) ||
        !numerus_size_multiply(q_count, sizeof(*q_values), &q_bytes) ||
        !numerus_size_multiply(rank_bound, columns, &r_count) ||
        !numerus_size_multiply(r_count, sizeof(*r_values), &r_bytes) ||
        !numerus_size_multiply(rank_bound, sizeof(*tau), &tau_bytes)) {
        return NUMERUS_MATRIX_OVERFLOW;
    }

    work = matrix_qr_alloc(work_bytes);
    tau = matrix_qr_alloc(tau_bytes);
    q_values = matrix_qr_alloc(q_bytes);
    r_values = matrix_qr_alloc(r_bytes);
    if (work == NULL || tau == NULL || q_values == NULL || r_values == NULL) {
        status = NUMERUS_MATRIX_OUT_OF_MEMORY;
        goto cleanup;
    }

    for (row = 0; row < rows; row++) {
        for (column = 0; column < columns; column++) {
            double value;
            status = numerus_matrix_get(matrix, row, column, &value);
            if (status != NUMERUS_MATRIX_SUCCESS) {
                goto cleanup;
            }
            if (!isfinite(value)) {
                status = NUMERUS_MATRIX_NON_FINITE;
                goto cleanup;
            }
            work[row * columns + column] = value;
        }
    }

    /*
     * Store each Householder vector below R's diagonal. The normalized
     * denominator avoids forming alpha - beta, which can overflow even when
     * both alpha and the column norm are finite.
     */
    for (step = 0; step < rank_bound; step++) {
        double norm = 0.0;
        double alpha = work[step * columns + step];
        double beta;
        double denominator_ratio;

        for (row = step; row < rows; row++) {
            norm = hypot(norm, work[row * columns + step]);
            if (!isfinite(norm)) {
                status = NUMERUS_MATRIX_NON_FINITE;
                goto cleanup;
            }
        }

        if (norm == 0.0) {
            tau[step] = 0.0;
            continue;
        }

        beta = -copysign(norm, alpha);
        tau[step] = 1.0 - alpha / beta;
        denominator_ratio = alpha / norm - beta / norm;
        if (!isfinite(tau[step]) || denominator_ratio == 0.0 ||
            !isfinite(denominator_ratio)) {
            status = NUMERUS_MATRIX_NON_FINITE;
            goto cleanup;
        }

        for (row = step + 1; row < rows; row++) {
            work[row * columns + step] =
                (work[row * columns + step] / norm) / denominator_ratio;
        }
        work[step * columns + step] = beta;

        for (column = step + 1; column < columns; column++) {
            double dot = work[step * columns + column];

            for (row = step + 1; row < rows; row++) {
                dot += work[row * columns + step] *
                    work[row * columns + column];
                if (!isfinite(dot)) {
                    status = NUMERUS_MATRIX_NON_FINITE;
                    goto cleanup;
                }
            }

            dot *= tau[step];
            if (!isfinite(dot)) {
                status = NUMERUS_MATRIX_NON_FINITE;
                goto cleanup;
            }
            work[step * columns + column] -= dot;
            if (!isfinite(work[step * columns + column])) {
                status = NUMERUS_MATRIX_NON_FINITE;
                goto cleanup;
            }

            for (row = step + 1; row < rows; row++) {
                work[row * columns + column] -=
                    work[row * columns + step] * dot;
                if (!isfinite(work[row * columns + column])) {
                    status = NUMERUS_MATRIX_NON_FINITE;
                    goto cleanup;
                }
            }
        }
    }

    /* Q = H0 H1 ... H(k-1); apply reflectors to basis vectors in reverse. */
    for (column = 0; column < rank_bound; column++) {
        for (row = 0; row < rows; row++) {
            q_values[row * rank_bound + column] = row == column ? 1.0 : 0.0;
        }

        step = rank_bound;
        while (step > 0) {
            double dot;
            size_t index;

            step--;
            if (tau[step] == 0.0) {
                continue;
            }

            dot = q_values[step * rank_bound + column];
            for (row = step + 1; row < rows; row++) {
                dot += work[row * columns + step] *
                    q_values[row * rank_bound + column];
                if (!isfinite(dot)) {
                    status = NUMERUS_MATRIX_NON_FINITE;
                    goto cleanup;
                }
            }
            dot *= tau[step];
            if (!isfinite(dot)) {
                status = NUMERUS_MATRIX_NON_FINITE;
                goto cleanup;
            }

            q_values[step * rank_bound + column] -= dot;
            for (index = step + 1; index < rows; index++) {
                q_values[index * rank_bound + column] -=
                    work[index * columns + step] * dot;
                if (!isfinite(q_values[index * rank_bound + column])) {
                    status = NUMERUS_MATRIX_NON_FINITE;
                    goto cleanup;
                }
            }
        }
    }

    for (row = 0; row < rank_bound; row++) {
        for (column = 0; column < columns; column++) {
            r_values[row * columns + column] =
                row <= column ? work[row * columns + column] : 0.0;
        }
    }

    status = (numerus_matrix_status) numerus_matrix_create_dense(
        rows, rank_bound, q_values, q
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        goto cleanup;
    }

    status = (numerus_matrix_status) numerus_matrix_create_dense(
        rank_bound, columns, r_values, r
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_destroy(*q);
        *q = NULL;
        goto cleanup;
    }

cleanup:
    matrix_qr_free(r_values);
    matrix_qr_free(q_values);
    matrix_qr_free(tau);
    matrix_qr_free(work);
    return status;
}
