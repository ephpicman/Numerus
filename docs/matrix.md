# Matrix

Matrix is Numerus's internal logical matrix layer above Storage. It provides a
uniform coordinate-based interface while allowing roots, derived views, and
joined matrices to share data without copying it.

Matrix is currently an internal C API, not a PHP-facing object API. The
function-level contract is declared in [`numerus_matrix.h`](../numerus_matrix.h).

## Object model

A Matrix has positive row and column counts and is one of four forms:

- **Root Matrix:** owns an immutable `numerus_storage`.
- **Derived view:** refers to one parent Matrix and applies coordinate and/or
  value transforms when an element is read.
- **Joined view:** refers to two parent Matrices and delegates each read to the
  appropriate parent.
- **Binary arithmetic view:** refers to two equally shaped parents and combines
  their corresponding values on each read.

The Matrix structure is opaque. Callers cannot access or change its internal
fields through the public header.

## Storage representations

Root construction delegates to Storage. Supported kinds are dense,
upper-triangular, lower-triangular, diagonal, identity, constant, zero,
scaled-identity, sparse, symmetric, and banded. Representation-specific
constraints and input layouts are documented in [Storage](storage.md).

Use the general constructor when a representation's full parameter set is
needed:

```c
numerus_matrix_data data = {0};
numerus_matrix *matrix = NULL;

data.values = values;
int status = numerus_matrix_create(
    NUMERUS_STORAGE_DENSE, rows, columns, &data, &matrix
);
```

Convenience constructors are also available for each representation, including
`numerus_matrix_create_dense()`, `numerus_matrix_create_identity()`,
`numerus_matrix_create_sparse()`, and `numerus_matrix_create_banded()`.
The root owns the Storage created for it. Constructor input buffers are copied
by Storage and are not retained.

Dimensions must be positive. Constructors that use square-only representations
return `NUMERUS_MATRIX_NOT_SQUARE` when rows and columns differ.

## Lazy views

Views do not materialise a second matrix buffer. A read maps child coordinates
to parent coordinates, reads the parent value, and optionally transforms that
value.

Supported convenience views:

| Constructor | Result |
| --- | --- |
| `numerus_matrix_create_transpose()` | Rows and columns are swapped |
| `numerus_matrix_create_flip_rows()` | Row order is reversed |
| `numerus_matrix_create_flip_columns()` | Column order is reversed |
| `numerus_matrix_create_rotate_90_clockwise()` | Clockwise quarter-turn; dimensions are swapped |
| `numerus_matrix_create_rotate_180()` | Half-turn; dimensions are unchanged |
| `numerus_matrix_create_rotate_90_counterclockwise()` | Counter-clockwise quarter-turn; dimensions are swapped |
| `numerus_matrix_create_remove_row(parent, row, ...)` | Omits one zero-based row; result has one fewer row |
| `numerus_matrix_create_remove_column(parent, column, ...)` | Omits one zero-based column; result has one fewer column |
| `numerus_matrix_create_swap_rows(parent, row1, row2, ...)` | Exchanges two zero-based row positions |
| `numerus_matrix_create_swap_columns(parent, column1, column2, ...)` | Exchanges two zero-based column positions |
| `numerus_matrix_create_scale(parent, scalar, ...)` | Multiplies each logical value by a copied scalar |
| `numerus_matrix_create_negate(parent, ...)` | Negates each logical value |
| `numerus_matrix_create_divide_scalar(parent, divisor, ...)` | Divides each logical value by a nonzero scalar |

Row/column removal and swaps are coordinate transforms. They do not mutate the
parent or copy element data. Removing the only row or only column is rejected
with `NUMERUS_MATRIX_INVALID_ARGUMENT`, because Matrix dimensions must remain
positive. An index outside the corresponding parent dimension returns
`NUMERUS_MATRIX_OUT_OF_BOUNDS`. Swapping an index with itself is valid and
produces an identity mapping.

