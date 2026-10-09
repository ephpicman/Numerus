/**
 * @file numerus_matrix.h
 * @brief Internal immutable Matrix abstraction built on Storage.
 *
 * Matrix is the logical layer above immutable Storage. A root Matrix owns its
 * Storage; a derived Matrix references a parent Matrix and delegates element
 * reads to it. Parent relationships are non-owning and are intended to be
 * managed by the eventual PHP object layer.
 */
#ifndef NUMERUS_MATRIX_H
#define NUMERUS_MATRIX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "numerus_storage.h"

/**
 * @brief Status codes returned by Matrix operations.
 */
typedef enum {
    /** Operation completed successfully. */
    NUMERUS_MATRIX_SUCCESS = 0,
    /** A required argument is NULL or otherwise invalid. */
    NUMERUS_MATRIX_INVALID_ARGUMENT,
    /** A size calculation would overflow size_t. */
    NUMERUS_MATRIX_OVERFLOW,
    /** Memory allocation failed. */
    NUMERUS_MATRIX_OUT_OF_MEMORY,
    /** A matrix coordinate is outside the valid range. */
    NUMERUS_MATRIX_OUT_OF_BOUNDS,
    /** The requested Storage representation requires a square Matrix. */
    NUMERUS_MATRIX_NOT_SQUARE,
    NUMERUS_MATRIX_DIMENSION_MISMATCH,
    /** Scalar division rejected a zero divisor. */
    NUMERUS_MATRIX_DIVISION_BY_ZERO,
    /** A numerical analysis cannot classify non-finite input or intermediate values. */
    NUMERUS_MATRIX_NON_FINITE,
    /** The operation requires a nonsingular Matrix, but numerical rank is deficient. */
    NUMERUS_MATRIX_SINGULAR,
    /** The requested numerical solve requires full column rank, but rank is deficient. */
    NUMERUS_MATRIX_RANK_DEFICIENT,
    /** The Matrix is not symmetric within the numerical symmetry tolerance. */
    NUMERUS_MATRIX_NOT_SYMMETRIC,
    /** The Matrix is not numerically positive definite. */
    NUMERUS_MATRIX_NOT_POSITIVE_DEFINITE,
    /** An unpivoted decomposition encountered a zero or numerically small pivot. */
    NUMERUS_MATRIX_PIVOT_TOO_SMALL,
    /** A numerical iteration failed to converge within its documented limit. */
    NUMERUS_MATRIX_NO_CONVERGENCE,
    /** A symmetric matrix has a materially negative eigenvalue and is not PSD. */
    NUMERUS_MATRIX_NOT_POSITIVE_SEMIDEFINITE
} numerus_matrix_status;

typedef enum {
    NUMERUS_MATRIX_NORMALIZE_WHOLE = 0,
    NUMERUS_MATRIX_NORMALIZE_ROWS,
    NUMERUS_MATRIX_NORMALIZE_COLUMNS
} numerus_matrix_normalize_axis;

typedef enum {
    NUMERUS_MATRIX_NORMALIZE_L1 = 0,
    NUMERUS_MATRIX_NORMALIZE_L2,
    NUMERUS_MATRIX_NORMALIZE_INFINITY
} numerus_matrix_normalize_norm;

/**
 * Numerical classification of the solutions to A X = B.
 */
typedef enum {
    /** Every right-hand side has exactly one solution. */
    NUMERUS_MATRIX_SOLUTION_UNIQUE = 0,
    /** Every right-hand side is consistent, but solutions are not unique. */
    NUMERUS_MATRIX_SOLUTION_INFINITE,
    /** At least one right-hand side is outside the numerical column space of A. */
    NUMERUS_MATRIX_SOLUTION_INCONSISTENT
} numerus_matrix_solution_kind;

/**
 * @brief Layout used to join two Matrices.
 */
typedef enum {
    /** Append the second Matrix to the right of the first. */
    NUMERUS_MATRIX_JOIN_HORIZONTAL = 0,
    /** Append the second Matrix below the first. */
    NUMERUS_MATRIX_JOIN_VERTICAL
} numerus_matrix_join_type;

/**
 * @brief Cached structural properties of a Matrix.
 *
 * Floating-point properties use NUMERUS_EPSILON from numerus_numeric.h.
 * A square zero Matrix is also diagonal, upper-triangular, lower-triangular,
 * and symmetric.
 */
typedef struct {
    bool square;
    bool zero;
    bool diagonal;
    bool upper_triangular;
    bool lower_triangular;
    bool symmetric;
    bool identity;
} numerus_matrix_flags;

/**
 * @brief Parameters consumed by the general Matrix factory.
 *
 * Only fields required by the selected Storage kind are used. The factory
 * delegates representation-specific construction to Storage.
 */
typedef struct {
    /** Value buffer for dense, triangular, diagonal, symmetric and banded Storage. */
    const double *values;
    /** Single value used by constant and scaled-identity Storage. */
    double value;
    /** Explicit entries used by sparse Storage. */
    const numerus_storage_sparse_entry *entries;
    /** Number of sparse entries. */
    size_t count;
    /** Default value used by sparse Storage. */
    double default_value;
    /** Lower bandwidth used by banded Storage. */
    size_t lower_bandwidth;
    /** Upper bandwidth used by banded Storage. */
    size_t upper_bandwidth;
} numerus_matrix_data;

/**
 * @brief Opaque immutable Matrix object.
 */
typedef struct numerus_matrix numerus_matrix;

/**
 * @brief Map a child Matrix coordinate to its parent's coordinate.
 *
 * The callback must write both parent coordinates on success. It must return
 * a Matrix status on failure; the status is propagated to the caller.
 * The callback must be deterministic and must not depend on mutable external
 * state. The context is borrowed, read-only, and must remain valid and
 * logically unchanged for the lifetime of the child Matrix.
 */
typedef numerus_matrix_status (*numerus_coordinate_transform_fn)(
    size_t row,
    size_t column,
    size_t *parent_row,
    size_t *parent_column,
    const void *context
);

