# Matrix Operation Contracts

This document establishes the conventions new numerical operations must follow. It complements the implementation roadmap and the declarations in `numerus_matrix.h`.

## Result types

- An operation that mathematically produces a matrix returns a `numerus_matrix *` through an output parameter.
- Scalar analyses (determinant, trace, norms, rank, aggregate values and condition estimates) return their scalar value through a typed output parameter plus a status.
- Decompositions and analyses with multiple outputs use an explicit result structure or separate documented outputs. Do not encode scalar values as 1×1 matrices merely to force a uniform return type.

## Dimensions and indexing

- Dimensions are strictly positive; empty matrices are not currently supported.
- Public matrix coordinates and selection indices are zero-based.
- Every dimension addition, dimension product, element count, and byte count must be checked before allocation or indexing.
- Use `numerus_size.h` for checked `size_t` addition, multiplication, and triangular counts. Do not repeat ad-hoc overflow expressions in new algorithms.
- A failed operation must not expose a partially initialized Matrix. Initialize output pointers to `NULL` as soon as the output argument itself has been validated, and publish the result only after construction succeeds.
- For non-pointer output parameters, preserve the caller's previous value on failure.

## Status and numeric failure policy

The current Matrix status enum distinguishes invalid arguments, out-of-bounds access, incompatible dimensions, non-square inputs, size overflow, allocation failure, scalar division by zero, and success. Scalar division rejects a zero divisor with `NUMERUS_MATRIX_DIVISION_BY_ZERO`; element-wise division instead follows IEEE-754 and returns infinities/NaN as successful numeric values. These meanings must remain distinct.

New status values should be added when a concrete operation requires them:
- A singular matrix is a valid input for which an inverse may not exist; it is not an invalid argument and must not be reported as a successful zero-valued inverse.
- Failure to converge is different from invalid input and from a valid numerical result.
- A numerical breakdown or unsupported input class must be documented separately from allocation or dimension errors.

Do not add speculative statuses before an operation needs them. When required, update both Matrix and Storage mapping where applicable, document the exact trigger, and test that failure leaves outputs unchanged. Never use a numeric result such as `0.0`, `NAN`, or infinity as the sole error signal.

## Floating-point policy

- Matrix values are C `double`; integer-valued inputs are represented as doubles. Complex numbers and arbitrary precision are out of scope.
- Use `numerus_numeric.h` for approximate equality and structural classification.
- Do not automatically use the structural epsilon for pivot selection, singularity classification, numerical rank, or convergence. Those algorithms need scale-aware criteria specific to their numerical method.
- Define NaN/infinity behavior for each operation that compares, aggregates, or transforms values.
- Exact equality and approximate equality are different operations and must remain separately named/documented.

## Laziness and materialization

- A lazy view is appropriate when reading one result element maps cheaply to its parent(s) without recomputing an expensive algorithm.
- An operation such as matrix multiplication should not be represented by a naive lazy getter if each element read repeats an entire dot product. Prefer a materialized independent Storage-backed result unless a measured alternative avoids repeated work.
- Every view must document non-owning parent/context lifetime requirements. The future PHP object layer must retain referenced parent objects.
- Cache flags and results only after successful computation. Cache state must distinguish “not computed” from valid false/zero results. A failed operation must remain retryable unless documented otherwise.
- Do not add a generic cache or per-element cache without benchmark evidence and a clear memory/ownership policy.

## Testing and performance acceptance

Every operation PR should include:
- happy-path and shape-validation tests;
- bounds, overflow, and output-preservation tests where applicable;
- numerical edge cases (including NaN/infinity, scale extremes, and ill-conditioned inputs where relevant);
- tests for lazy-view composition and lifetime assumptions where relevant;
- native tests with `-std=c11 -Wall -Wextra -Wpedantic -Werror`, extension/PHPT checks, supported PHP CI jobs, and Debug build;
- benchmark results when performance determines whether to use a view, materialize a result, or cache an analysis.

Benchmarks should compare representative sizes and storage kinds, report compiler/build context and allocations where feasible, and avoid unstable timing thresholds as hard CI gates until the harness is reliable.

## Positive-definiteness classification

`numerus_matrix_classify_definiteness()` requires a finite, real symmetric square Matrix. It reports two predicates through distinct output pointers and leaves both values unchanged on failure. Eigenvalues within `NUMERUS_EPSILON × n × max(|λᵢ|)` of zero are treated as zero. The zero matrix is positive semidefinite but not positive definite. This is a numerical classification, not a symbolic proof.


## Alternative inverse algorithm: Gauss–Jordan

`numerus_matrix_inverse_gauss_jordan()` is an explicit alternative to the default
LU-based inverse. It uses partial pivoting and the same scale-aware numerical
rank classification as the existing inverse contract. It is intended for
algorithm comparison and education, not as a claim of better stability or
performance. It returns an independently materialized dense matrix; on invalid,
non-square, singular, non-finite, overflow, or allocation failure, the output
pointer remains NULL. The LU-based inverse remains the recommended default.