The convenience constructors above use the generic coordinate-transform
mechanism internally. Use `numerus_matrix_create_from_parent_with_transforms()`
when a custom coordinate mapping or value transformation is needed.

Views can be composed: a view can use another view or a joined Matrix as its
parent. This composes the read mapping without copying element data.

### Custom transforms

`numerus_matrix_create_from_parent_with_transforms()` accepts optional
coordinate and value callbacks. A NULL coordinate callback uses identity
coordinates; a NULL value callback passes the parent value through unchanged.

Callbacks must write their output parameters only on success and return a
`numerus_matrix_status`. A callback error is propagated to the caller, and
the checked getter leaves the caller's output value unchanged on failure.

The context pointer is borrowed: Matrix neither owns nor frees it. It must
remain valid and logically unchanged for the entire lifetime of the child.
Callbacks must be deterministic and must not depend on mutable external state.
The API uses `const void *` so callbacks cannot mutate the context through that
pointer; `const` does not prevent mutation through another alias. If a context
contains pointers to mutable data, the caller remains responsible for preserving
the logical immutability contract.

A custom transform can be logically mutable if its callback depends on mutable
external state; that violates the API contract even though the Matrix object's
fields themselves are opaque.

The scalar multiplication and negation convenience views store their scalar inside
the view itself, so they do not borrow a caller-owned scalar or callback context.
Their parent Matrix is still non-owning and must outlive the view. They use normal
IEEE-754 `double` multiplication semantics: zero, signed zero, NaN, and infinities
are not converted into status errors.

## Joins

Joins are lazy and hold two non-owning parent references.

- **Horizontal join:** appends the second Matrix to the right of the first.
  Parents must have equal row counts. Result rows equal the parent row count,
  and result columns are the sum of their column counts.
- **Vertical join:** appends the second Matrix below the first. Parents must
  have equal column counts. Result columns equal the parent column count, and
  result rows are the sum of their row counts.

Dimension addition is checked for `size_t` overflow. Incompatible dimensions
return `NUMERUS_MATRIX_DIMENSION_MISMATCH`; overflow returns
`NUMERUS_MATRIX_OVERFLOW`. Nested joins remain lazy.

## Matrix multiplication

`numerus_matrix_multiply(left, right, &result)` computes the mathematical
matrix product, not the element-wise Hadamard product. The left column count
must equal the right row count; a mismatch returns
`NUMERUS_MATRIX_DIMENSION_MISMATCH`. The implementation materializes an
independent dense result because a lazy getter would repeat a complete dot
product for every element read.

The current baseline uses the standard row/column/inner triple loop, accumulates
each dot product left-to-right in `double`, and runs in O(m × n × k) time for
an m×k left operand and k×n right operand. The output buffer uses O(m × n)
temporary memory in addition to the independent dense Storage. Result dimension
and byte-count arithmetic are checked before allocation. Parent read failures
are propagated and the output pointer remains NULL on failure. Non-finite
values follow IEEE-754 arithmetic; they are not status errors.

## Materialization

`numerus_matrix_materialize(source, &result)` copies the logical values exposed
by any Matrix node into a new dense, Storage-backed Matrix. It supports roots,
nested coordinate/value views, joins, and lazy binary arithmetic nodes. The
result owns its Storage and does not retain the source or its parents.

The operation checks both element-count and byte-count arithmetic before
allocation. It reads through the checked getter and propagates callback/parent
errors. If allocation or any read fails, the temporary buffer is released and
the output pointer remains NULL; a partially populated result is never
published. Materialization has O(rows × columns) time and O(rows × columns)
temporary memory, in addition to the independent dense result Storage.

## Exact and approximate equality

`numerus_matrix_is_equal()` compares corresponding values with exact C
`double ==` semantics. `numerus_matrix_is_close()` uses
`numerus_double_equals()` from `numerus_numeric.h`, with the established
combined absolute/relative tolerance (`NUMERUS_EPSILON = 1e-9`). These are
separate operations: approximate equality must not silently replace exact
equality.

