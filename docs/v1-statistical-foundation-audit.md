# v1 Statistical Foundation — Source and Test Audit

Audit base: `main` at `eeeccfa0022748bb2848628dd4a7d828eb1cce22` (2026-10-10).

This is a source-tree audit, not a claim that CI was executed locally. The execution environment could not resolve `github.com`, so native tests were not run in this environment. Existing test files and workflow commands were inspected; the PR's GitHub Actions results must be checked before merge.

## Scope and method

Inspected the public internal C API in `numerus_matrix.h`, numerical helpers in `numerus_numeric.h`, the solve/least-squares/weighted-least-squares/Cholesky/LDLT/system/statistics implementations, relevant native tests, the PHPT files, and `.github/workflows/tests.yml`.

There are no PHP-visible Matrix objects or statistical functions today: `numerus.c` is an extension skeleton and the Matrix API is C-internal. The current codebase is a Matrix/Storage foundation, not yet a statistical API.

## Evidence-based requirement status

Status meanings in this audit:
- **Present, test evidence found**: implementation and a named native test exist; this does not claim every edge case is covered.
- **Partial**: a related primitive exists, but one or more acceptance guarantees or compositions are absent.
- **Missing**: no matching implementation/API was found in the audited tree.
- **Decision**: requires an explicit contract/scope decision before implementation.
- **Deferred/out of scope**: retain the roadmap's stated non-goal.

