# Matrix Storage-Aware Optimization Audit

Date: 2026-10-09

## Current representations

The Storage implementation currently supports these immutable representations:

| Kind | Value storage | Read behavior / implications |
| --- | --- | --- |
| Dense | O(rows × columns) doubles | Row-major direct lookup. |
| Upper triangular | Packed triangular values | Reads below the diagonal as zero. |
| Lower triangular | Packed triangular values | Reads above the diagonal as zero. |
| Diagonal | O(n) doubles | Reads off-diagonal elements as zero. |
| Identity | No value buffer | Computes 1 on the diagonal and 0 elsewhere. |
| Constant | One double | Every coordinate returns the same value. |
| Zero | No value buffer | Every coordinate returns zero. |
| Scaled identity | One double | Shared diagonal value, zero elsewhere. |
| Sparse | O(explicit entries) entries | Binary-search lookup; absent coordinates use the configured default. |
| Symmetric | Packed triangular values | Reflects coordinates across the diagonal. |
| Banded | O(rows × (lower bandwidth + upper bandwidth + 1)) doubles | Uses fixed row width and returns zero outside the configured band. |

The representation is private to `numerus_storage.c`; callers use the logical accessor and do not depend on internal buffers.

## Current operation behavior

- `numerus_matrix_multiply()` uses a generic triple loop. It checks the inner dimensions, reads both operands through `numerus_matrix_get()`, accumulates each dot product left-to-right in `double`, and materializes independent dense output.
- The multiplication algorithm does not dispatch on Storage kind. Identity, zero, diagonal, triangular, sparse, symmetric, and banded operands still go through the same O(m × k × n) loop and coordinate reads.
- Existing benchmark coverage measures reads for dense, diagonal, upper-triangular, and sparse matrices, but multiplication timing currently covers only dense 32×32 operands. The initial measurements are a single smoke run and are not enough to justify algorithm selection.
- Element-wise binary operations are lazy views; they preserve the logical Storage semantics but do not currently specialize their arithmetic by representation.

## Correctness constraints before adding fast paths

The current implementation evaluates IEEE-754 operations even where an implicit Storage element is zero. For example, a generic dot product can evaluate `0 × INFINITY` or `0 × NaN`, producing NaN. A shortcut that returns the other operand for multiplication by identity, returns an all-zero matrix for multiplication by zero, or skips zero terms may therefore change observable results for non-finite inputs.

Any specialized path must choose and document one of these policies before implementation:

1. Preserve the existing IEEE-754 result exactly, including NaN propagation from zero-times-non-finite terms; or
2. Explicitly narrow the fast-path precondition (for example, prove operands finite) and fall back to the generic implementation otherwise.

Do not silently change the numerical contract in the name of performance. Preserve dimension/status behavior, output-pointer failure guarantees, and materialized-result ownership.

## Recommended sequence

1. Expand the standalone benchmark to compare generic multiplication against candidate paths for identity, zero, diagonal, triangular, sparse, symmetric, and banded operands across small and medium shapes.
2. Record repeated samples and medians, requested allocation counts/bytes, and a correctness comparison against the generic implementation. Include finite and non-finite inputs.
3. Implement only shortcuts that demonstrate a meaningful win and pass semantic-equivalence tests. Keep the generic algorithm as the fallback.
4. Consider sparse multiplication/factorization separately: sparse fill-in and output representation require their own design rather than a dense result hidden behind a sparse label.

This audit does not introduce specialized arithmetic paths; it establishes the evidence and semantic requirements for Phase 10.2.