For both operations, a shape mismatch is a successful comparison with a
`false` result, not a status error. NaN compares unequal to itself in both
modes; equal infinities compare equal, and positive/negative zero compare equal.
If a parent read fails, the status is propagated and the caller's boolean
output is unchanged. Null arguments return `NUMERUS_MATRIX_INVALID_ARGUMENT`.

## Scalar division

`numerus_matrix_create_divide_scalar(parent, divisor, &result)` returns a lazy
value-transform view for `parent / divisor`. Unlike element-wise division,
scalar division rejects positive or negative zero with
`NUMERUS_MATRIX_DIVISION_BY_ZERO`; the output pointer remains NULL. Nonzero
divisors, including NaN and infinities, follow IEEE-754 `double` semantics.
This is direct division by the scalar, not multiplication by a precomputed
reciprocal, so rounding follows the requested operation. The parent is
non-owning and must outlive the view.

## Element-wise addition and subtraction

`numerus_matrix_create_add()`, `numerus_matrix_create_subtract()`,
`numerus_matrix_create_hadamard_product()`, and `numerus_matrix_create_divide()`
create lazy two-parent nodes. Both parents must have identical row and column
counts; otherwise the constructor returns `NUMERUS_MATRIX_DIMENSION_MISMATCH`
and leaves the output pointer NULL. Each read obtains the corresponding value
from both parents, propagates either parent's read failure, then applies ordinary
IEEE-754 `double` addition, subtraction, multiplication, or division. Hadamard
multiplication is element-wise and is not the dot-product-based matrix
multiplication operation.

Element-wise division intentionally follows IEEE-754 behavior: a nonzero value
divided by positive or negative zero produces the corresponding signed
infinity; zero divided by zero and infinity divided by infinity produce NaN;
NaN inputs propagate. These numeric results do not become Matrix status errors.
The operations do not mutate or materialize either input. Both parents must
outlive the result.

## Ownership and lifetime

Ownership is deliberately simple but important:

- A root Matrix owns and destroys its Storage.
- A derived view does not own its parent.
- A joined view does not own either parent.
- A binary arithmetic view does not own either parent.
- Destroying a view does not destroy its parent.
- Every parent must remain alive while any dependent view can be read.

The C caller is responsible for satisfying these lifetime rules. The planned
PHP object layer must retain parent objects (including both parents of a join)
so that the underlying C pointers cannot outlive their targets.

Destroying a parent while a child or joined Matrix still refers to it leaves a
dangling pointer and makes later reads invalid. Do not rely on destruction
order being inferred or managed by the C Matrix API.

## Reading values and errors

`numerus_matrix_get()` validates the Matrix pointer, output pointer, and
coordinates. It returns a `numerus_matrix_status` and writes the output value
only on success.

Relevant statuses:

- `NUMERUS_MATRIX_SUCCESS`
- `NUMERUS_MATRIX_INVALID_ARGUMENT`
- `NUMERUS_MATRIX_OVERFLOW`
- `NUMERUS_MATRIX_OUT_OF_MEMORY`
- `NUMERUS_MATRIX_OUT_OF_BOUNDS`
- `NUMERUS_MATRIX_NOT_SQUARE`
- `NUMERUS_MATRIX_DIMENSION_MISMATCH`
- `NUMERUS_MATRIX_DIVISION_BY_ZERO`

A numeric zero is a valid value, not an error signal.

`numerus_matrix_get_unchecked()` skips validation and is intended for callers
that have already established valid pointers and coordinates. Despite its name,
it returns a status because custom callbacks and parent reads can still fail.
Only use it when the Matrix and coordinate preconditions are satisfied.