/**
 * @brief Transform a value read from the parent.
 *
 * row and column are the original coordinates requested from the child.
 * The callback writes result only when it returns NUMERUS_MATRIX_SUCCESS.
 * The callback must be deterministic and must not depend on mutable external
 * state. The context is borrowed, read-only, and must remain valid and
 * logically unchanged for the lifetime of the child Matrix.
 */
typedef numerus_matrix_status (*numerus_value_transform_fn)(
    size_t row,
    size_t column,
    double parent_value,
    double *result,
    const void *context
);

/**
 * @brief Create a root Matrix from a Storage representation.
 *
 * On success, Matrix owns the newly created Storage.
 *
 * @param kind Storage representation to construct.
 * @param rows Number of rows; must be greater than zero.
 * @param columns Number of columns; must be greater than zero.
 * @param data Representation-specific construction data. It may be NULL for
 *             representations that require no data.
 * @param matrix Receives the new Matrix.
 */
int numerus_matrix_create(
    numerus_storage_kind kind,
    size_t rows,
    size_t columns,
    const numerus_matrix_data *data,
    numerus_matrix **matrix
);

/**
 * @brief Create a Matrix whose values are provided by an existing parent.
 *
 * A child may have different logical dimensions from its parent. This is
 * required for derived views such as a transpose. The parent reference is
 * non-owning; the eventual object layer is responsible for keeping it alive.
 *
 * @param parent Existing parent Matrix.
 * @param rows Number of rows exposed by the child.
 * @param columns Number of columns exposed by the child.
 * @param matrix Receives the new child Matrix.
 */
int numerus_matrix_create_from_parent(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
);

/**
 * @brief Create a child Matrix with coordinate and value transforms.
 *
 * A NULL callback selects its identity/pass-through default. The context is
 * borrowed and is never modified or freed by Matrix. It must remain valid and
 * logically unchanged for the child's lifetime. Callbacks must be deterministic
 * and must not depend on mutable external state; this contract is required for
 * the child to remain logically immutable. The const qualifier documents and
 * enforces read-only access through this API, but cannot prevent a caller from
 * mutating the same object through another alias.
 */
int numerus_matrix_create_from_parent_with_transforms(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_coordinate_transform_fn coordinate_transform,
    numerus_value_transform_fn value_transform,
    const void *context,
    numerus_matrix **matrix
);

/**
 * @brief Create a lazy transpose view of an existing Matrix.
 *
 * The returned child exposes parent columns as rows and parent rows as
 * columns. No element data is copied. The parent is non-owning and must
 * remain alive for the lifetime of the transpose view.
 */
