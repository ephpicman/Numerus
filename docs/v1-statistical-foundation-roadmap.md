# Numerus v1 Statistical and Numerical Foundation Roadmap

This document defines the **product-level v1 acceptance scope** for Numerus. It complements, rather than replaces, the completed Matrix implementation history in [Matrix Operations Roadmap](matrix-operations-roadmap.md) and the operation contracts in [Matrix Operation Contracts](matrix-operation-contracts.md).

## 1. Product boundary

Numerus is a low-level C numerical foundation exposed through a PHP extension. It is **not** responsible for fitting named statistical models. Model packages should be able to implement OLS, GLS, maximum-likelihood estimation, Bayesian inference, Gaussian models, bootstrap, and sampling using composable primitives provided here.

The v1 goal is not to implement every statistical algorithm. It is to make the foundational numerical, optimisation, probability, and random-number facilities sufficiently complete, stable, testable, and documented for those algorithms to be implemented outside the extension.

### Accepted constraints

- Matrix and Storage remain immutable; matrix values remain real C `double`.
- Complex numbers, arbitrary precision, GPU execution, and a general tensor API are outside v1.
- Do not implement model-specific APIs such as `fit_ols()`, `fit_gls()`, `fit_gaussian_process()`, or a generic Bayesian model framework.
- Do not add a dependency on a large third-party numerical library without a separate design and licensing/build decision.
- Keep C algorithms independently testable; PHP bindings must preserve C ownership, error, and reproducibility contracts.
- Prefer solves and factorizations over explicit matrix inversion.
- Add capabilities only where they support multiple plausible consumers or are necessary to make a declared v1 use case reliable.
- All requirements below are proposals until merged implementation and tests establish their status. “Existing” means supported by current repository documentation/inventory; it is not a fresh independent source-code audit of every implementation.

## 2. Requirement status vocabulary

| Status | Meaning |
|---|---|
| **Existing** | The repository documents this primitive as implemented. Validate its contract and tests when integrating it into a higher-level workflow. |
| **Partial** | Some underlying support exists, but a specific API, numerical guarantee, or important edge case remains. |
| **Missing** | No implementation is currently documented; requires design and implementation if retained in v1. |
| **Decision needed** | Scope/contract must be decided before implementation; do not silently choose an algorithm or promise. |
| **Deferred** | Not required for v1; revisit only with a concrete consumer and justification. |

Priority: **P0** blocks the declared v1 foundation; **P1** is a high-value v1 capability after P0; **P2** is conditional or post-v1.

## 3. Requirements matrix

### A. Linear models: OLS and GLS

| ID | Requirement | Current baseline | Priority | Acceptance criteria |
|---|---|---|---|---|
| LIN-01 | Matrix/vector products and multi-RHS solves | **Existing**: matrix multiplication and square-system solve with multiple RHS | P0 | Shape/overflow/failure contracts; residual tests; no inverse-times-vector default path. |
| LIN-02 | Stable least-squares solve | **Existing**: pivoted-QR least squares; rank-deficient/minimum-norm work supported through SVD/pseudoinverse | P0 | Over-, under-, and exactly determined systems; compare residual and solution properties to trusted small reference cases. |
| LIN-03 | Rank-aware/minimum-norm solution | **Existing**: numerical rank, SVD, pseudoinverse | P0 | Rank thresholds documented; rank-deficient and badly scaled tests; no claim of symbolic rank. |
| LIN-04 | Weighted least squares with diagonal weights | **Existing**: finite nonnegative weights applied through square-root row scaling | P0 | Zero weights, unequal scales, invalid weights, and equivalence to explicitly transformed reference problem. |
| LIN-05 | General GLS covariance weighting | **Partial**: Cholesky and general solves exist; a documented covariance-whitening path is not established | P0 | Solve/triangular-solve primitives or an equivalent stable whitening path; avoid explicitly forming covariance inverse; validate against (X^T\Sigma^{-1}X) reference on small well-conditioned cases without using that approach as production implementation. |
| LIN-06 | Symmetric positive-definite validation and factorization | **Existing/partial**: Cholesky and definiteness classification documented | P0 | Clear finite/symmetry/positive-definiteness failure semantics; reconstruction tests and scale-aware behavior. |
| LIN-07 | Covariance/precision quadratic forms and log determinant | **Partial/missing**: solve and determinant exist; a stable documented log-determinant API is not established | P0 | Compute log determinant from a suitable factorization; correct sign/singularity behavior; quadratic forms avoid explicit inverse. |
| LIN-08 | Residuals and scalar reductions | **Existing**: arithmetic, aggregates, norms, variance/stddev | P0 | Stable enough reductions for common workloads; document NaN/infinity behavior and test extreme scales. |
| LIN-09 | Matrix-valued covariance operations and structured storage use | **Partial**: specialized Storage representations exist; algorithm-level exploitation varies | P1 | Avoid densification when an existing representation enables a materially cheaper correct path; benchmark before adding specialized branches. |