| ID | Audited status | Source/test evidence and gap |
|---|---|---|
| LIN-01 | Partial — core solve exists | `numerus_matrix_multiply.c`, `numerus_matrix_solve.c`; `tests/matrix_multiply_test.c`, `tests/matrix_solve_test.c`. Solve supports multiple RHS and rejects singular systems/non-finite RHS. The solve uses LU; broader scale-aware stability is not established by the presence of the API alone. |
| LIN-02 | Partial — full-column-rank least squares exists | `numerus_matrix_least_squares.c`, `numerus_matrix_qr.c`; `tests/matrix_least_squares_test.c`, `tests/matrix_qr_test.c`. Pivoted QR is used, but `numerus_matrix_least_squares()` returns `RANK_DEFICIENT` rather than a rank-deficient minimum-norm solution. Pseudoinverse is separate. Underdetermined behavior needs an explicit acceptance test/contract. |
| LIN-03 | Partial — primitives exist | `numerus_matrix_rank()`, SVD and pseudoinverse implementations; `tests/matrix_rank_test.c`, `tests/matrix_svd_test.c`, `tests/matrix_pseudoinverse_test.c`. Numerical rank is not symbolic rank; public contract and threshold guarantees need to be consolidated. |
| LIN-04 | Present, test evidence found | `numerus_matrix_weighted_least_squares.c`; `tests/matrix_weighted_least_squares_test.c` covers equal weights, zero-weight exclusion, invalid/non-finite weights, rank failure and callback read failure. Implementation square-root-scales rows and then calls least squares. |
| LIN-05 | Missing — confirmed gap | Cholesky exists in `numerus_matrix_cholesky.c`, and general solve exists in `numerus_matrix_solve.c`, but no public triangular-solve API or composed covariance-whitening/GLS path was found. Cholesky alone does not provide whitening. Track implementation only after #111 defines the contract (#113). |
| LIN-06 | Partial — factorization/classification exist | `numerus_matrix_cholesky.c`, `numerus_matrix_definiteness.c`; `tests/matrix_cholesky_test.c`, `tests/matrix_definiteness_test.c`. Tests cover reconstruction and several SPD/PSD/indefinite/scale cases. Symmetry and pivot decisions use a tolerance; policy and edge cases should be documented consistently. |
| LIN-07 | Missing/partial — determinant exists, log determinant and quadratic-form API do not | `numerus_matrix_determinant()` and LU exist; `tests/matrix_determinant_test.c`. No log-determinant or reusable covariance/precision quadratic-form API was found. Do not implement a naïve `log(det(A))` path; #114 is conditional on #111's contract. |
| LIN-08 | Partial — basic reductions exist | `numerus_matrix_statistics.c`, matrix norm/trace/sum/min/max functions; `tests/matrix_statistics_test.c`, `tests/matrix_norm_test.c`, `tests/matrix_finite_test.c`. Variance uses an online update, but comprehensive extreme-scale/stability guarantees are not established. |
| LIN-09 | Partial — representations exist, algorithm use varies | Storage supports dense, triangular, diagonal, identity, constant, zero, scaled identity, sparse, symmetric and banded forms; see `numerus_storage.h/.c`, `docs/storage.md`. No broad evidence that statistical algorithms exploit these representations without materialization. P1; benchmark before specialization. |
| OPT-01 | Partial — matrix numeric helpers only | `numerus_numeric.h` provides finite approximate comparisons and zero/one checks. No selected scalar `log1p`, `expm1`, logistic/logit or log-sum-exp API was found. Scope must be chosen in #112 before #118. |
| OPT-02 | Missing | No objective callback/problem/result contract found in C headers or implementations. Define in #110. |
| OPT-03 | Decision needed | No optimizer derivative callback API exists. Decide caller-supplied gradient and whether any finite-difference helpers are in scope in #110; do not imply automatic differentiation. |
| OPT-04 | Missing | No generic continuous optimizer implementation or optimizer tests found. Contract/algorithm selection precedes #119. |
| OPT-05 | Decision needed | No optimizer constraints/bounds contract exists. Decide whether to defer them in #110; do not invent a broad constraint framework. |
| OPT-06 | Missing/decision needed | No optimizer Hessian/curvature API found. Revisit only after optimizer use cases establish whether it is necessary. |
| OPT-07 | Partial | Matrix arithmetic and reductions can support caller-written objectives, but no objective callback contract or selected stable scalar/log-domain primitives exists. No model-specific likelihood API should be added. |
| OPT-08 | Missing | Matrix statuses distinguish several failures, but there is no optimizer termination taxonomy or convergence result structure. Define in #110. |
| RNG-01 | Missing | No PRNG implementation or algorithm contract found. #109 must choose the contract before #115. |
| RNG-02 | Missing | No explicit opaque RNG state/lifecycle API found. |
| RNG-03 | Missing | No RNG vectors, seed/state replay contract, or cross-version/platform reproducibility promise found. |
| RNG-04 | Decision needed | No stream splitting/jump API found. Treat independent streams as a separate decision; different seeds alone are not proof of independence. |
| RNG-05 | Missing | No uniform/normal random variate implementation found. |
| RNG-06 | Decision needed/missing | No distribution API found; select only concrete consumers after the base RNG and v1 needs are clear. |
| RNG-07 | Partial foundation only | Matrix Cholesky exists, but there is no RNG or multivariate-normal sampler. |
| RNG-08 | Missing | No sampling-with/without-replacement index API found. |
| RNG-09 | Missing | No generic resampling/bootstrap index primitive found. |
| RNG-10 | Missing | No RNG-specific deterministic vectors or statistical sanity tests found. Existing Matrix property tests are not RNG-quality tests. |
| RNG-11 | Out of scope | Roadmap correctly excludes cryptographic randomness. |
| PROB-01 | Missing as a composed primitive | Solve and Cholesky exist, but no triangular solve, stable log determinant, or multivariate Gaussian log-density API exists. |
| PROB-02 | Missing | No multivariate Gaussian sampler; requires RNG plus an explicit covariance-factor policy. |
| PROB-03 | Missing | No log-sum-exp/log-domain accumulation helper found. |
| PROB-04 | Blocked by missing contract | Callers can write C callbacks themselves, but there is no agreed optimizer objective callback/result API (#110), so this is not an established reusable contract. |
| PROB-05 | Deferred | MCMC remains out of scope for v1. |
| PROB-06 | Deferred | Bayesian diagnostics remain out of scope absent a concrete low-level consumer. |
| PROB-07 | Deferred | No GP model fitting in Numerus v1; matrix primitives only. |
| PROB-08 | Decision needed | No distribution object/API hierarchy exists. Avoid one until concrete reusable distributions are selected. |
| PROB-09 | Decision needed | No special-function API found. Choose exact functions only from accepted algorithms. |
| ENG-01 | Partial — existing C foundation | Opaque Storage/Matrix APIs and documented ownership exist (`numerus_storage.h`, `numerus_matrix.h`, `docs/storage.md`, `docs/matrix.md`). New callback/RNG lifetime and reentrancy contracts still need design. |
| ENG-02 | Partial — existing Matrix statuses | `numerus_matrix_status` distinguishes invalid input, overflow, allocation failure, non-finite, singularity, rank deficiency, symmetry/definiteness and non-convergence. Optimizer/RNG/probability-specific statuses do not exist yet. |
| ENG-03 | Present, test evidence found | `numerus_size.h` supplies checked size arithmetic and is used in numerical allocation paths; native allocation-failure/adversarial tests exist. Each new API must follow this pattern. |
| ENG-04 | Partial | Matrix functions commonly reject non-finite inputs with `NUMERUS_MATRIX_NON_FINITE`; `numerus_numeric.h` documents comparison behavior. There is no unified policy for future scalar functions, RNG transforms, callbacks and probability primitives. |
| ENG-05 | Present foundation, incomplete scope | Native tests cover Storage and Matrix; `tests/001-load.phpt` and `tests/002-version.phpt` cover PHP-visible skeleton behavior. Statistical API tests do not exist because those APIs do not exist. |
| ENG-06 | Partial — sanitizer coverage is now explicit for the v1 statistical APIs | `.github/workflows/tests.yml` already had an AddressSanitizer/UBSan step for Matrix algebra property tests inside the PHP matrix job (not a separate sanitizer job). This issue adds a cross-API statistical property binary and runs it normally and under ASan/UBSan. CI status still must be verified from the actual run; workflow presence alone is not proof of green checks. |
| ENG-07 | Partial — Matrix property/reference tests exist | `tests/matrix_property_test.c` and operation-specific tests validate matrix invariants. OLS/WLS/GLS composition, RNG replay, optimizer and probability property tests are absent. |
| ENG-08 | Present foundation | `benchmarks/matrix_benchmark.c`, `benchmarks/README.md`, and `benchmarks/baseline-2026-10-09.md` provide a Matrix benchmark harness/baseline. No statistical primitive benchmarks exist yet. |
| ENG-09 | Decision needed — C-only today | Matrix is internal C API; `numerus.c` exposes no Matrix/statistical object or function surface. Decide C-only versus PHP-visible scope before adding statistical APIs (#108). |
| ENG-10 | Partial | `config.m4`, `config.w32`, and workflow build definitions exist. The audited files do not by themselves establish that every supported PHP/compiler/platform combination is currently passing. RNG reproducibility may have additional platform constraints. |
| ENG-11 | Partial — Matrix docs exist, statistical docs do not | `docs/matrix.md`, `docs/storage.md`, and Matrix contracts/roadmap document current primitives. No statistical C contracts/examples exist yet. Add examples only as APIs are accepted and merged. |

## Confirmed implementation gaps and issue mapping

- **#113**: justified. The current API has Cholesky and general square solve but no triangular solve or whitening composition.
- **#114**: justified provisionally. Determinant exists; stable log determinant is absent. The exact API and sign/singularity semantics must be decided in #111 before implementation.
- **#122**: still needed as composition tests. Existing tests cover OLS-like least squares and WLS primitives individually; they do not establish a GLS covariance-whitening workflow.
- **#123**: remains needed for statistical property/sanitizer coverage, with sanitizer support first verified against actual CI constraints.
- **#124**: remains needed for statistical contracts and examples.
- Optimizer/RNG/scalar/probability gaps remain represented by #109, #110, #112, #115–#121. Do not create duplicate issues for already-tracked work.

## Decisions for the next issues

1. Keep Matrix/Storage as C-internal APIs unless #108 explicitly approves a PHP-visible surface.
2. Do not add model-specific estimator functions.
3. Resolve GLS whitening and log determinant contracts in #111 before #113/#114 implementation.
4. Specify scalar functions in #112 before implementing them.
5. No RNG implementation before #109; no optimizer implementation before #110.
6. This report inventories source and tests; it does not claim tests passed. Merge requires green required checks on the PR.


## 2026-10-10 implementation follow-up

The later implementation work is recorded in [the final acceptance audit](v1-statistical-foundation-final-audit.md). The older source-audit rows above describe the pre-implementation baseline and must not be read as current status. The final audit supersedes those baseline statuses for release-gate decisions.