int numerus_matrix_create_transpose(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/** Create a lazy view with the order of rows reversed. */
int numerus_matrix_create_flip_rows(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/** Create a lazy view with the order of columns reversed. */
int numerus_matrix_create_flip_columns(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/** Create a lazy view that omits one row. */
int numerus_matrix_create_remove_row(
    numerus_matrix *parent,
    size_t row,
    numerus_matrix **matrix
);

/** Create a lazy view that omits one column. */
int numerus_matrix_create_remove_column(
    numerus_matrix *parent,
    size_t column,
    numerus_matrix **matrix
);

/** Create a lazy view that swaps two rows. */
int numerus_matrix_create_swap_rows(
    numerus_matrix *parent,
    size_t row1,
    size_t row2,
    numerus_matrix **matrix
);

/** Create a lazy view that swaps two columns. */
int numerus_matrix_create_swap_columns(
    numerus_matrix *parent,
    size_t column1,
    size_t column2,
    numerus_matrix **matrix
);

/** Create a lazy 90-degree clockwise rotation view. */
int numerus_matrix_create_rotate_90_clockwise(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/** Create a lazy 180-degree rotation view. */
int numerus_matrix_create_rotate_180(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/** Create a lazy 90-degree counter-clockwise rotation view. */
int numerus_matrix_create_rotate_90_counterclockwise(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/**
 * @brief Create a lazy horizontal join of two Matrices.
 *
 * Both parents must have the same number of rows. The second Matrix is
 * appended to the right of the first. Neither parent is owned by the result.
 */
int numerus_matrix_create_join_horizontal(
    numerus_matrix *parent,
    numerus_matrix *parent2,
    numerus_matrix **matrix
);

/**
 * @brief Create a lazy vertical join of two Matrices.
 *
 * Both parents must have the same number of columns. The second Matrix is
 * appended below the first. Neither parent is owned by the result.
 */
int numerus_matrix_create_join_vertical(
    numerus_matrix *parent,
    numerus_matrix *parent2,
    numerus_matrix **matrix
);

/**
 * Create a lazy scalar-multiplication view.
 *
 * The scalar is copied into the view; the parent is borrowed and must outlive
 * the result. IEEE-754 multiplication semantics apply, including NaN and
 * infinities. The output pointer is set to NULL before work begins.
 */
int numerus_matrix_create_scale(
    numerus_matrix *parent,
    double scalar,
    numerus_matrix **matrix
);

/**
 * Create a lazy view that adds a scalar to every matrix element.
 * The scalar is copied into the view and IEEE-754 addition semantics apply.
 */
int numerus_matrix_create_scalar_add(
    numerus_matrix *parent,
    double scalar,
    numerus_matrix **matrix
);

/**
 * Create a lazy view that subtracts a scalar from every matrix element.
 * This computes A[i,j] - scalar (not scalar - A[i,j]).
 */
int numerus_matrix_create_scalar_subtract(
    numerus_matrix *parent,
    double scalar,
    numerus_matrix **matrix
);

/** Create a lazy unary-negation view. */
int numerus_matrix_create_negate(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/**
 * Create a lazy element-wise sum of two equally sized Matrices.
 *
 * Both parent references are non-owning and must remain alive for the result's
 * lifetime. Addition follows IEEE-754 double semantics.
 */
int numerus_matrix_create_add(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create a lazy element-wise difference of two equally sized Matrices.
 *
 * Both parent references are non-owning and must remain alive for the result's
 * lifetime. Subtraction follows IEEE-754 double semantics.
 */
int numerus_matrix_create_subtract(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create a lazy Hadamard (element-wise) product of equally sized Matrices.
 *
 * This is element-wise multiplication, not matrix multiplication. Both parent
 * references are non-owning and must remain alive for the result's lifetime.
 * IEEE-754 double semantics apply.
 */
int numerus_matrix_create_hadamard_product(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create a lazy element-wise quotient of equally sized Matrices.
 *
 * Division follows IEEE-754 double semantics: zero divisors produce the
 * corresponding infinities or NaN rather than a Matrix status error. Both
 * parent references are non-owning and must remain alive for the result.
 */
int numerus_matrix_create_divide(
    numerus_matrix *left,
    numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create an independent dense Matrix containing element-wise minimum/maximum.
 * Inputs must have equal dimensions. NaN propagates from either operand.
 * The output pointer remains NULL on failure.
 */
int numerus_matrix_create_elementwise_min(
    const numerus_matrix *left, const numerus_matrix *right,
    numerus_matrix **matrix
);
/**
 * Create an independent dense Matrix containing element-wise maxima.
 * Inputs must have equal dimensions; NaN propagates from either operand.
 * The output pointer remains NULL on failure.
 */
int numerus_matrix_create_elementwise_max(
    const numerus_matrix *left, const numerus_matrix *right,
    numerus_matrix **matrix
);
/**
 * Clamp values to [lower, upper] in an independent dense Matrix. NaN values
 * propagate; NaN bounds or lower > upper are invalid. Infinite bounds are OK.
 */
int numerus_matrix_create_clamp(
    const numerus_matrix *source, double lower, double upper,
    numerus_matrix **matrix
);

/**
 * Create a lazy map view. Callback and context must remain deterministic and
 * logically unchanged for the view lifetime; parent is borrowed and must live.
 */
int numerus_matrix_create_map(
    const numerus_matrix *parent, numerus_value_transform_fn transform,
    const void *context, numerus_matrix **matrix
);
/**
 * Eagerly apply a deterministic callback into independent dense Storage.
 * Callback context is borrowed for the duration of this call only.
 */
int numerus_matrix_apply(
    const numerus_matrix *source, numerus_value_transform_fn transform,
    const void *context, numerus_matrix **matrix
);

/**
 * Normalize globally, by rows, or by columns using the selected norm. Inputs
 * must be finite; zero-norm vectors return DIVISION_BY_ZERO. Result is dense.
 */
int numerus_matrix_create_normalized(
    const numerus_matrix *source,
    numerus_matrix_normalize_axis axis,
    numerus_matrix_normalize_norm norm,
    numerus_matrix **matrix
);

/**
 * Materialize the logical values of a Matrix into independent dense Storage.
 *
 * Works for roots, nested views, joins, and binary arithmetic nodes. The result
 * owns its Storage and does not retain the source or its parents. On failure,
 * the output pointer remains NULL.
 */
int numerus_matrix_materialize(
    const numerus_matrix *source,
    numerus_matrix **matrix
);

/**
 * Compute the matrix product left × right into independent dense Storage.
 *
 * The left column count must equal the right row count. The operation is
 * materialized rather than a lazy view because a lazy getter would repeat the
 * dot product for every read. On failure, the output pointer remains NULL.
 */
int numerus_matrix_multiply(
    const numerus_matrix *left,
    const numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create a lazy view dividing every parent value by a scalar.
 *
 * A positive or negative zero divisor is rejected with
 * NUMERUS_MATRIX_DIVISION_BY_ZERO. Other values use IEEE-754 double
 * division semantics. The parent is borrowed and must outlive the view.
 */
int numerus_matrix_create_divide_scalar(
    numerus_matrix *parent,
    double divisor,
    numerus_matrix **matrix
);

/**
 * Compare two Matrices using exact C double equality.
 *
 * Shape mismatch is a successful comparison with result=false. NaN compares
 * unequal; equal infinities and signed zero follow C double == semantics.
 * The output is unchanged if a Matrix read fails.
 */
numerus_matrix_status numerus_matrix_is_equal(
    const numerus_matrix *left,
    const numerus_matrix *right,
    bool *equal
);

/**
 * Compare two Matrices using NUMERUS_EPSILON's combined tolerance.
 *
 * Shape mismatch is a successful comparison with result=false. NaN compares
 * unequal, and equal infinities compare equal. The output is unchanged if a
 * Matrix read fails.
 */
numerus_matrix_status numerus_matrix_all_close(
    const numerus_matrix *left, const numerus_matrix *right, bool *close
);
/**
 * Return true when at least one corresponding element pair is close under
 * NUMERUS_EPSILON's combined tolerance. Shape mismatch returns success/false;
 * NaN values compare unequal. Output is unchanged on read failure.
 */
numerus_matrix_status numerus_matrix_any_close(
    const numerus_matrix *left, const numerus_matrix *right, bool *close
);

/**
 * Compare all corresponding elements using NUMERUS_EPSILON's combined
 * tolerance. This is the all-elements predicate; use any_close() when one
 * matching element is sufficient. Shape mismatch returns success/false.
 */
numerus_matrix_status numerus_matrix_is_close(
    const numerus_matrix *left,
    const numerus_matrix *right,
    bool *close
);

/**
 * Create a 1×length row-vector Matrix from copied values.
 *
 * The result is an ordinary dense Matrix, not a separate Vector type.
 * Length must be positive and values must contain at least length elements.
 */
int numerus_matrix_create_row_vector(
    size_t length,
    const double *values,
    numerus_matrix **matrix
);

/**
 * Create a length×1 column-vector Matrix from copied values.
 *
 * The result is an ordinary dense Matrix, not a separate Vector type.
 * Length must be positive and values must contain at least length elements.
 */
int numerus_matrix_create_column_vector(
    size_t length,
    const double *values,
    numerus_matrix **matrix
);

/**
 * Compute base raised to a non-negative integer exponent.
 *
 * The base must be square. Exponent zero returns an identity Matrix; exponent
 * one returns an independent materialized copy. Larger powers use
 * exponentiation by squaring and return independent dense Storage.
 */
int numerus_matrix_power(
    const numerus_matrix *base,
    size_t exponent,
    numerus_matrix **matrix
);

/**
 * Evaluate c[0] + c[1]A + ... + c[count - 1]A^(count - 1) by Horner's
 * method. Coefficients are ordered from the constant term upward. The base
 * must be square and count must be positive; the result is independent dense
 * Storage. The output pointer remains NULL on failure.
 */
int numerus_matrix_polynomial(
    const numerus_matrix *base,
    const double *coefficients,
    size_t coefficient_count,
    numerus_matrix **matrix
);

/**
 * Compute exp(A) using scaling-and-squaring with a degree-13 Padé
 * approximant. Input must be square and finite; the result is independent
 * dense Storage. Non-finite intermediate results and solver failures are
 * reported as statuses, and the output pointer remains NULL on failure.
 */
int numerus_matrix_exponential(
    const numerus_matrix *matrix,
    numerus_matrix **exponential
);

/**
 * Compute base raised to a signed integer exponent.
 *
 * Negative exponents invert the square base once, then use exponentiation by
 * squaring. A singular or numerically singular base returns
 * NUMERUS_MATRIX_SINGULAR. The output is NULL on failure.
 */
int numerus_matrix_power_signed(
    const numerus_matrix *base,
    int64_t exponent,
    numerus_matrix **matrix
);

/**
 * Compute the Kronecker product left ⊗ right into independent dense Storage.
 *
 * Result dimensions are left.rows × right.rows by left.columns × right.columns.
 * All dimension, element-count, and byte-count arithmetic is checked. On
 * failure, the output pointer remains NULL.
 */
int numerus_matrix_kronecker_product(
    const numerus_matrix *left,
    const numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Compute the Kronecker sum left ⊕ right = left ⊗ I + I ⊗ right.
 *
 * Both inputs must be square. The result is an independent dense Matrix with
 * dimension left.rows * right.rows. Dimensions and allocation sizes are
 * checked; the output pointer remains NULL on failure.
 */
int numerus_matrix_kronecker_sum(
    const numerus_matrix *left,
    const numerus_matrix *right,
    numerus_matrix **matrix
);

/**
 * Create a lazy range slice of a Matrix.
 *
 * The slice contains row_count × column_count values starting at the specified
 * zero-based parent coordinates. Counts must be positive and the complete
 * range must fit inside the parent. The parent is borrowed and must outlive
 * the result.
 */
int numerus_matrix_create_slice(
    numerus_matrix *parent,
    size_t row_start,
    size_t row_count,
    size_t column_start,
    size_t column_count,
    numerus_matrix **matrix
);

/**
 * Select rows by a copied array of zero-based indices.
 *
 * Index order is preserved and repeated indices are allowed. The result is a
 * lazy view; the index array is copied, while the parent remains borrowed.
 */
int numerus_matrix_create_select_rows(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix **matrix
);

/**
 * Select columns by a copied array of zero-based indices.
 *
 * Index order is preserved and repeated indices are allowed. The result is a
 * lazy view; the index array is copied, while the parent remains borrowed.
 */
int numerus_matrix_create_select_columns(
    numerus_matrix *parent,
    const size_t *indices,
    size_t count,
    numerus_matrix **matrix
);

/**
 * Create a lazy row-major reshape view.
 *
 * The requested shape must contain exactly the same number of elements as
 * the parent. Logical traversal order is row-major. The parent is borrowed
 * and must outlive the view.
 */
int numerus_matrix_create_reshape(
    numerus_matrix *parent,
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
);

/**
 * Create a lazy row-vector view of all parent elements in row-major order.
 *
 * The result has shape 1 × (parent.rows * parent.columns). Element-count
 * overflow is reported as NUMERUS_MATRIX_OVERFLOW.
 */
int numerus_matrix_create_flatten(
    numerus_matrix *parent,
    numerus_matrix **matrix
);

/**
 * Create a lazy constant-padded view.
 *
 * top/bottom/left/right specify the number of rows or columns added on each
 * side. Existing values retain their positions; all padding cells use value.
 * The parent is borrowed and must outlive the view. Dimension overflow is
 * reported as NUMERUS_MATRIX_OVERFLOW.
 */
int numerus_matrix_create_pad(
    numerus_matrix *parent,
    size_t top,
    size_t bottom,
    size_t left,
    size_t right,
    double value,
    numerus_matrix **matrix
);

/** Create a lazy zero-padded view with the same shape and ownership rules. */
int numerus_matrix_create_zero_extend(
    numerus_matrix *parent,
    size_t top,
    size_t bottom,
    size_t left,
    size_t right,
    numerus_matrix **matrix
);

/**
 * Tile a Matrix lazily row_repetitions × column_repetitions times.
 *
 * Repetition counts must be positive. The parent is borrowed and must outlive
 * the view; output dimensions are checked for overflow.
 */
int numerus_matrix_create_repeat(
    numerus_matrix *parent,
    size_t row_repetitions,
    size_t column_repetitions,
    numerus_matrix **matrix
);

/**
 * Create a lazy block-diagonal Matrix containing repetitions copies of parent.
 *
 * Off-diagonal blocks are zero. The parent may be rectangular; result
 * dimensions are (parent.rows * repetitions) ×
 * (parent.columns * repetitions). Repetitions must be positive, and the
 * borrowed parent must outlive the view.
 */
int numerus_matrix_create_block_diagonal(
    numerus_matrix *parent,
    size_t repetitions,
    numerus_matrix **matrix
);

/**
 * Assemble a lazy block grid from a row-major array of Matrix pointers.
 *
 * Every block must be non-NULL. Blocks in each block row must have equal row
 * counts, and blocks in each block column must have equal column counts.
 * Missing/NULL blocks are rejected rather than implicitly treated as zeros.
 * The pointer array is copied; all block Matrices are borrowed and must
 * outlive the resulting view. Grid dimensions and cumulative output sizes are
 * checked for overflow.
 */
int numerus_matrix_create_block_grid(
    numerus_matrix *const *blocks,
    size_t block_row_count,
    size_t block_column_count,
    numerus_matrix **matrix
);

/**
 * Extract a diagonal into a 1×N row-vector view.
 *
 * Offset zero selects the main diagonal; positive offsets select diagonals
 * above it, and negative offsets select diagonals below it. An offset with no
 * elements in the Matrix returns NUMERUS_MATRIX_OUT_OF_BOUNDS. The parent is
 * borrowed and must outlive the result.
 */
int numerus_matrix_create_diagonal_extract(
    numerus_matrix *parent,
    ptrdiff_t offset,
    numerus_matrix **matrix
);

/**
 * Create a square diagonal view from a row or column vector.
 *
 * The main diagonal is selected by offset zero; positive offsets place values
 * above it and negative offsets below it. The output side length is the vector
 * length plus the absolute offset, checked for overflow. Off-diagonal elements
 * are zero. The vector is borrowed and must outlive the result.
 */
int numerus_matrix_create_diagonal_from_vector(
    numerus_matrix *vector,
    ptrdiff_t offset,
    numerus_matrix **matrix
);

/**
 * Permute all rows using a full zero-based permutation.
 *
 * The index count must equal the parent row count; every index must be in
 * range and appear exactly once. The index list is copied and the parent is
 * borrowed.
 */
int numerus_matrix_create_permute_rows(
    numerus_matrix *parent,
    const size_t *permutation,
    size_t count,
    numerus_matrix **matrix
);

/**
 * Permute all columns using a full zero-based permutation.
 *
 * The index count must equal the parent column count; every index must be in
 * range and appear exactly once. The index list is copied and the parent is
 * borrowed.
 */
int numerus_matrix_create_permute_columns(
    numerus_matrix *parent,
    const size_t *permutation,
    size_t count,
    numerus_matrix **matrix
);

/** Create a dense Matrix. */
int numerus_matrix_create_dense(
    size_t rows,
    size_t columns,
    const double *values,
    numerus_matrix **matrix
);

/** Create an identity Matrix. */
int numerus_matrix_create_identity(size_t size, numerus_matrix **matrix);

/** Create a zero Matrix. */
int numerus_matrix_create_zero(
    size_t rows,
    size_t columns,
    numerus_matrix **matrix
);

/** Create a square zero Matrix. */
int numerus_matrix_create_zero_square(size_t size, numerus_matrix **matrix);

/** Create a diagonal Matrix. */
int numerus_matrix_create_diagonal(
    size_t size,
    const double *values,
    numerus_matrix **matrix
);

/** Create a constant-value Matrix. */
int numerus_matrix_create_constant(
    size_t rows,
    size_t columns,
    double value,
    numerus_matrix **matrix
);

/** Create a scaled-identity Matrix. */
int numerus_matrix_create_scaled_identity(
    size_t size,
    double value,
    numerus_matrix **matrix
);

/** Create an upper-triangular Matrix. */
int numerus_matrix_create_upper_triangular(
    size_t size,
    const double *values,
    numerus_matrix **matrix
);

/** Create a lower-triangular Matrix. */
int numerus_matrix_create_lower_triangular(
    size_t size,
    const double *values,
    numerus_matrix **matrix
);

/** Create a sparse Matrix with a default value and explicit overrides. */
int numerus_matrix_create_sparse(
    size_t rows,
    size_t columns,
    double default_value,
    const numerus_storage_sparse_entry *entries,
    size_t count,
    numerus_matrix **matrix
);

/** Create a symmetric Matrix. */
int numerus_matrix_create_symmetric(
    size_t size,
    const double *values,
    numerus_matrix **matrix
);

/** Create a banded Matrix. */
int numerus_matrix_create_banded(
    size_t rows,
    size_t columns,
    size_t lower_bandwidth,
    size_t upper_bandwidth,
    const double *values,
    numerus_matrix **matrix
);

/**
 * @brief Compute or retrieve cached structural properties.
 *
 * Properties describe the logical values exposed by this Matrix. The output
 * is unchanged if the scan fails.
 */
numerus_matrix_status numerus_matrix_get_flags(
    numerus_matrix *matrix,
    numerus_matrix_flags *flags
);

/**
 * @brief Compute or retrieve the cached determinant of a square Matrix.
 *
 * Uses partial-pivoted Gaussian elimination in the general case, with cheaper
 * paths for zero, identity, or triangular matrices when structural properties
 * have already been cached. Successful results, including zero, are cached. Failures are not.
 * The output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_determinant(
    numerus_matrix *matrix,
    double *determinant
);

/**
 * Estimate numerical rank using scale-aware, complete-pivoting elimination.
 *
 * The threshold is NUMERUS_EPSILON * max(rows, columns) relative to the
 * largest absolute input element. NaN/infinity input and non-finite
 * elimination intermediates return NUMERUS_MATRIX_NON_FINITE. The output is
 * unchanged on any failure.
 */
/**
 * Materialize a row-echelon form using scale-aware pivoting.
 * The result is independent dense Storage. Non-finite input or intermediates
 * return NUMERUS_MATRIX_NON_FINITE; on failure, *result remains NULL.
 */
numerus_matrix_status numerus_matrix_row_echelon_form(
    const numerus_matrix *matrix,
    numerus_matrix **result
);

/**
 * Materialize a reduced row-echelon form using scale-aware pivoting.
 * The result is independent dense Storage. Non-finite input or intermediates
 * return NUMERUS_MATRIX_NON_FINITE; on failure, *result remains NULL.
 */
numerus_matrix_status numerus_matrix_reduced_row_echelon_form(
    const numerus_matrix *matrix,
    numerus_matrix **result
);

/**
 * Compute an inverse through LU factorization and triangular solves.
 * Numerical singularity uses numerus_matrix_rank() and its scale-aware
 * threshold. The returned Matrix owns independent dense Storage. On failure,
 * *inverse remains NULL; singular inputs return NUMERUS_MATRIX_SINGULAR.
 */
/**
 * Solve A X = B for square, numerically nonsingular A.
 *
 * B may contain one or multiple right-hand-side columns. One LU
 * factorization is reused across all columns. The result is an independent
 * dense Matrix; on failure, *solution remains NULL.
 */
/**
 * Estimate the matrix 1-norm condition number ||A||_1 * ||A^-1||_1.
 *
 * The estimate is based on the implemented inverse and may be infinity for
 * singular/numerically singular matrices or floating-point overflow. This is
 * an estimate of numerical conditioning, not a proof of forward accuracy.
 * The output is unchanged on failure.
 */
/**
 * Compute a reduced Householder QR factorization A = Q R.
 *
 * Q has shape rows(A) × min(rows(A), columns(A)); R has shape
 * min(rows(A), columns(A)) × columns(A). Q has orthonormal columns up to
 * floating-point error. This is unpivoted QR and does not provide a
 * rank-revealing permutation. Both outputs own independent dense Storage.
 * On failure, both output pointers remain NULL.
 */
/**
 * Compute a basis for the numerical null space of a Matrix.
 *
 * For nullity > 0, *basis is a dense Matrix with one basis vector per column.
 * For full-column-rank input, the null space is trivial and the successful
 * result is *basis == NULL with *nullity == 0; Matrix dimensions remain
 * strictly positive. Numerical rank uses NUMERUS_EPSILON scaled by the
 * larger input dimension and the largest absolute input element.
 *
 * On failure, *basis remains NULL and *nullity remains unchanged.
 */
/**
 * Return a basis for the numerical column space of a Matrix.
 *
 * The basis columns are selected from the input columns using column-pivoted
 * Householder QR. If the numerical rank is zero, success is represented by
 * *basis == NULL and *dimension == 0. On failure, *basis is NULL and
 * *dimension is unchanged.
 */
numerus_matrix_status numerus_matrix_column_space_basis(
    const numerus_matrix *matrix,
    numerus_matrix **basis,
    size_t *dimension
);

/**
 * Return a basis for the numerical row space of a Matrix.
 *
 * The basis rows are selected from the input rows using column-pivoted QR of
 * the transpose. If the numerical rank is zero, success is represented by
 * *basis == NULL and *dimension == 0. On failure, *basis is NULL and
 * *dimension is unchanged.
 */
numerus_matrix_status numerus_matrix_row_space_basis(
    const numerus_matrix *matrix,
    numerus_matrix **basis,
    size_t *dimension
);

/**
 * Return a basis for the numerical null space as columns.
 * For a trivial null space, success is represented by *basis == NULL and
 * *nullity == 0. On failure, *basis remains NULL and *nullity is unchanged.
 */
numerus_matrix_status numerus_matrix_null_space(
    const numerus_matrix *matrix,
    numerus_matrix **basis,
    size_t *nullity
);

/**
 * Compute the lower-triangular Cholesky factor L for a symmetric positive-
 * definite Matrix, so A is approximately L * transpose(L).
 *
 * Symmetry is checked relative to the largest absolute input element.
 * Positive definiteness uses a scale-aware pivot threshold. On failure,
 * *lower remains NULL.
 */
/**
 * Compute an unpivoted LDL^T factorization for a symmetric Matrix.
 *
 * A is approximately L * D * transpose(L), where L is unit lower triangular
 * and D is diagonal. Symmetry and pivot checks use scale-aware tolerances.
 * This implementation supports matrices whose leading pivots remain safely
 * nonzero; it does not pivot and can reject nonsingular matrices that require
 * symmetric pivoting. Both outputs remain NULL on failure.
 */
/**
 * Compute a reduced real SVD: A = U * Sigma * Vt.
 *
 * If k = min(rows(A), columns(A)), U has shape rows(A)-by-k, singular_values
 * is a k-by-1 column vector sorted descending and nonnegative, and Vt has
 * shape k-by-columns(A). U's columns and Vt's rows are orthonormal within
 * floating-point error. Outputs are independent dense Matrices; all are NULL
 * on failure.
 */
/**
 * Compute the eigendecomposition of a real symmetric Matrix.
 *
 * Eigenvalues are returned as an n-by-1 column vector sorted in descending
 * algebraic order. Eigenvectors are the corresponding columns of an n-by-n
 * orthonormal Matrix V, so A is approximately V * D * transpose(V).
 * Symmetry and convergence use scale-aware tolerances. Outputs remain NULL
 * on failure.
 */
/**
 * Compute the spectral norm (largest singular value) of any real Matrix.
 * The scalar output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_spectral_norm(
    const numerus_matrix *matrix,
    double *norm
);

/**
 * Compute the spectral radius for a real symmetric Matrix only.
 * The radius is the largest absolute eigenvalue. Non-symmetric input returns
 * NUMERUS_MATRIX_NOT_SYMMETRIC; output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_symmetric_spectral_radius(
    const numerus_matrix *matrix,
    double *radius
);

/**
 * Compute real eigenvalues and orthonormal eigenvectors for a symmetric Matrix.
 * Eigenvalues are descending in algebraic order; eigenvectors are columns.
 * Non-symmetric input and non-convergence return explicit statuses; outputs
 * remain NULL on failure.
 */
numerus_matrix_status numerus_matrix_symmetric_eigen(
    const numerus_matrix *matrix,
    numerus_matrix **eigenvalues,
    numerus_matrix **eigenvectors
);

/**
 * Compute the principal square root of a real symmetric positive-semidefinite
 * matrix by spectral decomposition. Small negative eigenvalues within the
 * scale-relative tolerance are clamped to zero; materially negative values
 * return NUMERUS_MATRIX_NOT_POSITIVE_SEMIDEFINITE.
 */
numerus_matrix_status numerus_matrix_symmetric_square_root(
    const numerus_matrix *matrix,
    numerus_matrix **square_root
);

/**
 * Compute the real matrix logarithm of a symmetric positive-definite matrix
 * by spectral decomposition. Eigenvalues at or below the scale-relative
 * positivity tolerance return NUMERUS_MATRIX_NOT_POSITIVE_DEFINITE.
 */
numerus_matrix_status numerus_matrix_symmetric_logarithm(
    const numerus_matrix *matrix,
    numerus_matrix **logarithm
);

/** Compute sin(A) for a real symmetric Matrix using its spectral decomposition. */
numerus_matrix_status numerus_matrix_symmetric_sine(
    const numerus_matrix *matrix,
    numerus_matrix **sine
);

/** Compute cos(A) for a real symmetric Matrix using its spectral decomposition. */
numerus_matrix_status numerus_matrix_symmetric_cosine(
    const numerus_matrix *matrix,
    numerus_matrix **cosine
);


/**
 * Compute the Moore-Penrose pseudoinverse using the reduced real SVD.
 *
 * Singular values at or below NUMERUS_EPSILON * max(rows, columns) *
 * largest_singular_value are treated as zero. The output has shape
 * columns(A)-by-rows(A), is independent dense Storage, and remains NULL
 * on failure.
 */
numerus_matrix_status numerus_matrix_pseudoinverse(
    const numerus_matrix *matrix,
    numerus_matrix **pseudoinverse
);

/**
 * Compute the reduced real SVD A = U * Sigma * Vᵀ.
 * Singular values are nonnegative and sorted descending. U, the singular-value
 * column vector, and Vᵀ are independent dense outputs; all remain NULL on failure.
 */
numerus_matrix_status numerus_matrix_svd(
    const numerus_matrix *matrix,
    numerus_matrix **u,
    numerus_matrix **singular_values,
    numerus_matrix **vt
);

/**
 * Compute an unpivoted LDLᵀ factorization of a symmetric Matrix.
 * Matrices requiring symmetric pivoting may return PIVOT_TOO_SMALL despite
 * being nonsingular. Both outputs remain NULL on failure.
 */
numerus_matrix_status numerus_matrix_ldlt_decompose(
    const numerus_matrix *matrix,
    numerus_matrix **lower,
    numerus_matrix **diagonal
);

/**
 * Compute lower-triangular L such that A is approximately L * Lᵀ.
 * The input must be symmetric positive definite under the documented numerical
 * checks. The output remains NULL on failure.
 */
numerus_matrix_status numerus_matrix_cholesky(
    const numerus_matrix *matrix,
    numerus_matrix **lower
);

/**
 * Classify a finite real symmetric Matrix as positive definite and/or positive
 * semidefinite using eigenvalues and a scale-relative tolerance.
 * Eigenvalues within NUMERUS_EPSILON × n × max(|lambda_i|) count as zero.
 * Both outputs are unchanged on failure and must be distinct.
 */
numerus_matrix_status numerus_matrix_classify_definiteness(
    const numerus_matrix *matrix,
    int *positive_definite,
    int *positive_semidefinite
);

/**
 * Compute reduced, unpivoted Householder QR: A = Q * R.
 * Q is rows(A)×min(rows(A), columns(A)); R is min(rows(A), columns(A))×columns(A).
 * This API is not rank-revealing. Both outputs remain NULL on failure.
 */
numerus_matrix_status numerus_matrix_qr_decompose(
    const numerus_matrix *matrix,
    numerus_matrix **q,
    numerus_matrix **r
);

/**
 * Estimate the 1-norm condition number ||A||₁ ||A⁻¹||₁.
 * This implementation uses the inverse-based estimate; singular/numerically
 * singular inputs can yield infinity. The scalar output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_condition_estimate_one(
    const numerus_matrix *matrix,
    double *condition_estimate
);

/**
 * Classify the numerical solutions to A X = B.
 *
 * Supports rectangular A and multiple right-hand-side columns in B. Consistency
 * is tested by projecting each RHS onto the numerical column space obtained
 * from column-pivoted QR. The classification output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_classify_system(
    const numerus_matrix *matrix,
    const numerus_matrix *right_hand_side,
    numerus_matrix_solution_kind *classification
);

/**
 * Solve the least-squares problem min_X ||A X - B||_2 using pivoted QR.
 *
 * Requires A to have full column rank; supports overdetermined and square
 * systems, and multiple right-hand-side columns. The result is a dense
 * columns(A)-by-columns(B) Matrix. Rank-deficient or underdetermined inputs
 * return NUMERUS_MATRIX_RANK_DEFICIENT; no normal equations or inverse are
 * formed. On failure, *solution remains NULL.
 */
/**
 * Solve weighted least squares: minimize sum_i weights[i] * ||A_i X - B_i||².
 *
 * weights_count must equal rows(A); weights must be finite and nonnegative.
 * Zero weights exclude rows. The implementation scales rows by sqrt(weight)
 * and uses the QR least-squares solver, not normal equations. On failure,
 * *solution remains NULL.
 */
numerus_matrix_status numerus_matrix_weighted_least_squares(
    const numerus_matrix *matrix,
    const numerus_matrix *right_hand_side,
    const double *weights,
    size_t weights_count,
    numerus_matrix **solution
);

/**
 * Solve min_X ||A X - B||₂ using column-pivoted QR.
 * Requires full column rank and supports overdetermined/square A and multiple
 * RHS columns. Rank-deficient or underdetermined inputs return RANK_DEFICIENT.
 */
numerus_matrix_status numerus_matrix_least_squares(
    const numerus_matrix *matrix,
    const numerus_matrix *right_hand_side,
    numerus_matrix **solution
);

/**
 * Solve A X = B for square, numerically nonsingular A using one LU factorization.
 * Multiple RHS columns share that factorization. The result is independent
 * dense Storage; the output remains NULL on failure.
 */
numerus_matrix_status numerus_matrix_solve(
    const numerus_matrix *matrix,
    const numerus_matrix *right_hand_side,
    numerus_matrix **solution
);

/**
 * Compute an inverse using Gauss–Jordan elimination with partial pivoting.
 * This alternative is primarily useful for comparison and education; the
 * LU-based numerus_matrix_inverse() remains the default production path.
 * Singular and non-finite inputs return explicit statuses; output stays NULL
 * on failure.
 */
numerus_matrix_status numerus_matrix_inverse_gauss_jordan(
    const numerus_matrix *matrix,
    numerus_matrix **inverse
);

numerus_matrix_status numerus_matrix_inverse(
    const numerus_matrix *matrix,
    numerus_matrix **inverse
);

/**
 * Estimate numerical rank using scale-aware complete-pivoting elimination.
 * This is a tolerance-dependent numerical classification, not symbolic rank.
 * The output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_rank(
    const numerus_matrix *matrix,
    size_t *rank
);

/**
 * Return whether a square Matrix is skew-symmetric within NUMERUS_EPSILON.
 * A non-square Matrix returns success with false. Output is unchanged on read
 * failure.
 */
numerus_matrix_status numerus_matrix_is_skew_symmetric(
    const numerus_matrix *matrix,
    bool *is_skew_symmetric
);

/**
 * Return whether a square Matrix has orthonormal rows within
 * NUMERUS_EPSILON. A non-square Matrix returns success with false. This check
 * uses O(n^3) time and O(1) auxiliary memory; output is unchanged on read
 * failure.
 */
numerus_matrix_status numerus_matrix_is_orthogonal(
    const numerus_matrix *matrix,
    bool *is_orthogonal
);

/**
 * Return whether any element is NaN. The output is unchanged if a read fails.
 * The scan stops at the first matching element.
 */
numerus_matrix_status numerus_matrix_has_nan(
    const numerus_matrix *matrix,
    bool *has_nan
);

/**
 * Return whether any element is positive or negative infinity. The output is
 * unchanged if a read fails; the scan stops at the first matching element.
 */
numerus_matrix_status numerus_matrix_has_infinity(
    const numerus_matrix *matrix,
    bool *has_infinity
);

/**
 * Return whether every element is finite (neither NaN nor infinity).
 * The output is unchanged if a read fails; the scan stops at the first
 * non-finite element.
 */
numerus_matrix_status numerus_matrix_is_finite(
    const numerus_matrix *matrix,
    bool *is_finite
);

/**
 * Compute the Frobenius norm using a stable hypot-based accumulation.
 * If any element is NaN, the result is NaN; infinities otherwise follow
 * IEEE-754 behavior. Output is unchanged if a read fails.
 */
numerus_matrix_status numerus_matrix_norm_frobenius(
    const numerus_matrix *matrix,
    double *norm
);

/**
 * Compute the induced matrix 1-norm (maximum absolute column sum).
 * Any NaN element makes the result NaN; output is unchanged on read failure.
 */
numerus_matrix_status numerus_matrix_norm_one(
    const numerus_matrix *matrix,
    double *norm
);

/**
 * Compute the induced matrix infinity-norm (maximum absolute row sum).
 * Any NaN element makes the result NaN; output is unchanged on read failure.
 */
numerus_matrix_status numerus_matrix_norm_infinity(
    const numerus_matrix *matrix,
    double *norm
);

/**
 * Compute the trace of a square Matrix.
 *
 * Diagonal elements are accumulated in increasing index order using ordinary
 * double addition. NaN and infinities follow IEEE-754 arithmetic. The output
 * is unchanged if validation or any element read fails.
 */
numerus_matrix_status numerus_matrix_trace(
    const numerus_matrix *matrix,
    double *trace
);

/**
 * Compute the sum of all elements using row-major double accumulation.
 * NaN and infinities follow IEEE-754 arithmetic; output is unchanged on error.
 */
numerus_matrix_status numerus_matrix_sum(
    const numerus_matrix *matrix,
    double *sum
);

/**
 * Compute the minimum element. If any element is NaN, the result is NaN.
 * Output is unchanged if an element read fails.
 */
numerus_matrix_status numerus_matrix_min(
    const numerus_matrix *matrix,
    double *minimum
);

/**
 * Compute the maximum element. If any element is NaN, the result is NaN.
 * Output is unchanged if an element read fails.
 */
numerus_matrix_status numerus_matrix_max(
    const numerus_matrix *matrix,
    double *maximum
);

/** Compute the arithmetic mean using row-major sum divided by element count. */
numerus_matrix_status numerus_matrix_variance(const numerus_matrix *matrix, bool sample, double *variance);
/**
 * Compute the population or sample standard deviation.
 *
 * Uses the corresponding variance definition selected by sample and returns
 * its square root. Input elements must be finite; sample mode requires at
 * least two elements. The output is unchanged on failure.
 */
numerus_matrix_status numerus_matrix_standard_deviation(const numerus_matrix *matrix, bool sample, double *standard_deviation);

/**
 * Compute the arithmetic mean using row-major double accumulation.
 * NaN and infinities follow IEEE-754 arithmetic; output is unchanged on error.
 */
numerus_matrix_status numerus_matrix_mean(
    const numerus_matrix *matrix,
    double *mean
);

/**
 * Return per-row sums as an independent rows×1 dense Matrix.
 * NaN and infinities follow IEEE-754 arithmetic.
 */
numerus_matrix_status numerus_matrix_row_sums(
    const numerus_matrix *matrix,
    numerus_matrix **sums
);

/**
 * Return per-column sums as an independent 1×columns dense Matrix.
 * NaN and infinities follow IEEE-754 arithmetic.
 */
numerus_matrix_status numerus_matrix_column_sums(
    const numerus_matrix *matrix,
    numerus_matrix **sums
);

/** Return per-row arithmetic means as an independent rows×1 dense Matrix. */
numerus_matrix_status numerus_matrix_row_means(
    const numerus_matrix *matrix,
    numerus_matrix **means
);

/** Return per-column arithmetic means as an independent 1×columns dense Matrix. */
numerus_matrix_status numerus_matrix_column_means(
    const numerus_matrix *matrix,
    numerus_matrix **means
);

/** Read one Matrix element with validation. */
numerus_matrix_status numerus_matrix_get(
    const numerus_matrix *matrix,
    size_t row,
    size_t column,
    double *value
);

/**
 * @brief Read one Matrix element without validation.
 *
 * This is the hot-path accessor for callers that already validated the
 * Matrix and coordinates.
 */
numerus_matrix_status numerus_matrix_get_unchecked(
    const numerus_matrix *matrix,
    size_t row,
    size_t column,
    double *value
);

/** Return the number of rows, or zero for NULL. */
size_t numerus_matrix_rows(const numerus_matrix *matrix);

/** Return the number of columns, or zero for NULL. */
size_t numerus_matrix_columns(const numerus_matrix *matrix);

/** Return the Storage kind of the root Matrix, or ZERO for NULL. */
numerus_storage_kind numerus_matrix_storage_kind(
    const numerus_matrix *matrix
);

/**
 * @brief Release a Matrix object.
 *
 * A root Matrix releases its owned Storage. A child Matrix only releases its
 * own Matrix object; its parent remains untouched.
 */
void numerus_matrix_destroy(numerus_matrix *matrix);

#endif
