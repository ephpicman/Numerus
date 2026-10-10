/**
 * @file numerus_matrix_php.c
 * @brief PHP-facing Matrix value object.
 *
 * The PHP façade owns every native Matrix it exposes. Operations that create
 * native views are materialized before returning so no PHP object can retain
 * a dangling non-owning C parent reference.
 */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "Zend/zend_exceptions.h"
#include "../php_numerus.h"
#include "matrix/numerus_matrix.h"

#include <math.h>
#include <stdint.h>

typedef struct {
    numerus_matrix *matrix;
    zend_object std;
} numerus_php_matrix;

static zend_class_entry *numerus_matrix_ce;
static zend_object_handlers numerus_matrix_handlers;

#define Z_NUMERUS_MATRIX_P(zv) \
    ((numerus_php_matrix *)((char *) Z_OBJ_P((zv)) - XtOffsetOf(numerus_php_matrix, std)))

static zend_object *numerus_matrix_object_create(zend_class_entry *ce)
{
    numerus_php_matrix *object = zend_object_alloc(sizeof(*object), ce);

    object->matrix = NULL;
    zend_object_std_init(&object->std, ce);
    object_properties_init(&object->std, ce);
    object->std.handlers = &numerus_matrix_handlers;

    return &object->std;
}

static zend_object *numerus_matrix_object_clone(zend_object *std)
{
    (void) std;
    zend_throw_error(NULL, "Cloning Numerus\\Matrix objects is not supported");
    return NULL;
}

static void numerus_matrix_object_free(zend_object *std)
{
    numerus_php_matrix *object =
        (numerus_php_matrix *)((char *) std - XtOffsetOf(numerus_php_matrix, std));

    if (object->matrix != NULL) {
        numerus_matrix_destroy(object->matrix);
        object->matrix = NULL;
    }

    zend_object_std_dtor(&object->std);
}

static void numerus_matrix_throw_status(numerus_matrix_status status)
{
    const char *message;
    bool value_error = false;

    switch (status) {
        case NUMERUS_MATRIX_INVALID_ARGUMENT:
            message = "Invalid Matrix argument";
            value_error = true;
            break;
        case NUMERUS_MATRIX_OVERFLOW:
            message = "Matrix dimensions overflow the supported size";
            value_error = true;
            break;
        case NUMERUS_MATRIX_OUT_OF_MEMORY:
            message = "Unable to allocate Matrix memory";
            break;
        case NUMERUS_MATRIX_OUT_OF_BOUNDS:
            message = "Matrix coordinate is out of bounds";
            value_error = true;
            break;
        case NUMERUS_MATRIX_NOT_SQUARE:
            message = "Operation requires a square Matrix";
            value_error = true;
            break;
        case NUMERUS_MATRIX_DIMENSION_MISMATCH:
            message = "Matrix dimensions are incompatible";
            value_error = true;
            break;
        case NUMERUS_MATRIX_DIVISION_BY_ZERO:
            message = "Matrix operation attempted division by zero";
            break;
        case NUMERUS_MATRIX_NON_FINITE:
            message = "Matrix operation encountered a non-finite value";
            break;
        case NUMERUS_MATRIX_SINGULAR:
        case NUMERUS_MATRIX_RANK_DEFICIENT:
        case NUMERUS_MATRIX_PIVOT_TOO_SMALL:
            message = "Matrix is singular or numerically rank deficient";
            break;
        case NUMERUS_MATRIX_NOT_SYMMETRIC:
            message = "Matrix must be symmetric";
            value_error = true;
            break;
        case NUMERUS_MATRIX_NOT_POSITIVE_DEFINITE:
        case NUMERUS_MATRIX_NOT_POSITIVE_SEMIDEFINITE:
            message = "Matrix does not satisfy the required definiteness";
            value_error = true;
            break;
        case NUMERUS_MATRIX_NO_CONVERGENCE:
            message = "Numerical algorithm did not converge";
            break;
        case NUMERUS_MATRIX_SUCCESS:
            return;
        default:
            message = "Matrix operation failed";
            break;
    }

    if (value_error) {
        zend_value_error("%s", message);
    } else {
        zend_throw_exception(zend_ce_exception, message, 0);
    }
}

static numerus_matrix *numerus_matrix_require(zval *value)
{
    numerus_php_matrix *object;

    if (Z_TYPE_P(value) != IS_OBJECT
        || !instanceof_function(Z_OBJCE_P(value), numerus_matrix_ce)) {
        zend_type_error("Expected an instance of Numerus\\Matrix");
        return NULL;
    }

    object = Z_NUMERUS_MATRIX_P(value);
    if (object->matrix == NULL) {
        zend_throw_error(NULL, "Uninitialized Numerus\\Matrix object");
        return NULL;
    }

    return object->matrix;
}