### B. Numerical optimization and maximum likelihood

| ID | Requirement | Current baseline | Priority | Acceptance criteria |
|---|---|---|---|---|
| OPT-01 | Stable scalar numerical primitives | **Partial**: core matrix numeric policy exists; a broader scalar utility API is not established | P0 | Define scope for `log1p`, `expm1`, stable logistic/logit, `log-sum-exp`, and related functions; test extreme finite inputs and domain errors. |
| OPT-02 | Optimisation problem/result contract | **Missing** | P0 | Explicit objective callback, context lifetime, parameter dimensions, status, iteration/evaluation counts, termination reason, and output-preservation rules. |
| OPT-03 | Derivative interface | **Decision needed** | P0 | Start with caller-supplied objective/gradient callbacks; decide whether finite-difference gradient/Hessian helpers belong in v1. Do not imply automatic differentiation. |
| OPT-04 | General-purpose continuous optimiser | **Missing** | P0 | Choose a small supported initial set based on use cases; at minimum robust unconstrained smooth optimisation with convergence diagnostics and failure tests. Algorithm choice must be documented before implementation. |
| OPT-05 | Constraints and bounds | **Decision needed** | P1 | Decide whether v1 supports bounds/constraints or leaves them to higher-level packages; do not ship an ambiguous callback contract. |
| OPT-06 | Hessian and local curvature utilities | **Partial/missing** | P1 | Support caller-provided Hessian and/or documented numerical approximation if justified; test symmetry, scaling, and invalid evaluations. |
| OPT-07 | ML building blocks (log-likelihood accumulation, stable objective evaluation) | **Partial**: general numerical primitives exist; no model-specific likelihood API should be assumed | P0 | Generic scalar objective callbacks and stable arithmetic are sufficient; no named model estimators in the extension. |
| OPT-08 | Optimiser convergence and failure semantics | **Missing as a common API contract** | P0 | Distinguish converged, iteration limit, invalid objective/gradient, numerical breakdown, and allocation failure; never encode failure only as NaN. |

### C. Random number generation, sampling, and bootstrap