The metadata functions `numerus_matrix_rows()` and
`numerus_matrix_columns()` return zero for a NULL pointer.
`numerus_matrix_storage_kind()` reports the root Storage kind for ordinary
derived views and returns `NUMERUS_STORAGE_ZERO` for NULL. For a joined or binary arithmetic Matrix,
the implementation currently follows the first parent; therefore, this value
does not describe both sides of the node and should not be treated as a complete
summary of its input representations.


## Cached analysis

Each Matrix has a per-object analysis cache. It stores structural metadata and
scalar results, not element values, so it does not change the lazy-view model.

`numerus_matrix_get_flags()` computes and caches structural properties:
square, zero, diagonal, upper-triangular, lower-triangular, symmetric, and
identity. These flags describe the logical values exposed by that Matrix,
including views and joins. Floating-point classification uses the comparison
policy in `numerus_numeric.h` (`NUMERUS_EPSILON = 1e-9`): zero checks use an
absolute tolerance, while equality checks use combined absolute/relative
tolerance. A square zero Matrix is also diagonal, upper-triangular,
lower-triangular, and symmetric.

`numerus_matrix_determinant()` caches a successfully computed determinant.
Structural flags use `NUMERUS_EPSILON` (`1e-9`) with an absolute tolerance
for zero checks and a scale-aware combined tolerance for equality checks.
The determinant elimination pivot check remains exact-zero: applying a fixed
epsilon there would incorrectly classify small but valid matrices as singular.
The general path uses Gaussian elimination with partial pivoting. If flags
have already been computed, zero, identity, and triangular matrices can use
cheaper paths. A determinant of zero is a valid cached result. Allocation
failures and element-read failures are not cached and can be retried.

Each Matrix has its own cache. A View or Join does not reuse its parent's
analysis cache because its logical values may differ. These APIs do not
establish thread safety: concurrent access while a cache is being populated is
not guaranteed safe. Keep the Matrix and all parents alive during every call.

## Concurrency

This documentation does not promise general thread safety or concurrent-read
safety. Immutability prevents mutation through the Storage API, but it does not
manage parent lifetime, synchronise custom callbacks, or make concurrent
destruction safe. Keep objects alive for the duration of every operation and
supply callbacks/context that satisfy their documented lifetime and
determinism requirements.

## Native tests

The native Matrix tests cover storage-backed constructors, orientation
transforms, row/column removal and swaps, composed views, lazy scalar
multiplication, scalar division, exact/approximate equality, element-wise
arithmetic, nested-view materialization, rectangular products, overflow and
read-failure handling, cached structural flags, and determinant calculations.

Run them from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_test tests/matrix_test.c numerus_matrix.c numerus_storage.c
./tests/matrix_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_unary_test tests/matrix_unary_test.c numerus_matrix.c numerus_storage.c
./tests/matrix_unary_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_binary_test tests/matrix_binary_test.c numerus_matrix.c numerus_matrix_binary.c numerus_storage.c
./tests/matrix_binary_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_hadamard_test tests/matrix_hadamard_test.c numerus_matrix.c numerus_matrix_binary.c numerus_storage.c
./tests/matrix_hadamard_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_division_test tests/matrix_division_test.c numerus_matrix.c numerus_matrix_binary.c numerus_storage.c
./tests/matrix_division_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_materialize_test tests/matrix_materialize_test.c numerus_matrix.c numerus_matrix_binary.c numerus_matrix_materialize.c numerus_storage.c
./tests/matrix_materialize_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_multiply_test tests/matrix_multiply_test.c numerus_matrix.c numerus_matrix_binary.c numerus_matrix_materialize.c numerus_matrix_multiply.c numerus_storage.c
./tests/matrix_multiply_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_scalar_division_test tests/matrix_scalar_division_test.c numerus_matrix.c numerus_storage.c
./tests/matrix_scalar_division_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_compare_test tests/matrix_compare_test.c numerus_matrix.c numerus_matrix_compare.c numerus_storage.c
./tests/matrix_compare_test
```

The standalone allocator defines are only for native tests. Extension builds
use the Zend Memory Manager.
