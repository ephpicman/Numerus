# Matrix Operations Roadmap

This roadmap defines the implementation order for Numerus's internal C Matrix API. Execute it incrementally: each phase is a small, independently reviewable pull request, and no phase is merged until its required CI checks pass.

## 1. Accepted design constraints

- Matrix and Storage are immutable.
- Matrix elements are represented as C double. Integer-valued numbers are represented by doubles. Complex numbers and arbitrary-precision numeric types are out of scope.
- Matrix transformations should be lazy views when correct and beneficial.
- Operations whose repeated lazy reads would redo expensive work should materialize a new independent Storage-backed Matrix when benchmarks and semantics justify it.
- Parent references in the current C API are non-owning. Every view/operation must document lifetime requirements. The future PHP object layer must retain all referenced parents.
- Structural flags and computed results may be cached per Matrix. Cache state must distinguish “not computed” from valid results such as zero, false, or an empty set.
- Coordinate transforms must remain deterministic and logically immutable. Any transform context is borrowed and must outlive the view.
- Preserve current API conventions: validate arguments at boundaries, return explicit status codes, and leave output parameters unchanged on failure.
- Each implementation PR adds native tests; add PHPT tests when PHP-visible behavior exists. Update documentation and benchmarks when relevant.
- Do not add a generic cache framework pre-emptively. Add only specific cache entries and lifetime rules required by proven operations.

## 2. Result-type clarification

Most element-wise and structural transformations return a Matrix. Not every mathematical operation can return a Matrix without changing its meaning: determinant, trace, norms, rank, condition number, and individual eigenvalues are scalar results; eigenvectors and decomposition outputs can contain multiple matrices. These APIs must use the mathematically correct C result type (or a documented result structure) rather than encoding scalars as 1×1 matrices. “All operations return Matrix” therefore applies to matrix-valued transformations, not scalar-valued analyses.

## 3. Core architecture rules

### Lazy views vs. materialized results

Use a lazy view when each read is cheap, deterministic, and naturally maps to the parent:
- transpose, flips, rotations, row/column removal and swaps;
- slices, row/column selections, and submatrices;
- simple reshape/flatten mappings where logical element order is well-defined;
- horizontal/vertical joins and block views;
- cheap unary element-wise transforms where the callback contract is safe.

Prefer a new Storage-backed result when the operation is computational and lazy access would repeatedly redo substantial work:
- matrix multiplication (each output read otherwise repeats a dot product);
- inverse and decomposition results;
- matrix power for positive powers beyond trivial cases;
- SVD/eigenvalue-related outputs and other numerical algorithms;
- statistical transforms that need a full pass and are reused.

These are defaults, not dogma. Measure representative workloads before choosing a more complicated caching design. A materialized result remains immutable and can use specialized Storage when genuinely cheaper.

### Cache policy

1. Keep per-Matrix structural flag caching (flags_computed) and the determinant cache.
2. Add result caches only for expensive, deterministic scalar/matrix analyses with a clear ownership and memory-cost policy.
3. A cached matrix result needs an explicit lifetime strategy. Do not keep dangling references or create reference cycles.
4. Mark a cache entry computed only after the full operation succeeds. Allocation, callback, and numerical failures must remain retryable unless an API explicitly documents otherwise.
5. Cache valid zero-valued results, false flags, and singularity outcomes distinctly from “not computed.”
6. Do not cache each lazy element by default: it can add O(rows×columns) memory, complicate concurrency, and duplicate Storage. Materialize the whole result or add a per-element cache only when measurements justify it.
7. Do not promise thread safety for mutable cache population until synchronization is implemented and tested. Document current concurrency assumptions.
8. Prefer caching reusable decompositions (for example LU) only after ownership, memory overhead, and sharing across determinant/solve/inverse have been designed.

### Numerical policy

