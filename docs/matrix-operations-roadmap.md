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
- [ ] 0.6 Establish benchmark baselines. Include dense, diagonal, triangular, sparse, view-backed, and joined matrices at small/medium sizes; record time and allocations without making noisy microbenchmarks CI gates initially.

### Phase 0 decisions recorded

- Matrix-valued operations return a Matrix; scalar analyses return scalar outputs, and multi-output decompositions use explicit result structures.
- Dimension mismatch, bounds errors, invalid arguments, overflow, allocation failure, and non-square input retain distinct statuses. Future algorithm-specific statuses such as singularity or non-convergence are introduced only when an operation needs them; they must not be disguised as invalid arguments or valid numeric results.
- Checked size arithmetic lives in `numerus_size.h`. Its add, multiply, and triangular-count helpers leave output parameters unchanged on failure. Storage maps helper failures to its existing overflow status; Matrix uses the helpers for determinant workspace sizing.
- Native tests already use assertion helpers and explicit output-preservation checks. The shared arithmetic helpers now have boundary and overflow tests.

Exit criteria: documented contracts, robust size checks, reusable tests, and a documented baseline benchmark methodology. Actual timing baselines remain pending until Phase 1 introduces arithmetic operations worth benchmarking; do not invent baseline numbers. No numerical algorithm expansion in this phase.

# Phase 1 — Essential matrix-valued operations