| ID | Requirement | Current baseline | Priority | Acceptance criteria |
|---|---|---|---|---|
| RNG-01 | Specified pseudorandom generator | **Missing** | P0 | Select and document algorithm, state size, seeding, stream semantics, and portability expectations before implementation. |
| RNG-02 | Explicit RNG state lifecycle | **Missing** | P0 | Create/seed/advance/clone or serialize-and-restore state through an opaque C API; ownership and invalid-state behavior tested. |
| RNG-03 | Reproducibility contract | **Missing** | P0 | Same algorithm/version, seed/state, and call sequence reproduce results on supported platforms; explicitly state whether distribution transforms are stable across releases/platforms. |
| RNG-04 | Independent streams | **Decision needed** | P1 | Provide a defensible stream-splitting/jump strategy if supported; do not claim independence merely because seeds differ. |
| RNG-05 | Uniform and normal variates | **Missing** | P0 | Document endpoint and distribution semantics; test determinism, moments/quantiles with non-flaky statistical tolerances, and extreme inputs. |
| RNG-06 | Other common distributions | **Missing / scope needed** | P1 | Select only general-purpose distributions justified by likely consumers; define parameter domains and stable algorithms. |
| RNG-07 | Multivariate normal sampling | **Partial foundation**: matrix decompositions exist; RNG and distribution composition are missing | P0 | Accept mean and valid covariance/factor; handle positive-semidefinite policy explicitly; deterministic output for fixed RNG state; test empirical covariance statistically. |
| RNG-08 | Sampling without replacement / index resampling | **Missing** | P0 | Deterministic index generation for a fixed state; define replacement, population-size, and sample-size semantics. |
| RNG-09 | Bootstrap-ready resampling primitives | **Missing** | P0 | Provide generic index/resampling primitives, not a model-fitting bootstrap API; test edge cases and reproducibility. |
| RNG-10 | Statistical quality tests | **Missing** | P0 | Reproducibility tests plus deterministic test vectors and non-flaky distribution sanity checks; document that these do not prove cryptographic quality. |
| RNG-11 | Cryptographic randomness | **Out of scope** | — | Numerus RNG is for simulation/numerical methods, not secrets or security tokens. |

### D. Gaussian and Bayesian model foundations

| ID | Requirement | Current baseline | Priority | Acceptance criteria |
|---|---|---|---|---|
| PROB-01 | Stable multivariate Gaussian log density | **Partial/missing**: matrix solves/decompositions exist; composed probability primitive is not established | P0 | Use factorization, triangular solves and log determinant; no explicit covariance inverse; validate against hand-computed low-dimensional cases. |
| PROB-02 | Multivariate Gaussian sampling | **Missing** | P0 | Compose RNG with a covariance factorization; deterministic with a fixed RNG contract; explicit handling of invalid covariance. |
| PROB-03 | Log-domain accumulation | **Missing / decision needed** | P0 | Stable `log-sum-exp` and related minimal helpers needed by likelihoods and Bayesian calculations; test extreme log weights. |
| PROB-04 | Generic log-density/objective composition | **Existing by callback design only after OPT-02** | P0 | Callers can implement priors, likelihoods, and posterior log density without model-specific extension APIs. |
| PROB-05 | MCMC engine | **Deferred** | P2 | Not required to make v1 a usable foundation. Revisit only after RNG, objective contracts, diagnostics scope, and a concrete consumer are established. |
| PROB-06 | Bayesian diagnostics (ESS, R-hat, divergence diagnostics) | **Deferred / scope needed** | P2 | These are inference-package concerns unless a compelling reusable low-level requirement is demonstrated. |
| PROB-07 | Gaussian Process implementation | **Deferred** | P2 | Provide reusable covariance matrix, factorization, solve, and log-density primitives; do not implement a GP model in Numerus v1. |
| PROB-08 | General probability distributions and distribution objects | **Decision needed** | P1 | Begin with a small set of sampling/math primitives; avoid a large object hierarchy before concrete API consumers exist. |
| PROB-09 | Special functions | **Decision needed** | P1 | Identify exact required functions from selected distribution and optimiser algorithms; do not implement a speculative full special-functions library. |

### E. Cross-cutting engineering and release quality