- Use numerus_numeric.h and the established epsilon policy for approximate comparisons and structural classification.
- Do not apply a fixed epsilon blindly to pivot/singularity decisions. Use scale-aware numerical criteria appropriate to each algorithm and document them.
- Distinguish an exact mathematical property from a floating-point numerical estimate (e.g. numerical rank, positive definiteness, near-singularity).
- Test NaN, infinities, signed zero, very small/large magnitudes, overflow, underflow, and ill-conditioned inputs where relevant.
- A successful result containing zero is not an error. Every failure path must return a status and preserve output parameters.

## 4. Delivery process for every phase

For each phase:
1. Inspect current code and conventions before changing implementation.
2. Create one focused branch and PR.
3. Add correctness tests, invalid-input tests, edge cases, and allocation/callback failure tests where applicable.
4. Run native tests with -std=c11 -Wall -Wextra -Wpedantic -Werror, PHPT/build checks, supported PHP-version CI, and the Debug job.
5. Review asymptotic complexity, overflow safety, numerical stability, storage representation, ownership, and API consistency.
6. Update relevant docs and record benchmark results when performance is a design criterion.
7. Merge only after every required CI job passes; otherwise fix the branch and rerun checks.
8. Delete the feature branch after merge if repository policy permits.
9. Check off roadmap items only after implementation PRs are merged and verified.

Do not combine unrelated algorithms into one large PR. This roadmap is a sequence of independently mergeable deliverables, not a mandate to implement every advanced algorithm before the project can be useful.

---

# Phase 0 — Contracts and operation infrastructure

- [x] 0.1 Define operation contracts. Document shape rules, result type, error statuses, numerical behavior, and whether each operation returns a view or materialized Matrix.
- [x] 0.2 Define error-status policy; add missing status codes only when needed. Decide how singular matrices, non-convergence, and numerically invalid inputs are represented; avoid collapsing all such cases into invalid argument.
- [x] 0.3 Add shared safe size arithmetic helpers. Check rows×columns, buffer sizes, output dimensions, and all allocation-size calculations for overflow.
- [x] 0.4 Review shared iteration helpers. Decision: retain explicit hot loops for now; reuse the checked/unchecked accessors and add a shared iteration abstraction only when multiple operations demonstrate the same stable traversal contract. This avoids premature abstraction in performance-sensitive C loops.
- [x] 0.5 Establish baseline test helpers. Matrix construction/assertion helpers, approximate comparisons, status assertions, and failure-path output-preservation tests.
- [x] 0.6 Establish benchmark baselines. Include dense, diagonal, triangular, sparse, view-backed, and joined matrices at small/medium sizes; record time and allocations without making noisy microbenchmarks CI gates initially. (Harness and initial measurements: `benchmarks/README.md`, `benchmarks/baseline-2026-10-09.md`.)

### Phase 0 decisions recorded

- Matrix-valued operations return a Matrix; scalar analyses return scalar outputs, and multi-output decompositions use explicit result structures.
- Dimension mismatch, bounds errors, invalid arguments, overflow, allocation failure, and non-square input retain distinct statuses. Future algorithm-specific statuses such as singularity or non-convergence are introduced only when an operation needs them; they must not be disguised as invalid arguments or valid numeric results.
- Checked size arithmetic lives in `numerus_size.h`. Its add, multiply, and triangular-count helpers leave output parameters unchanged on failure. Storage maps helper failures to its existing overflow status; Matrix uses the helpers for determinant workspace sizing.
- Native tests already use assertion helpers and explicit output-preservation checks. The shared arithmetic helpers now have boundary and overflow tests.

Exit criteria: documented contracts, robust size checks, reusable tests, and a documented benchmark methodology with an initial recorded timing/allocation baseline. The recorded run is a single CI smoke sample; use repeated samples and medians before drawing performance conclusions. No numerical algorithm expansion in this phase.

# Phase 1 — Essential matrix-valued operations

