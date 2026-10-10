# v1 Statistical Foundation — Final Acceptance Audit

Audit date: 2026-10-10  
Audit base: `main` after PR #146.  
Outcome: **core C-internal statistical primitives are implemented and tested; overall userland v1 release gate is BLOCKED.**

This is an evidence-based source/CI audit, not a claim that the extension is ready for PHP statistical packages. The current extension skeleton registers no PHP functions/classes, so the C primitives documented below are not callable from PHP userland yet.

## Evidence reviewed

- Implementation headers and sources: `numerus_matrix*.c/.h`, `numerus_numeric.c/.h`, `numerus_optimizer.c/.h`, `numerus_rng.c/.h`, `numerus_rng_sampling.c`, and `numerus_probability.c/.h`.
- Focused native tests: `matrix_least_squares_test.c`, `matrix_weighted_least_squares_test.c`, `matrix_triangular_solve_test.c`, `matrix_log_determinant_test.c`, `matrix_statistical_workflow_test.c`, `rng_state_test.c`, `rng_variates_test.c`, `rng_sampling_test.c`, `numeric_stable_functions_test.c`, `optimizer_test.c`, `gaussian_log_density_test.c`, `gaussian_sampling_test.c`, and `statistical_property_test.c`.
- Contracts and examples: [API boundary](statistical-api-boundary.md), [GLS/log-determinant](gls-whitening-and-log-determinant-contracts.md), [RNG](reproducible-rng-contract.md), [optimizer](optimizer-contract.md), [non-finite/portability policy](numerical-portability-and-nonfinite-policy.md), and [statistical foundation guide](statistical-foundation.md).
- Workflow `.github/workflows/tests.yml`, including PHP 8.2–8.5 on Ubuntu, debug build, focused native tests, and ASan/UBSan test runs.
- PR #146 CI passed for PHP 8.2, 8.3, 8.4, 8.5 and the debug build: [workflow run](https://github.com/ephpicman/Numerus/actions/runs/38028916807).

A green CI run establishes the configured test/build commands passed on the listed Linux matrix. It does not establish Windows support, broad cross-platform floating-point identity, or complete numerical guarantees outside the tested cases.

## P0 requirement disposition