static void numerus_matrix_return_owned(zval *return_value, numerus_matrix *matrix)
{
    object_init_ex(return_value, numerus_matrix_ce);
    Z_NUMERUS_MATRIX_P(return_value)->matrix = matrix;
}

ZEND_BEGIN_ARG_INFO_EX(arginfo_matrix_private_construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_matrix_from_rows, 0, 1, Numerus\\Matrix, 0)
    ZEND_ARG_TYPE_INFO(0, rows, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_matrix_zeros, 0, 2, Numerus\\Matrix, 0)
    ZEND_ARG_TYPE_INFO(0, rows, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, columns, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_matrix_dimension, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_matrix_get, 0, 2, IS_DOUBLE, 0)
    ZEND_ARG_TYPE_INFO(0, row, IS_LONG, 0)
    ZEND_ARG_TYPE_INFO(0, column, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_matrix_unary, 0, 0, Numerus\\Matrix, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_matrix_binary, 0, 1, Numerus\\Matrix, 0)
    ZEND_ARG_OBJ_INFO(0, other, Numerus\\Matrix, 0)
ZEND_END_ARG_INFO()

PHP_METHOD(NumerusMatrix, __construct)
{
    ZEND_PARSE_PARAMETERS_NONE();
    zend_throw_error(NULL, "Use Numerus\\Matrix factory methods to create matrices");
}

PHP_METHOD(NumerusMatrix, fromRows)
{
    zval *rows_zv;
    zval *row_zv;
    zval *value_zv;
    HashTable *rows;
    HashTable *row;
    uint32_t row_count;
    uint32_t column_count = 0;
    uint32_t current_row = 0;
    uint32_t current_column;
    double *values;
    numerus_matrix *matrix = NULL;
    numerus_matrix_status status = NUMERUS_MATRIX_SUCCESS;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(rows_zv)
    ZEND_PARSE_PARAMETERS_END();

    rows = Z_ARRVAL_P(rows_zv);
    row_count = zend_hash_num_elements(rows);
    if (row_count == 0) {
        zend_value_error("Matrix rows must not be empty");
        RETURN_THROWS();
    }

    ZEND_HASH_FOREACH_VAL(rows, row_zv) {
        if (Z_TYPE_P(row_zv) != IS_ARRAY) {
            zend_type_error("Every Matrix row must be an array");
            RETURN_THROWS();
        }
        row = Z_ARRVAL_P(row_zv);
        if (current_row == 0) {
            column_count = zend_hash_num_elements(row);
            if (column_count == 0) {
                zend_value_error("Matrix rows must not be empty");
                RETURN_THROWS();
            }
        } else if (zend_hash_num_elements(row) != column_count) {
            zend_value_error("All Matrix rows must have the same number of columns");
            RETURN_THROWS();
        }
        current_row++;
    } ZEND_HASH_FOREACH_END();

    if ((size_t) row_count > SIZE_MAX / (size_t) column_count) {
        zend_value_error("Matrix dimensions are too large");
        RETURN_THROWS();
    }

    values = safe_emalloc((size_t) row_count * column_count, sizeof(*values), 0);
    current_row = 0;
    ZEND_HASH_FOREACH_VAL(rows, row_zv) {
        current_column = 0;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(row_zv), value_zv) {
            if (Z_TYPE_P(value_zv) != IS_LONG && Z_TYPE_P(value_zv) != IS_DOUBLE) {
                efree(values);
                zend_type_error("Matrix elements must be integers or floats");
                RETURN_THROWS();
            }
            values[(size_t) current_row * column_count + current_column] =
                zval_get_double(value_zv);
            current_column++;
        } ZEND_HASH_FOREACH_END();
        current_row++;
    } ZEND_HASH_FOREACH_END();

    status = (numerus_matrix_status) numerus_matrix_create_dense(
        row_count, column_count, values, &matrix
    );
    efree(values);

    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, matrix);
}

PHP_METHOD(NumerusMatrix, zeros)
{
    zend_long rows;
    zend_long columns;
    numerus_matrix *matrix = NULL;
    numerus_matrix_status status;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(rows)
        Z_PARAM_LONG(columns)
    ZEND_PARSE_PARAMETERS_END();

    if (rows <= 0 || columns <= 0) {
        zend_value_error("Matrix dimensions must be positive integers");
        RETURN_THROWS();
    }

    status = (numerus_matrix_status) numerus_matrix_create_zero(
        (size_t) rows, (size_t) columns, &matrix
    );
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, matrix);
}

PHP_METHOD(NumerusMatrix, rows)
{
    numerus_matrix *matrix = numerus_matrix_require(ZEND_THIS);
    if (matrix == NULL) {
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long) numerus_matrix_rows(matrix));
}

PHP_METHOD(NumerusMatrix, columns)
{
    numerus_matrix *matrix = numerus_matrix_require(ZEND_THIS);
    if (matrix == NULL) {
        RETURN_THROWS();
    }
    RETURN_LONG((zend_long) numerus_matrix_columns(matrix));
}