- [x] 1.1 Scalar multiplication and unary negation. Return a lazy value-transform view when safe; preserve dimensions and parent lifetime. (Merged in PR #13.)
- [x] 1.2 Element-wise addition and subtraction. Require equal dimensions. Use a safe two-parent elementwise node if needed; do not hide parent pointers in a fragile borrowed callback context. (Merged in PR #14.)
- [x] 1.3 Hadamard product. Element-wise multiplication with equal-dimension validation. (Merged in PR #15.)
- [x] 1.4 Element-wise division. Define division-by-zero and non-finite behavior before implementation; do not silently invent a rule. IEEE-754 infinities/NaN are successful numeric results, not status errors. (Merged in PR #16.)
- [x] 1.5 Materialization API. Copy any logical Matrix (including nested views/joins) into independent Storage. Check allocation overflow and callback errors. (Merged in PR #17.)
- [x] 1.6 Matrix multiplication. Validate inner dimensions and overflow. Default to materializing the result because a naive lazy getter repeats each dot product on every read; benchmark dense and specialized inputs before adding optimized paths. (Merged in PR #18; baseline benchmark harness is being added in the Phase 0.6 follow-up.)
- [x] 1.7 Scalar division. Reject a zero divisor with `NUMERUS_MATRIX_DIVISION_BY_ZERO`; direct division is used instead of multiplying by a reciprocal. (Merged in PR #20.)
- [x] 1.8 Approximate and exact matrix equality. Exact equality is separate from epsilon-based close comparison; shape mismatch returns success with `false`. (Merged in PR #21.)
- [x] 1.9 Constructors and diagonal utilities. Existing constant/identity/diagonal constructors were retained; row-vector and column-vector constructors were added as ordinary dense Matrices, without a separate Vector type. (Merged in PR #22.)
- [x] 1.10 Matrix power for non-negative integer exponents. Use identity for exponent zero and exponentiation by squaring. Materialize computed results except for well-defined trivial cases. (Merged in PR #23.)
- [x] 1.11 Kronecker product. Validate output dimensions and allocation overflow; materialize by default. (Merged in PR #24.)

Exit criteria: fundamental arithmetic and multiplication have shape, overflow, non-finite, and representation tests; benchmarks establish a baseline.

# Phase 2 — Structural views and assembly

Some capabilities already exist. Reuse them; do not reimplement existing behavior.

- [x] 2.1 Slice and submatrix views. Rectangular ranges use strict zero-based bounds and positive result dimensions. (Merged in PR #25.)
- [x] 2.2 Row/column extraction and selection. Single rows/columns are supported by a one-element index list; arbitrary lists preserve order and allow duplicates. (Merged in PR #26.)
- [x] 2.3 Reshape and flatten views. Preserve row-major traversal order; require element counts to match and guard multiplication overflow. (Merged in PR #27.)
- [x] 2.4 Padding and zero extension. Lazy constant-fill mapping with checked dimensions; zero extension is a convenience wrapper. (Merged in PR #28.)
- [x] 2.5 Repetition and block matrices. Lazy tiling and block-diagonal views validate positive repetition counts and checked output dimensions. (Merged in PR #29.)
- [x] 2.6 General block assembly. Row-major block grids require non-NULL blocks, consistent heights per block row and widths per block column, and checked sizes. (Merged in PR #30.)
- [x] 2.7 Diagonal extraction and construction from row/column vectors. Offset zero is the main diagonal; positive offsets are above and negative offsets below. (Merged in PR #31.)
- [x] 2.8 Row/column permutations. Full zero-based permutations validate uniqueness, bounds, and dimension match. (Merged in PR #32.)

Exit criteria: no input copies for pure views, correct composition of nested views, tests for boundary indices and parent lifetime.

# Phase 3 — Basic scalar analysis and structural predicates

These operations correctly return scalar values/flags, not matrices.

- [x] 3.1 Trace. Square matrices only; sum the main diagonal in documented order with IEEE-754 behavior. (Merged in PR #33.)
- [x] 3.2 Element aggregates. Minimum, maximum, sum, mean, row/column sums, and row/column means; NaN and infinity behavior is documented. (Merged in PR #34.)
- [x] 3.3 Matrix norms. Frobenius, induced 1-norm, and induced infinity-norm; stable Frobenius accumulation and NaN behavior are tested. (Merged in PR #37.)
- [x] 3.4 Finite-value inspection. Detect NaN and infinity; distinguish predicates from element transforms. (Merged in PR #38.)
- [x] 3.5 Structural predicates. Expose/reuse square, zero, diagonal, triangular, symmetric, and identity flags. (Already implemented in `numerus_matrix_get_flags()` and covered by native tests.)
- [x] 3.6 Additional predicates. Skew-symmetric and orthogonal, with dimension requirements and epsilon semantics. (Merged in PR #39.)
- [x] 3.7 Cache review. Structural flag bits are grouped and cached after one shared scan; failed reads do not populate cache, and tests verify cache hits and shared symmetry reads. (Audited; no implementation change required.)
- [ ] 3.8 Aggregate caching. Deferred: no benchmark evidence currently justifies retaining aggregate results or expanding cache memory/lifetime complexity.

Exit criteria: documented semantics for NaN/infinity and approximate predicates; all cache state transitions tested.

# Phase 4 — Determinant, rank, and foundational elimination

- [x] 4.1 Determinant contract and edge cases. Singular, pivoting, triangular, zero, identity, small/large-scale, and near-singular cases are covered. (Merged in PR #40.)
- [x] 4.2 Factorization infrastructure. Reusable owned work buffers and pivot arrays are isolated behind a private read-only interface. (Merged in PR #41.)
- [x] 4.3 LU with partial pivoting. Private factorization exposes permutation and L/U accessors; reconstruction tests verify PA ≈ LU. (Merged in PR #41.)
- [x] 4.4 Reuse LU for determinant. The general determinant path reuses LU while preserving zero/identity/triangular fast paths. (Merged in PR #42.)
- [x] 4.5 Numerical rank. Complete-pivoting elimination uses a scale-aware threshold; this is a numerical estimate, not symbolic rank. (Merged in PR #43.)
- [x] 4.6 REF and RREF. Independent work buffers, documented pivot tolerance, and dense materialized results. (Merged in PR #44.)
- [x] 4.7 Gaussian elimination and Gauss–Jordan. REF/RREF reuse pivoting logic and test row swaps, zero pivots, rank-deficient cases, and rectangular matrices. (Merged in PR #44.)

Exit criteria: factorization reconstruction tests, numerical edge-case coverage, determinant regression tests, documented rank tolerance.

# Phase 5 — Inverse and linear-system solving

- [x] 5.1 Define singularity and failure semantics. Singular and near-singular inputs use scale-aware numerical rank; failures return statuses and no result. (Implemented and merged in PR #45.)
- [x] 5.2 Inverse via LU solves. Materialize the inverse as independent Storage-backed Matrix; no adjugate/determinant path. (Merged in PR #45.)
- [ ] 5.3 Inverse via Gauss–Jordan. Implement as a separate selectable algorithm only if it adds educational value or a measured trade-off; test independently.
- [ ] 5.4 Reference/cofactor inverse for small matrices. Optional, primarily as a reference/test oracle for tiny matrices, not a default production algorithm. Do not present it as generally stable or fast.
- [ ] 5.5 Cache inverse results. Choose a cache ownership design first: a retained immutable result with clear destruction, or a separate reusable factorization/result object. Never store a raw pointer to a Matrix whose lifetime is not owned. Bound memory and avoid cycles.
- [x] 5.6 Solve Ax=b. Matrix RHS supports multiple columns sharing one LU factorization; no inverse-times-vector default path. (Merged in PR #46.)
- [ ] 5.7 Reuse cached factorization. Share LU among determinant, inverse, and repeated solves only after a coherent factorization cache is designed and benchmarked.
- [x] 5.8 Condition estimate. One-norm estimate distinguishes conditioning from exact properties; singular inputs report infinity. (Merged in PR #47.)
- [x] 5.9 Inverse algorithm benchmarks. LU inverse and condition estimation report CPU time, allocations, and residuals for well-conditioned and ill-conditioned inputs. No competing inverse algorithm is implemented; Gauss–Jordan/small-matrix alternatives remain deferred unless they offer a justified trade-off. (Completed in PR #50.)

Exit criteria: independent inverse Storage, explicit singular/error behavior, residual and stability tests, documented caching/lifetime policy.

# Phase 6 — Spaces and least squares

- [x] 6.1 Null space basis. Column-pivoted Householder QR returns a numerical basis; NULL plus nullity=0 represents the trivial null space. (Merged in PR #51.)
- [x] 6.2 Column-space and row-space bases. Column basis vectors are columns; row basis vectors are rows. Both use the documented scale-aware numerical-rank threshold. (Merged in PR #52.)
- [x] 6.3 Consistency and solution classification. Pivoted QR and an RHS-relative residual distinguish inconsistent, unique, and infinitely many solutions. (Merged in PR #53.)
- [x] 6.4 Least-squares solve. Column-pivoted QR solves full-column-rank overdetermined and square systems without normal equations; rank-deficient/minimum-norm cases are deferred to SVD/pseudoinverse work. (Merged in PR #54.)
- [x] 6.5 Weighted least squares. Finite nonnegative weights are applied by row scaling with sqrt(weight), then solved by pivoted QR. (Merged in PR #55.)
- [x] 6.6 Moore–Penrose pseudoinverse. SVD-based implementation supports rectangular, rank-deficient, and zero matrices; native tests cover all four Moore–Penrose conditions and the documented scale-aware singular-value threshold.

Exit criteria: rectangular and rank-deficient test suites; residual-based validation; documented numerical thresholds.

# Phase 7 — Additional decompositions

- [x] 7.1 QR decomposition. Reduced Householder QR defines Q as m×min(m,n) and R as min(m,n)×n; unpivoted QR is not rank-revealing. (Merged in PR #49.)
- [x] 7.2 Cholesky decomposition. Scale-aware symmetry and positive-pivot checks; semidefinite/indefinite inputs return explicit statuses. (Merged in PR #56.)
- [x] 7.3 LDLᵀ decomposition. Unpivoted LDLᵀ supports symmetric matrices with safely nonzero leading pivots, including some indefinite matrices; pivot-required cases return an explicit small-pivot status. (Integrated in PR #58.)
- [x] 7.4 SVD. One-sided Jacobi SVD covers tall/wide, rank-deficient and zero matrices with reconstruction, orthogonality, ordering and convergence tests. (Merged in PR #59.)
- [ ] 7.5 Schur decomposition. Add after a reliable eigenvalue foundation exists.
- [ ] 7.6 Hessenberg and bidiagonal reductions. Implement as internal building blocks when required by eigenvalue/SVD algorithms.
- [ ] 7.7 Decomposition reuse/cache. Share only immutable, successfully computed factors with explicit ownership and measured benefit.

Exit criteria: reconstruction and orthogonality tests, convergence/error reporting, algorithm documentation and benchmark results.

# Phase 8 — Eigenvalues and matrix functions

- [x] 8.1 Symmetric-matrix eigenvalues/eigenvectors. Cyclic Jacobi rotations return descending real eigenvalues and orthonormal eigenvectors with reconstruction/convergence tests. (Merged in PR #60.)
- [ ] 8.2 General real-matrix eigenvalues. Deferred: complex numbers are an explicit non-goal, and the current API exposes only real symmetric eigendecomposition. Do not silently discard imaginary components; revisit only after a complex-value representation and error contract are designed.
- [x] 8.3 Spectral radius and spectral norm. General spectral norm reuses SVD; symmetric spectral radius reuses symmetric eigendecomposition. General nonsymmetric spectral radius remains deferred until the real/complex eigenvalue contract is implemented. (Merged in PR #61.)
- [ ] 8.4 Diagonalizability checks. Define numerical rather than symbolic semantics.
- [x] 8.5 Matrix power for negative integer exponents. Builds on one LU-based inverse and exponentiation by squaring. (Merged in PR #62.)
- [ ] 8.6 Matrix exponential. Prefer a numerically appropriate scaling-and-squaring method with Padé approximation rather than naïve Taylor summation as the only production path.
- [ ] 8.7 Matrix square root and logarithm. Specify supported input classes and failure behavior.
- [ ] 8.8 Matrix sine/cosine and polynomial functions. Add after matrix exponential and multiplication semantics are stable.

Exit criteria: convergence tests, residual checks, explicit real-only limitations, documented supported matrix classes.

# Phase 9 — Statistical, element-wise, and utility operations

- [x] 9.1 Element-wise min/max and clamp. NaN propagates; clamp rejects NaN bounds and reversed intervals while permitting infinite bounds. Results are independent dense matrices and source-read failures propagate.
- [x] 9.2 Map/apply. Lazy map borrows deterministic callback/context and parent; eager apply materializes independent dense Storage and propagates callback/read failures.
- [x] 9.3 Normalize. Supports global, row, and column axes with L1/L2/infinity norms; rejects non-finite inputs and zero-norm vectors without publishing partial output.
- [x] 9.4 Variance and standard deviation. Population/sample semantics are explicit; one-pass Welford accumulation rejects non-finite values and preserves outputs on failure.
- [x] 9.5 All-close / any-close. Reuses the established approximate comparison policy; exact equality remains separate.
- [x] 9.6 Kronecker sum. Requires square inputs, uses checked dimensions/allocation sizes, and materializes independent dense Storage.
- [x] 9.7 Matrix polynomial evaluation. Uses Horner's method, ascending coefficient order, materialized products, and explicit failure cleanup.

Exit criteria: clear scalar-vs-matrix result types, explicit NaN semantics, no hidden mutation of parent matrices.

# Phase 10 — Specialized matrices and storage-aware algorithms

- [x] 10.1 Audit current Storage kinds. Documented representation costs, generic multiplication behavior, IEEE-754 constraints, and the benchmark evidence needed before specialization in [the Storage optimization audit](matrix-storage-optimization-audit.md).
- [x] 10.2 Specialized multiplication paths. Added exact identity/zero shortcuts guarded by finite nonnegative operands; other inputs use the generic IEEE-754 loop. Diagonal, triangular, and sparse paths remain deferred until separate benchmarks justify their complexity.
- [x] 10.3 Constructor scope review. Deferred Toeplitz, Hankel, Vandermonde, Hilbert, and a separate permutation-matrix constructor because there is no current consumer; existing row/column permutation views cover the immediate need. See [the Storage optimization audit](matrix-storage-optimization-audit.md).
- [ ] 10.4 Sparse-aware multiplication and factorization. Avoid densifying large sparse inputs unnecessarily; define sparse fill-in behavior.
- [ ] 10.5 Sparse factorization and rank. Requires dedicated algorithm and memory benchmarks; do not promise sparse performance from dense implementations.
- [ ] 10.6 Tensor/array boundary. Kronecker product is still a matrix operation. General N-dimensional tensors are a separate abstraction and are not silently folded into Matrix.
- [ ] 10.7 Polar decomposition, matrix sign, Sylvester and Lyapunov equations. Treat as advanced, dependency-heavy features; prioritize only after foundational algorithms are robust.

Exit criteria: specialized paths are benchmarked against general paths and never change observable results beyond documented floating-point tolerances.

# Phase 11 — Hardening and release quality

- [ ] 11.1 Full API contract audit. Dimensions, index base, scalar/matrix result types, statuses, ownership, and numerical policy are consistent.
- [ ] 11.2 Adversarial tests. Invalid dimensions, overflow, allocation failure, callback failure, NaN/infinity, subnormals, extreme magnitudes, singular and ill-conditioned inputs.
- [ ] 11.3 Memory-safety checks. Debug builds and available sanitizers; verify parent/child lifetime and cache destruction paths.
- [ ] 11.4 Property tests. Examples: (AB)ᵀ = BᵀAᵀ, AI = A, A+0 = A, decomposition reconstruction, solve residuals, and inverse residuals.
- [ ] 11.5 Performance suite. Dense and specialized matrices, materialized results vs views, repeated reads, nested views, and cache hit/miss behavior.
- [ ] 11.6 Documentation. Public contracts, algorithm choice, complexity, stability caveats, and runnable examples.
- [ ] 11.7 Final CI matrix. All supported PHP versions, native tests, PHPT, extension load, and Debug job must pass before release.

---

## 5. Full operation inventory

This inventory captures the broader feature set discussed for Numerus. It is not a commitment to ship every advanced operation immediately; phase order and exit criteria determine implementation.

### Arithmetic and element-wise
- [x] Element-wise minimum, maximum, and clamp (Phase 9.1).
- [ ] Addition and subtraction
- [ ] Scalar addition/subtraction (if useful; distinguish from matrix-wide scalar transform)
- [ ] Scalar multiplication and division
- [ ] Unary negation
- [ ] Matrix multiplication
- [ ] Hadamard product
- [ ] Element-wise division
- [x] Exact equality and approximate equality, including all-close and any-close predicates
- [ ] Non-negative and negative integer matrix powers
- [x] Kronecker product and Kronecker sum (Phase 9.6)
- [x] Matrix polynomial (Phase 9.7)

### Structural and assembly
- [x] Transpose, row/column flips, rotations, row/column removal and swaps, horizontal/vertical joins (already implemented)
- [ ] Slice/submatrix
- [ ] Row/column extraction and selection
- [ ] Reshape/flatten
- [ ] Padding/zero extension
- [ ] Repetition/block assembly
- [ ] Main and offset diagonal extraction
- [ ] Row/column permutation

### Scalar analyses and predicates
- [x] Determinant (already implemented)
- [ ] Trace
- [ ] Rank and numerical rank
- [ ] Norms (Frobenius, 1, infinity, spectral)
- [ ] Condition number/condition estimate
- [ ] Sum, mean, min, max, variance, standard deviation
- [ ] Row/column aggregates
- [x] Zero/identity/diagonal/triangular/symmetric flags (already implemented)
- [ ] Skew-symmetric and orthogonal predicates
- [ ] Finite/NaN/infinity checks
- [ ] Positive-definite and positive-semidefinite classification

### Elimination, solving, and spaces
- [ ] Gaussian elimination
- [ ] Gauss–Jordan elimination
- [ ] REF/RREF
- [ ] LU factorization with partial pivoting
- [ ] Solve square systems and multiple right-hand sides
- [ ] Consistency and solution classification
- [ ] Null space, column space, row space
- [ ] Least squares and weighted least squares
- [ ] Moore–Penrose pseudoinverse

### Decompositions and spectral operations
- [ ] QR
- [ ] Cholesky
- [ ] LDLᵀ
- [ ] SVD
- [ ] Eigenvalues/eigenvectors (real-only contract must be explicit)
- [ ] Schur
- [ ] Hessenberg and bidiagonal reductions
- [ ] Polar decomposition
- [ ] Matrix exponential, logarithm, square root, sine/cosine
- [ ] Matrix sign
- [ ] Sylvester and Lyapunov equations

### Constructors and specialized representations
- [x] Zero, identity, constant, diagonal, triangular, symmetric, sparse, and banded constructors (already implemented)
- [ ] Random matrices (deterministic seed contract required for reproducibility)
- [ ] Permutation, Vandermonde, Toeplitz, Hankel, Hilbert
- [ ] Storage-aware dense/sparse/diagonal/triangular algorithms
- [ ] General tensors (separate abstraction; out of Matrix scope)

## 6. Explicit non-goals for the current implementation sprint

- Complex numbers
- Arbitrary precision
- A general tensor API
- A speculative universal cache framework
- Caching every element of every lazy view
- Implementing advanced operations without stable foundational algorithms and tests
- Claiming numerical stability solely because an algorithm passes ordinary small-integer examples

## 7. Immediate next PR

Start with Phase 0.1–0.3 and keep the PR narrow: define operation contracts, settle status semantics for numerical failures, and audit checked dimension/allocation arithmetic. Do not begin all arithmetic functions in one PR. Once the contracts are merged and CI is green, proceed to Phase 1 in the listed order.