| Requirement | Current disposition | Evidence / remaining limitation |
|---|---|---|
| LIN-01 — products and multi-RHS solves | Implemented; focused tests | Matrix multiply and solve APIs plus native tests. Prefer solves over inverse-times-vector. |
| LIN-02 — least squares | Partial | Pivoted-QR least squares and WLS exist and are tested. Rank-deficient inputs are reported rather than automatically returning a minimum-norm least-squares solution; pseudoinverse is a separate primitive. The underdetermined/minimum-norm contract needs an explicit decision/test before claiming full acceptance. |
| LIN-03 — rank-aware/minimum norm | Partial | Numerical rank, SVD and pseudoinverse exist. Numerical rank is not symbolic rank; threshold and minimum-norm composition guarantees need a consolidated public contract. |
| LIN-04 — diagonal WLS | Implemented; focused tests | Square-root row scaling; tests cover zero/unequal/invalid weights and rank failures. |
| LIN-05 — GLS whitening | Implemented; composition tests | Cholesky, triangular solves and a GLS reference workflow are tested; no covariance inverse is formed in the workflow. |
| LIN-06 — SPD validation/factorization | Implemented; tested | Cholesky and definiteness classification have reconstruction and invalid-matrix coverage. Tolerance behavior remains floating-point policy, not exact symbolic classification. |
| LIN-07 — log determinant | Implemented; tested | Signed log-absolute-determinant from LU; stable Gaussian log density composes it with Cholesky and triangular solve. No separate covariance quadratic-form abstraction was added because current composition suffices. |
| LIN-08 — reductions/residuals | Partial | Common reductions/norms/statistics exist and have focused tests. Extreme-scale guarantees are not exhaustive; avoid claiming universal high-precision statistics. |
| OPT-01 — scalar/log-domain functions | Implemented; tested | Sigmoid, logit, softplus, log-sigmoid, log-sum-exp; domain/non-finite and extreme finite inputs are covered. |
| OPT-02/03/08 — objective, gradient, result and termination contracts | Implemented; tested | C callback/context contract, optional gradient/finite-difference path, owned result, counters and distinct termination reasons are documented and tested. No automatic differentiation. |
| OPT-04/07 — generic optimization/objective composition | Implemented; tested | Unconstrained BFGS with Armijo backtracking. It is a moderate-dimensional smooth deterministic optimizer, not a universal or stochastic optimizer. |
| RNG-01/02/03 — algorithm/state/reproducibility | Implemented; tested | Explicit opaque PCG-XSH-RR 64/32 state, seed/stream, cloning, fixed raw vectors. Reproducibility guarantees are deliberately narrower for floating-point transforms across different math libraries. |
| RNG-05 — uniform/normal variates | Implemented; tested | Fixed uniform mapping and uncached Box–Muller normal; deterministic and statistical sanity tests. |
| RNG-07 — multivariate normal | Implemented; tested | Explicit RNG plus Cholesky sampling for SPD covariance. Singular/PSD covariance is rejected in v1. |
| RNG-08/09 — index resampling | Implemented; tested | Index sampling with and without replacement; no model-specific bootstrap/estimator API. |
| RNG-10 — RNG quality/replay tests | Implemented at v1 scope | Deterministic vectors, replay, range/uniqueness and distribution sanity tests exist. These are not cryptographic-quality proofs or exhaustive statistical test batteries. |
| PROB-01 — Gaussian log density | Implemented; tested | Uses Cholesky, triangular solve and log determinant without covariance inversion. |
| PROB-02 — Gaussian sampling | Implemented; tested | Uses `mean + Lz`, explicit RNG state and failure-safe output publication. |
| PROB-03 — log-domain accumulation | Implemented; tested | Stable log-sum-exp is part of the scalar API. |
| PROB-04 — generic likelihood/objective composition | Implemented at C callback level | Callers can implement a deterministic objective/log-likelihood and optional gradient. No model-specific likelihood API or PHP callback bridge exists. |
| ENG-01/02/03/04 — ownership, statuses, checked sizes, non-finite policy | Implemented for current C primitives; tested | Opaque handles, explicit release, checked allocation arithmetic, focused statuses, finite/domain validation and failure-safe outputs are documented and covered by native tests. This is not a guarantee that every historical Matrix edge case has exhaustive adversarial coverage. |
| ENG-05/06/07 — native tests, PHPT/build, sanitizer/property checks | Implemented at current CI scope | Native focused/reference/property tests and ASan/UBSan runs are registered; PHP 8.2–8.5 Linux build/PHPT matrix and debug build pass. PHPTs currently test extension loading/version only because no PHP-facing numerical API exists. |
| ENG-09 — PHP API boundary | Decision recorded; implementation missing | [Boundary decision](statistical-api-boundary.md) explicitly says C-first implementation is not enough for userland readiness. No Matrix/RNG/optimizer/probability PHP classes or functions are registered in `numerus.c`. This is the principal release blocker. |
| ENG-10 — portability | Partial | CI validates Ubuntu PHP 8.2–8.5 and a debug build. Windows is not in CI and remains unverified. Distribution-transform floating-point results are not promised bit-identical across all platforms/libm implementations. |
| ENG-11 — documentation/examples | Implemented for C-internal APIs | Contracts and an executable strict-C11 composition example are linked from README and run in CI. The example documents C composition, not PHP userland usage. |

## P1/P2 disposition

- **P1: structured-storage exploitation by statistical algorithms** — no broad claim that every operation avoids materialization; optimize only when benchmarks justify a specialized path.
- **P1: optimizer constraints/bounds, Hessian API, L-BFGS** — explicitly deferred; current optimizer is unconstrained BFGS with optional gradient/finite differences.
- **P1: independent stream splitting/jump-ahead, additional distributions, broad special functions, distribution object hierarchy** — deferred pending concrete consumers. Distinct PCG stream selectors are not a blanket independence guarantee.
- **P1: portability beyond tested Linux matrix** — Windows is unverified.
- **P2: MCMC, Bayesian diagnostics, GP fitting, cryptographic RNG, GPU/tensor/arbitrary-precision support** — explicitly out of scope for v1.

## Release gate

**The statistical C foundation passes its configured Linux CI, but overall Numerus v1 is not release-ready for PHP userland packages.** The API-boundary decision itself records this condition. The missing PHP façade is not a documentation gap and must not be waived by updating the README.

Required follow-up:
1. Design and implement a minimal ownership-safe PHP façade for the Matrix/value operations and selected statistical primitives.
2. Add PHPT coverage for PHP argument/type/shape validation, error mapping, object lifetime (especially Matrix views), explicit RNG state and callbacks if exposed.
3. Decide and test the underdetermined/rank-deficient least-squares contract before claiming full LIN-02 acceptance.
4. Keep Windows portability as unverified until a Windows build/test job exists; do not silently claim cross-platform support.

This audit issue can be closed as an audit deliverable only if its outcome is recorded as **release gate blocked**, with the remaining work tracked separately. It must not be interpreted as a v1 release approval.
