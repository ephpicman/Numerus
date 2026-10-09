# Matrix

Matrix is Numerus's internal logical matrix layer above Storage. It provides a
uniform coordinate-based interface while allowing roots, derived views, and
joined matrices to share data without copying it.

Matrix is currently an internal C API, not a PHP-facing object API. The
function-level contract is declared in [`numerus_matrix.h`](../numerus_matrix.h).

## Object model

A Matrix has positive row and column counts and is one of three forms:

- **Root Matrix:** owns an immutable `numerus_storage`.
- **Derived view:** refers to one parent Matrix and applies coordinate and/or
  value transforms when an element is read.
- **Joined view:** refers to two parent Matrices and delegates each read to the
  appropriate parent.

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

## Ownership and lifetime

Ownership is deliberately simple but important:

- A root Matrix owns and destroys its Storage.
- A derived view does not own its parent.
- A joined view does not own either parent.
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

A numeric zero is a valid value, not an error signal.

`numerus_matrix_get_unchecked()` skips validation and is intended for callers
that have already established valid pointers and coordinates. Despite its name,
it returns a status because custom callbacks and parent reads can still fail.
Only use it when the Matrix and coordinate preconditions are satisfied.

The metadata functions `numerus_matrix_rows()` and
`numerus_matrix_columns()` return zero for a NULL pointer.
`numerus_matrix_storage_kind()` reports the root Storage kind for ordinary
derived views and returns `NUMERUS_STORAGE_ZERO` for NULL. For a joined Matrix,
the implementation currently follows the first parent; therefore, this value
does not describe both sides of a join and should not be treated as a complete
summary of a joined Matrix's representations.


## Cached analysis

Each Matrix has a per-object analysis cache. It stores structural metadata and
scalar results, not element values, so it does not change the lazy-view model.

`numerus_matrix_get_flags()` computes and caches exact structural properties:
square, zero, diagonal, upper-triangular, lower-triangular, symmetric, and
identity. These flags describe the logical values exposed by that Matrix,
including views and joins. Comparisons use exact floating-point equality; no
tolerance is applied. A square zero Matrix is also diagonal, upper-triangular,
lower-triangular, and symmetric.

`numerus_matrix_determinant()` caches a successfully computed determinant.
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

The native Matrix tests cover storage-backed constructors, views and their
coordinate mappings, composed views, custom value transforms, callback error
propagation, joins, dimension mismatch and overflow handling, cached structural
flags, determinant calculations, and output-value preservation after failures.

Run them from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_test tests/matrix_test.c numerus_matrix.c numerus_storage.c
./tests/matrix_test
```

The standalone allocator defines are only for native tests. Extension builds
use the Zend Memory Manager.