| ID | Requirement | Current baseline | Priority | Acceptance criteria |
|---|---|---|---|---|
| ENG-01 | C API contracts and ownership | **Existing foundation** | P0 | New callback/context and RNG APIs document ownership, lifetime, reentrancy and destruction. |
| ENG-02 | Error/status taxonomy | **Existing foundation; extensions needed as algorithms require** | P0 | Invalid input, non-convergence, singularity, numerical breakdown and allocation failure remain distinct. |
| ENG-03 | Checked dimensions and allocation arithmetic | **Existing**: `numerus_size.h` | P0 | All new buffer sizes and products use checked helpers; output values remain unchanged on failure. |
| ENG-04 | NaN/infinity and domain policy | **Partial** | P0 | Each new scalar/probability/optimisation operation documents non-finite handling and has boundary tests. |
| ENG-05 | Native tests and PHPT coverage | **Existing foundation** | P0 | Native tests for C contracts; PHPT tests for PHP-visible API; test failure paths, not just happy paths. |
| ENG-06 | Sanitizers, debug builds, supported PHP matrix | **Existing CI documented** | P0 | Keep required CI green; add sanitizer/property checks for new numerical primitives where feasible. |
| ENG-07 | Reference and property-based tests | **Partial/existing foundation** | P0 | Compare decompositions/solves against invariants and trusted fixtures; test repeatability and mathematical properties. |
| ENG-08 | Benchmarks | **Existing baseline harness** | P1 | Benchmark only where algorithm selection or representation matters; record environment and avoid noisy timing gates. |
| ENG-09 | Public PHP API shape | **Decision needed**: Matrix remains internal C API | P0 decision, implementation staged | Decide which v1 capabilities are C-internal only and which need PHP-visible wrappers; wrappers must not expose unstable internals prematurely. |
| ENG-10 | Portability | **Partial** | P0 | Validate supported PHP versions and Unix/Windows build expectations; document any platform-dependent RNG behavior. |
| ENG-11 | Documentation and examples | **Partial** | P0 | Document each contract and provide small examples showing how external packages compose primitives for OLS/GLS, likelihood, Gaussian density, and resampling without embedding model fitting in Numerus. |

## 4. Version 1 roadmap and exit gates

The existing Matrix roadmap phases 0–11 are implementation history for the matrix layer. Do not reopen completed phases merely because a new consumer is identified. Add focused follow-up PRs only where the requirement matrix exposes a real contract or capability gap.

### Phase A — Requirement and API decisions

- [ ] A1. Review this matrix against current headers, implementation and tests; correct any mistaken baseline statuses.
- [ ] A2. Decide the minimum v1 C API exposure and PHP-visible API boundary.
- [ ] A3. Specify RNG algorithm/state/reproducibility guarantees.
- [ ] A4. Specify optimiser callback/result/error contracts and choose the initial algorithm set.
- [ ] A5. Specify GLS covariance-whitening and log-determinant contracts.
- [ ] A6. Choose the initial probability/distribution function set based on Gaussian/ML/Bayesian consumers.

**Exit gate:** decisions recorded in API contracts before implementation; no algorithm is marked complete on the basis of a design note alone.

### Phase B — Linear-model numerical completeness

- [ ] B1. Audit multi-RHS solve and QR/SVD least-squares behavior against the matrix requirement IDs.
- [ ] B2. Add or expose stable triangular solves/whitening primitives needed for GLS, if the audit confirms the gap.
- [ ] B3. Add stable log-determinant from suitable factorization, if absent.
- [ ] B4. Add covariance quadratic-form helpers only if they provide a useful composable API beyond existing solves/products.
- [ ] B5. Test OLS and GLS *reference workflows in tests/examples only*; do not add OLS/GLS estimator APIs to Numerus.
- [ ] B6. Test rank deficiency, near singularity, scaling, zero weights, and ill-conditioned covariance matrices.

**Exit gate:** a consumer can implement OLS, diagonal WLS and general GLS without explicit inverse formation; examples and tests validate numerical behavior.

### Phase C — RNG and deterministic sampling

- [ ] C1. Implement opaque RNG state with specified algorithm and seed/state contract.
- [ ] C2. Add deterministic uniform and normal variates.
- [ ] C3. Add deterministic index sampling with and without replacement.
- [ ] C4. Add state replay/restore and stream semantics as accepted in Phase A.
- [ ] C5. Add statistical sanity tests that avoid flaky exact thresholds and include fixed deterministic test vectors.
- [ ] C6. Document portability/version guarantees and non-cryptographic purpose.

**Exit gate:** fixed state and call sequence reproduce the documented sequence; lifecycle, invalid inputs, and distribution behavior are tested.

### Phase D — Optimisation and likelihood foundations