PHP_METHOD(NumerusMatrix, get)
{
    zend_long row;
    zend_long column;
    double value;
    numerus_matrix_status status;
    numerus_matrix *matrix;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(row)
        Z_PARAM_LONG(column)
    ZEND_PARSE_PARAMETERS_END();

    matrix = numerus_matrix_require(ZEND_THIS);
    if (matrix == NULL) {
        RETURN_THROWS();
    }
    if (row < 0 || column < 0) {
        zend_value_error("Matrix coordinates must be non-negative");
        RETURN_THROWS();
    }

    status = numerus_matrix_get(matrix, (size_t) row, (size_t) column, &value);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }
    RETURN_DOUBLE(value);
}

PHP_METHOD(NumerusMatrix, transpose)
{
    numerus_matrix *source = numerus_matrix_require(ZEND_THIS);
    numerus_matrix *view = NULL;
    numerus_matrix *result = NULL;
    numerus_matrix_status status;

    if (source == NULL) {
        RETURN_THROWS();
    }

    status = (numerus_matrix_status) numerus_matrix_create_transpose(source, &view);
    if (status == NUMERUS_MATRIX_SUCCESS) {
        status = numerus_matrix_materialize(view, &result);
    }
    if (view != NULL) {
        numerus_matrix_destroy(view);
    }
    if (status != NUMERUS_MATRIX_SUCCESS) {
        if (result != NULL) {
            numerus_matrix_destroy(result);
        }
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, result);
}

PHP_METHOD(NumerusMatrix, multiply)
{
    zval *other_zv;
    numerus_matrix *left;
    numerus_matrix *right;
    numerus_matrix *result = NULL;
    numerus_matrix_status status;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(other_zv, numerus_matrix_ce)
    ZEND_PARSE_PARAMETERS_END();

    left = numerus_matrix_require(ZEND_THIS);
    right = numerus_matrix_require(other_zv);
    if (left == NULL || right == NULL) {
        RETURN_THROWS();
    }

    status = (numerus_matrix_status) numerus_matrix_multiply(left, right, &result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, result);
}

PHP_METHOD(NumerusMatrix, inverse)
{
    numerus_matrix *source = numerus_matrix_require(ZEND_THIS);
    numerus_matrix *result = NULL;
    numerus_matrix_status status;

    if (source == NULL) {
        RETURN_THROWS();
    }

    status = numerus_matrix_inverse(source, &result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, result);
}

PHP_METHOD(NumerusMatrix, leastSquares)
{
    zval *rhs_zv;
    numerus_matrix *source;
    numerus_matrix *rhs;
    numerus_matrix *result = NULL;
    numerus_matrix_status status;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_OBJECT_OF_CLASS(rhs_zv, numerus_matrix_ce)
    ZEND_PARSE_PARAMETERS_END();

    source = numerus_matrix_require(ZEND_THIS);
    rhs = numerus_matrix_require(rhs_zv);
    if (source == NULL || rhs == NULL) {
        RETURN_THROWS();
    }

    status = numerus_matrix_least_squares(source, rhs, &result);
    if (status != NUMERUS_MATRIX_SUCCESS) {
        numerus_matrix_throw_status(status);
        RETURN_THROWS();
    }

    numerus_matrix_return_owned(return_value, result);
}

static const zend_function_entry numerus_matrix_methods[] = {
    PHP_ME(NumerusMatrix, __construct, arginfo_matrix_private_construct, ZEND_ACC_PRIVATE)
    PHP_ME(NumerusMatrix, fromRows, arginfo_matrix_from_rows, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(NumerusMatrix, zeros, arginfo_matrix_zeros, ZEND_ACC_PUBLIC | ZEND_ACC_STATIC)
    PHP_ME(NumerusMatrix, rows, arginfo_matrix_dimension, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, columns, arginfo_matrix_dimension, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, get, arginfo_matrix_get, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, transpose, arginfo_matrix_unary, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, inverse, arginfo_matrix_unary, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, multiply, arginfo_matrix_binary, ZEND_ACC_PUBLIC)
    PHP_ME(NumerusMatrix, leastSquares, arginfo_matrix_binary, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

void numerus_matrix_php_register(void)
{
    zend_class_entry ce;

    INIT_NS_CLASS_ENTRY(ce, "Numerus", "Matrix", numerus_matrix_methods);
    numerus_matrix_ce = zend_register_internal_class(&ce);
    numerus_matrix_ce->create_object = numerus_matrix_object_create;

    memcpy(&numerus_matrix_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
    numerus_matrix_handlers.offset = XtOffsetOf(numerus_php_matrix, std);
    numerus_matrix_handlers.free_obj = numerus_matrix_object_free;
    numerus_matrix_handlers.clone_obj = numerus_matrix_object_clone;

}
