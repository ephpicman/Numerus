#include "numerus_matrix.h"

numerus_matrix_status numerus_matrix_trace(
    const numerus_matrix *matrix,
    double *trace
)
{
    double result = 0.0;
    size_t index;
    size_t size;

    if (matrix == NULL || trace == NULL) {
        return NUMERUS_MATRIX_INVALID_ARGUMENT;
    }

    if (numerus_matrix_rows(matrix) != numerus_matrix_columns(matrix)) {
        return NUMERUS_MATRIX_NOT_SQUARE;
    }

    size = numerus_matrix_rows(matrix);
    for (index = 0; index < size; index++) {
        double value;
        numerus_matrix_status status = numerus_matrix_get(
            matrix, index, index, &value
        );

        if (status != NUMERUS_MATRIX_SUCCESS) {
            return status;
        }

        result += value;
    }

    *trace = result;
    return NUMERUS_MATRIX_SUCCESS;
}