- [ ] D1. Add stable scalar/log-domain helpers selected in Phase A.
- [ ] D2. Implement objective and optional gradient callback contracts with explicit context lifetime.
- [ ] D3. Implement the agreed initial continuous optimisation algorithm(s).
- [ ] D4. Return structured termination information and distinguish convergence from failure.
- [ ] D5. Add analytic test objectives with known optima, ill-scaled cases, invalid callback results, and iteration-limit tests.
- [ ] D6. Keep model-specific likelihoods and estimators outside the extension.

**Exit gate:** an external consumer can maximise a generic log-likelihood with caller-provided objective/gradient and diagnose why optimisation stopped.

### Phase E — Gaussian probability primitives

- [ ] E1. Implement multivariate Gaussian log density using factorisation, solves and log determinant.
- [ ] E2. Implement multivariate Gaussian sampling using the accepted RNG API and covariance-factor contract.
- [ ] E3. Add log-sum-exp and any other log-domain helpers selected in Phase A.
- [ ] E4. Test singular/semidefinite covariance policy, numerical scale, deterministic sampling, and reference log densities.
- [ ] E5. Provide examples showing how generic prior and likelihood callbacks can be composed, without shipping an inference framework.

**Exit gate:** common Gaussian likelihood/prior calculations can be composed without explicit covariance inversion, and sampling is reproducible under the documented contract.

### Phase F — Integration, quality, and v1 release audit

- [ ] F1. Map every P0 row to merged implementation, contract documentation and tests.
- [ ] F2. Resolve every P0 “Decision needed” item; no unresolved decisions hidden as implementation assumptions.
- [ ] F3. Run full native, PHPT, supported PHP-version, Debug, sanitizer and relevant property-test matrix.
- [ ] F4. Run targeted benchmarks for algorithmic choices; record results without making noisy microbenchmarks release gates.
- [ ] F5. Document non-goals and explicit limitations.
- [ ] F6. Update README status only after the release gates pass.
- [ ] F7. Review open PRs, CI results and branches; merge only green, reviewed changes and remove obsolete feature branches according to repository policy.

**v1 acceptance rule:** all P0 requirements are implemented and tested or explicitly removed from v1 scope through a documented decision. P1 items may remain only if they are not necessary to the stated OLS/GLS, ML, reproducible sampling, and Gaussian foundations. P2 items are not release blockers.

## 5. Explicit v1 non-goals

- Ready-made OLS, GLS, maximum-likelihood, Bayesian, Gaussian Process, or hierarchical model fitting.
- A full MCMC framework and convergence-diagnostics suite.
- Every probability distribution and special function.
- General non-symmetric matrix functions beyond individually justified algorithms.
- Complex numbers, arbitrary precision, GPU execution, general tensors, or cryptographic RNG.
- Speculative generic caching, generic automatic differentiation, or large frameworks without demonstrated consumers.

## 6. Definition of done for every requirement

A row can be marked **Existing/complete** only when:
1. The API and numerical contract are explicit.
2. The implementation is merged into `main`.
3. Native tests cover normal, boundary, failure, and numerical edge cases as applicable.
4. PHPT/build checks cover PHP-visible behavior.
5. Required CI passes.
6. Documentation and relevant examples are updated.
7. Performance-sensitive decisions have evidence when appropriate.

A passing ordinary example is not proof of numerical stability. A seed alone is not a reproducibility contract. A matrix inverse is not an acceptable substitute for a stable solve in production algorithms.


## 7. Source audit (2026-10-10)

The original “Current baseline” column records the planning inventory and must not be read as independent verification. The first source-and-test audit is recorded in [v1 Statistical Foundation Audit](v1-statistical-foundation-audit.md). That audit maps every LIN, OPT, RNG, PROB and ENG requirement to inspected source/tests and distinguishes implemented matrix primitives from missing statistical compositions.

Key status corrections from the source audit:

- **LIN-02 — Partial:** pivoted-QR least squares exists for full-column-rank systems; the least-squares API rejects rank-deficient input. Pseudoinverse is a separate primitive, not an automatic minimum-norm least-squares path.
- **LIN-05 — Missing composition:** Cholesky and general solve exist, but no triangular-solve API or covariance-whitening/GLS composition was found. #113 is a confirmed gap, contingent on #111's accepted contract.
- **LIN-07 — Missing stable log determinant:** determinant exists, but no log-determinant or reusable covariance quadratic-form API was found. #114 is a confirmed gap, contingent on #111's accepted contract.
- **ENG-06 — Partial:** the workflow defines native tests, PHPT/build checks and a debug-oriented build; no dedicated sanitizer job was identified in the inspected workflow. A workflow definition is not proof of a green run.
- **ENG-09 — Decision needed:** Matrix remains a C-internal API; the extension does not currently expose Matrix or statistical functions to PHP. Resolve the boundary in #108 before designing new APIs.

Other planned optimizer, RNG, scalar log-domain and Gaussian probability primitives were not found in the audited source tree and remain implementation/contract work as mapped in the audit. These findings do not imply that the current Matrix subsystem is incomplete outside the specific statistical-foundation requirements.


## 8. API boundary decision

The C-first implementation and PHP façade release gate are defined in [Statistical Foundation API Boundary](statistical-api-boundary.md) (issue #108). New statistical algorithms are to be specified and tested as internal C APIs first. This does not make the C-only foundation userland-ready: overall v1 readiness requires a separately designed and tested minimal PHP façade, with ownership-safe Matrix views, explicit RNG state, and documented mapping of C failures. No model-specific estimator APIs are authorized by this decision.


## 9. GLS and log-determinant contract

The accepted design contract for the GLS covariance-whitening path and signed log-absolute-determinant is recorded in [GLS Whitening and Log-Determinant Contracts](gls-whitening-and-log-determinant-contracts.md) (issue #111). The audit confirms #113 (triangular solve/whitening) and #114 (stable signed log determinant) as real implementation gaps. Neither capability is considered implemented by the design document; their implementations and executable native tests remain separate issues.


## 10. RNG algorithm and reproducibility decision

The v1 RNG algorithm/state contract is specified in [Reproducible RNG Contract](reproducible-rng-contract.md) (issue #109). It selects PCG-XSH-RR 64/32, explicit 64-bit seed and stream inputs, opaque mutable state with cloning, exact raw-output vectors, and conservative platform guarantees. The raw state implementation is now #115; uniform/normal mapping remains #116, and stream-splitting/independence claims remain out of scope unless separately justified.


## 11. Optimizer contract decision

The initial optimizer contract is specified in [Generic Optimizer Callback and Result Contract](optimizer-contract.md) (issue #110): unconstrained BFGS with Armijo backtracking, optional caller-supplied gradients with a deterministic central finite-difference fallback, explicit result ownership, and distinct termination reasons. The document defines a bounded initial algorithm, not a claim of implementation; #119 remains the implementation issue. Constraints, automatic differentiation, Hessian APIs, stochastic objectives and model-specific estimators are deferred.


## 12. Scalar/log-domain utility selection

The selected v1 scalar set is documented in [Scalar and Log-Domain Utility Selection](scalar-log-domain-utilities.md) (issue #112): stable sigmoid, logit, softplus, log-sigmoid and log-sum-exp. Numerus will use C standard `log1p()`/`expm1()` internally instead of wrapping them without a demonstrated need. Broad special functions and distribution catalogs remain deferred. Implementation and edge-case tests are tracked by #118.


## 13. Portability and non-finite policy

The supported/tested build matrix and numerical input/error policy are recorded in [Numerical Portability and Non-Finite Policy](numerical-portability-and-nonfinite-policy.md) (issue #125). CI currently validates PHP 8.2–8.5 on Ubuntu and a debug build; Windows has a configuration file but is not validated by CI, so the README now states that limitation. New statistical APIs must follow the finite-input, domain-error, status-separation and output-preservation rules in this policy.


## 14. Triangular solve implementation

Issue #113 is implemented by adding `numerus_matrix_solve_triangular()`, including transpose and multiple-RHS support, strict triangular-structure validation, finite checks, and failure-safe output handling. Native tests are registered in CI. The API is an internal C primitive; covariance whitening/GLS composition tests remain tracked by #122. This section becomes complete only after the implementation PR's CI is green and merged.


## 15. Stable signed log determinant

Issue #114 adds `numerus_matrix_log_determinant()`, calculating the determinant sign and log absolute magnitude from LU factors without multiplying pivots. It validates finite inputs and factorization intermediates, distinguishes singularity, and preserves scalar outputs on failure. Native tests cover identity, positive/negative determinant, SPD, row permutation, ill-conditioning, extreme scales, singularity and non-finite input. GLS/Gaussian composition tests remain separate from this primitive.


## 16. OLS/WLS/GLS reference workflows

Issue #122 adds a native composition test covering overdetermined OLS, ill-scaled and rank-deficient least squares, zero and unequal diagonal WLS weights, and GLS covariance whitening through Cholesky plus triangular solves. The GLS fixture checks the signed log determinant of the covariance and verifies non-SPD covariance rejection. These are tests of reusable primitives, not new estimator APIs.


## 17. PCG32 RNG state implementation

Issue #115 implements the opaque PCG-XSH-RR 64/32 state API with explicit seed/stream inputs, deterministic raw output, cloning and safe destruction. The exact eight-output reference vector in [the RNG contract](reproducible-rng-contract.md) was checked against the reference algorithm and corrected to `a15c02b7, 7b47f409, ba1d3330, 83d2f293, bfa4784b, cbed606e, bfc6a3ad, 812fff6d`. The native test is registered in CI. Uniform and normal variates remain issue #116.


## 18. Uniform and normal variates

Issue #116 adds `numerus_rng_uniform()` with a fixed 53-bit mapping to `[0,1)` and `numerus_rng_normal()` using Box–Muller without cached state. Uniform consumes two raw PCG32 outputs; normal consumes four. Native tests check exact uniform mapping, a tolerance-based normal reference, state advancement, invalid arguments, finite/range guarantees and deterministic mean/variance sanity bounds. The CI workflow runs both ordinary and ASan/UBSan test binaries. Cross-platform bitwise normal output is not promised.


## 19. Deterministic index sampling

Issue #117 adds generic index sampling with and without replacement, explicit RNG state, unbiased bounded-integer rejection sampling, and randomized order for no-replacement results. Tests verify exact vectors, bounds, uniqueness, full/empty sample semantics, invalid sizes, checked allocation overflow, OOM output preservation and unchanged RNG state on pre-sampling failure. The native test is included in the normal and ASan/UBSan CI paths.


## 20. Stable scalar and log-domain primitives

Issue #118 implements the selected `numerus_numeric_*` C APIs: sigmoid, logit, softplus, log-sigmoid and log-sum-exp. They reject non-finite inputs, distinguish logit domain errors, preserve outputs on failure, and use stable sign-aware/max-shift formulas. Native tests cover extreme finite values, endpoints, invalid domains, input ordering and output preservation; CI runs them normally and under ASan/UBSan. Standard `log1p()`/`expm1()` remain internal C math functions, not duplicate public wrappers.


## 21. Generic continuous optimizer

Issue #119 implements the contract in [optimizer-contract.md](optimizer-contract.md): unconstrained BFGS with Armijo backtracking, optional analytic gradient or central finite differences, an opaque owned result, evaluation counters, and explicit convergence/termination reasons. Native tests cover quadratic and Rosenbrock references, finite differences, ill-conditioned scaling, iteration/small-step/line-search stops, callback failures, non-finite values, invalid inputs and injected allocation failures. CI runs normal and ASan/UBSan variants.


## 22. Multivariate Gaussian log density

Issue #120 implements `numerus_multivariate_gaussian_log_density()` for column-vector observation/mean and SPD covariance. It uses Cholesky, a triangular solve and signed log determinant; it never forms the covariance inverse. Inputs are borrowed, scalar output is preserved on failure, and matrix errors are mapped to a focused probability status. Native tests cover standard-normal and correlated-covariance reference values, non-SPD/non-finite inputs, dimension errors, arithmetic overflow and allocation failure; CI runs normal and ASan/UBSan variants.