- [x] 1.1 Scalar multiplication and unary negation. Return a lazy value-transform view when safe; preserve dimensions and parent lifetime. (Merged in PR #13.)
- [x] 1.2 Element-wise addition and subtraction. Require equal dimensions. Use a safe two-parent elementwise node if needed; do not hide parent pointers in a fragile borrowed callback context. (Merged in PR #14.)
- [x] 1.3 Hadamard product. Element-wise multiplication with equal-dimension validation. (Merged in PR #15.)
- [x] 1.4 Element-wise division. Define division-by-zero and non-finite behavior before implementation; do not silently invent a rule. IEEE-754 infinities/NaN are successful numeric results, not status errors. (Merged in PR #16.)
- [x] 1.5 Materialization API. Copy any logical Matrix (including nested views/joins) into independent Storage. Check allocation overflow and callback errors. (Merged in PR #17.)
- [x] 1.6 Matrix multiplication. Validate inner dimensions and overflow. Default to materializing the result because a naive lazy getter repeats each dot product on every read; benchmark dense and specialized inputs before adding optimized paths. (Merged in PR #18; baseline benchmark harness is being added in the Phase 0.6 follow-up.)
- [ ] 1.7 Scalar division. Reject a zero divisor according to the documented numeric contract.
- [ ] 1.8 Approximate and exact matrix equality. Exact equality is separate from epsilon-based close comparison; define shape mismatch behavior.
- [ ] 1.9 Constructors and diagonal utilities. Add missing constant/identity/diagonal/row-vector/column-vector constructors needed by later algorithms; do not add a separate Vector type unless it provides concrete value.
- [ ] 1.10 Matrix power for non-negative integer exponents. Use identity for exponent zero and exponentiation by squaring. Materialize computed results except for well-defined trivial cases.
- [ ] 1.11 Kronecker product. Validate output dimensions and allocation overflow; materialize by default.

Exit criteria: fundamental arithmetic and multiplication have shape, overflow, non-finite, and representation tests; benchmarks establish a baseline.

# Phase 2 — Structural views and assembly

Some capabilities already exist. Reuse them; do not reimplement existing behavior.

- [ ] 2.1 Slice and submatrix views. Rectangular ranges with strict zero-based bounds and positive result dimensions.
- [ ] 2.2 Row/column extraction and selection. Support a single row/column and arbitrary index lists, with documented duplicate-index behavior.
- [ ] 2.3 Reshape and flatten views. Preserve a documented logical traversal order (initially row-major); require element counts to match and guard multiplication overflow.
- [ ] 2.4 Padding and zero extension. Lazy coordinate mapping when outside-parent values are constant; otherwise use suitable specialized Storage.
- [ ] 2.5 Repetition and block matrices. Validate compatible block dimensions and all output-size arithmetic.
- [ ] 2.6 General block assembly. Extend joins only after contracts for empty/missing blocks are explicit; preserve lazy parents where useful.
- [ ] 2.7 Diagonal extraction and diagonal construction from vector-like input. Define main and offset diagonal behavior.
- [ ] 2.8 Row/column permutations. Generalize existing swap views to permutations only if there is a clear use case; validate uniqueness and bounds.

Exit criteria: no input copies for pure views, correct composition of nested views, tests for boundary indices and parent lifetime.

# Phase 3 — Basic scalar analysis and structural predicates

These operations correctly return scalar values/flags, not matrices.

- [ ] 3.1 Trace. Square matrices only; sum the main diagonal with documented floating-point behavior.
- [ ] 3.2 Element aggregates. Minimum, maximum, sum, mean, row/column sums, and row/column means; define behavior for NaN and infinities.
- [ ] 3.3 Matrix norms. Frobenius, induced 1-norm, and induced infinity-norm.
- [ ] 3.4 Finite-value inspection. Detect NaN and infinity; distinguish a predicate from an element-transform operation.
- [ ] 3.5 Structural predicates. Expose/reuse square, zero, diagonal, triangular, symmetric, and identity flags.
- [ ] 3.6 Additional predicates. Skew-symmetric and orthogonal, with dimension requirements and epsilon semantics.
- [ ] 3.7 Cache review. Ensure every structural cache bit has a precise definition and tests; avoid duplicating scans where one shared pass can safely compute multiple flags.
- [ ] 3.8 Aggregate caching. Cache only frequently reused full-matrix aggregates after benchmarks demonstrate value and cache memory/lifetime rules are documented.

Exit criteria: documented semantics for NaN/infinity and approximate predicates; all cache state transitions tested.

# Phase 4 — Determinant, rank, and foundational elimination

- [ ] 4.1 Determinant contract and edge cases. Preserve existing behavior and tests; add singular, pivoting, triangular, zero, identity, small/large-scale, and near-singular cases.
- [ ] 4.2 Factorization infrastructure. Design reusable internal work buffers and pivot arrays without exposing mutable matrix data.
- [ ] 4.3 LU with partial pivoting. Use an internal factorization/result structure with explicit permutation and lifetime rules. Test reconstruction PA ≈ LU.
- [ ] 4.4 Reuse LU for determinant. Use LU when it improves reuse/clarity, retaining specialized paths when demonstrably better.
- [ ] 4.5 Numerical rank. Implement via stable factorization/QR or SVD with scale-aware threshold semantics. Do not equate fixed absolute epsilon with numerical rank.
- [ ] 4.6 REF and RREF. Use independent work buffers; document pivot tolerance and result representation. Materialize the result.
- [ ] 4.7 Gaussian elimination and Gauss–Jordan. Reuse pivoting infrastructure; test row swaps, zero pivots, rank-deficient cases, and rectangular matrices where applicable.

Exit criteria: factorization reconstruction tests, numerical edge-case coverage, determinant regression tests, documented rank tolerance.

# Phase 5 — Inverse and linear-system solving

- [ ] 5.1 Define singularity and failure semantics. A singular matrix must not produce a successful inverse. Near-singular classification must be scale-aware and documented.
- [ ] 5.2 Inverse via LU solves. Materialize the inverse as independent Storage-backed Matrix. Avoid computing it via adjugate/determinant in the production path.
- [ ] 5.3 Inverse via Gauss–Jordan. Implement as a separate selectable algorithm only if it adds educational value or a measured trade-off; test independently.
- [ ] 5.4 Reference/cofactor inverse for small matrices. Optional, primarily as a reference/test oracle for tiny matrices, not a default production algorithm. Do not present it as generally stable or fast.
- [ ] 5.5 Cache inverse results. Choose a cache ownership design first: a retained immutable result with clear destruction, or a separate reusable factorization/result object. Never store a raw pointer to a Matrix whose lifetime is not owned. Bound memory and avoid cycles.
- [ ] 5.6 Solve Ax=b. Support a matrix right-hand side so multiple right-hand sides can share one factorization. The solution is a Matrix; do not compute inverse times b as the default implementation.
- [ ] 5.7 Reuse cached factorization. Share LU among determinant, inverse, and repeated solves only after a coherent factorization cache is designed and benchmarked.
- [ ] 5.8 Condition estimate. Help users identify unreliable results; distinguish estimate from exact property.
- [ ] 5.9 Inverse algorithm benchmarks. Compare LU-based inverse, Gauss–Jordan, and optional small-matrix path on time, allocations, residual norm, and ill-conditioned inputs. Do not choose solely by raw speed.

Exit criteria: independent inverse Storage, explicit singular/error behavior, residual and stability tests, documented caching/lifetime policy.

# Phase 6 — Spaces and least squares

- [ ] 6.1 Null space basis. Use rank-revealing QR or SVD; return a documented matrix whose columns span the null space.
- [ ] 6.2 Column-space and row-space bases. Define basis orientation and numerical-rank threshold.
- [ ] 6.3 Consistency and solution classification. Distinguish inconsistent, unique, and infinitely many solutions.
- [ ] 6.4 Least-squares solve. Prefer QR for the general path; avoid normal equations as the default because they can worsen conditioning.
- [ ] 6.5 Weighted least squares. Add after the ordinary least-squares contract is stable.
- [ ] 6.6 Moore–Penrose pseudoinverse. Prefer SVD-based implementation and test all four Moore–Penrose conditions within a documented tolerance.

Exit criteria: rectangular and rank-deficient test suites; residual-based validation; documented numerical thresholds.

# Phase 7 — Additional decompositions

- [ ] 7.1 QR decomposition. Prefer Householder reflectors for a robust baseline; define reduced vs full Q and output contracts.
- [ ] 7.2 Cholesky decomposition. Require symmetric positive-definite input within documented tolerance; report failure cleanly.
- [ ] 7.3 LDLᵀ decomposition. Define pivoting support and supported matrix classes.
- [ ] 7.4 SVD. Select a proven algorithm and test reconstruction, orthogonality, singular-value ordering, and rank-deficient cases.
- [ ] 7.5 Schur decomposition. Add after a reliable eigenvalue foundation exists.
- [ ] 7.6 Hessenberg and bidiagonal reductions. Implement as internal building blocks when required by eigenvalue/SVD algorithms.
- [ ] 7.7 Decomposition reuse/cache. Share only immutable, successfully computed factors with explicit ownership and measured benefit.

Exit criteria: reconstruction and orthogonality tests, convergence/error reporting, algorithm documentation and benchmark results.

# Phase 8 — Eigenvalues and matrix functions

- [ ] 8.1 Symmetric-matrix eigenvalues/eigenvectors. Start with the real symmetric case and a well-defined convergence contract.
- [ ] 8.2 General real-matrix eigenvalues. Eigenvalues can be complex even for real matrices. Since complex values are out of scope, define an explicit limitation/error contract instead of silently discarding imaginary components.
- [ ] 8.3 Spectral radius and spectral norm. Reuse relevant eigenvalue/SVD implementation.
- [ ] 8.4 Diagonalizability checks. Define numerical rather than symbolic semantics.
- [ ] 8.5 Matrix power for negative integer exponents. Build on inverse and exponentiation by squaring.
- [ ] 8.6 Matrix exponential. Prefer a numerically appropriate scaling-and-squaring method with Padé approximation rather than naïve Taylor summation as the only production path.
- [ ] 8.7 Matrix square root and logarithm. Specify supported input classes and failure behavior.
- [ ] 8.8 Matrix sine/cosine and polynomial functions. Add after matrix exponential and multiplication semantics are stable.

Exit criteria: convergence tests, residual checks, explicit real-only limitations, documented supported matrix classes.

# Phase 9 — Statistical, element-wise, and utility operations

- [ ] 9.1 Element-wise min/max and clamp. Define NaN propagation and interval validation.
- [ ] 9.2 Map/apply. Require deterministic callbacks and a clear context lifetime contract; distinguish lazy views from eager materialization.
- [ ] 9.3 Normalize. Support explicitly named norms/axes; reject zero-norm normalization or define its behavior.
- [ ] 9.4 Variance and standard deviation. Define population vs sample semantics and numerical accumulation strategy.
- [ ] 9.5 All-close / any-close. Use established comparison policy and provide exact equality separately.
- [ ] 9.6 Kronecker sum and selected structured products. Add only when built on stable product primitives.
- [ ] 9.7 Matrix polynomial evaluation. Use Horner's method with matrix multiplication; document coefficient order.

Exit criteria: clear scalar-vs-matrix result types, explicit NaN semantics, no hidden mutation of parent matrices.

# Phase 10 — Specialized matrices and storage-aware algorithms

- [ ] 10.1 Audit current Storage kinds. Dense, triangular, diagonal, identity, constant, zero, scaled identity, sparse, symmetric, and banded representations already exist; operations should exploit them where beneficial.
- [ ] 10.2 Specialized arithmetic paths. Add identity/zero/diagonal/triangular shortcuts to arithmetic and multiplication only when equivalence and benchmarks support them.
- [ ] 10.3 Toeplitz, Hankel, Vandermonde, Hilbert, and permutation constructors. Add only if use cases justify public API surface.
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
- [ ] Addition and subtraction
- [ ] Scalar addition/subtraction (if useful; distinguish from matrix-wide scalar transform)
- [ ] Scalar multiplication and division
- [ ] Unary negation
- [ ] Matrix multiplication
- [ ] Hadamard product
- [ ] Element-wise division
- [ ] Exact equality and approximate equality
- [ ] Non-negative and negative integer matrix powers
- [ ] Kronecker product and Kronecker sum
- [ ] Matrix polynomial

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
