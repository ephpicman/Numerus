# Numerus Release Roadmap

This document defines the **user-facing release milestones** for Numerus. It is the source of truth for what a version means to a PHP user. The [v1 statistical foundation roadmap](v1-statistical-foundation-roadmap.md) remains the detailed inventory of reusable numerical capabilities and engineering gaps; its priority labels are not, by themselves, a promise that every listed capability must ship in 1.0.

## Product principle

Numerus exposes reusable numerical primitives through a PHP extension. Consumers compose those primitives and implement statistical models in PHP or in their own packages. Numerus does not provide model-specific estimators such as `fit_ols()`, `fit_wls()`, `fit_gls()`, or `fit_mle()`.

A release milestone is defined by a reliable, documented end-to-end workflow a PHP user can build—not by the number of functions exposed. Prefer stable solves and factorizations (QR, SVD, Cholesky, and triangular solves) over explicitly forming matrix inverses.

## Milestones

| Release | User-facing goal | Scope boundary |
|---|---|---|
| **1.0 — OLS-enabling foundation** | A PHP user can construct design/response matrices, compose operations, estimate OLS coefficients, and validate the result. | Ownership-safe PHP matrix API; multiplication/transpose and suitable stable solve/least-squares paths; explicit rank-deficiency semantics; PHPT/native tests, runnable PHP example, documentation, supported CI. No `fit_ols()`. |
| **1.1 — WLS-enabling foundation** | A PHP user can implement weighted least squares from primitives. | Weight application and composition using existing primitives; zero/unequal/invalid weights, scaling, and reference-equivalence tests. No `fit_wls()`. |
| **1.2 — GLS-enabling foundation** | A PHP user can implement GLS with non-diagonal covariance. | Compose Cholesky, triangular solves/whitening, log-determinant where needed, and covariance validation. Avoid explicit covariance inversion. No `fit_gls()`. |
| **1.3 — Linear-model inference primitives** | Users can compose common uncertainty and inference calculations for linear models. | Residuals, degrees of freedom, coefficient-covariance building blocks, and only the distribution functions required by a clearly scoped workflow. No ready-made model fitting. |
| **1.4 — MLE-enabling foundation** | A PHP user can implement a likelihood and optimize it from PHP. | Generic objective/gradient callback surface, optimizer results and termination diagnostics, numerical-gradient behavior, failure semantics, and examples. No model-specific likelihood/estimator API. |
| **1.5+ — Further probability/inference capabilities** | Add broadly reusable capabilities in response to demonstrated consumer needs. | Consider resampling/bootstrap primitives, distribution functions, and Gaussian/Bayesian foundations individually. Do not commit speculatively to MCMC, GP fitting, or broad frameworks. |

Milestones after 1.0 are provisional planning targets. Combine or split minor releases when implementation size, dependencies, and release quality justify it.

## 1.0 definition of done

1. Documented PHP APIs construct matrices and compose the operations required for an OLS workflow.
2. An end-to-end PHP example runs from input data to coefficients without exposing C pointers or internal storage.
3. Stable least-squares solving is available; rank-deficient and underdetermined behavior is explicit and tested. Explicit inverse formation is not the default recommended path.
4. PHP object lifetime is safe, including retention of parent PHP objects for Matrix views whose C references are non-owning. C statuses map consistently to PHP errors/exceptions, and allocation/validation failures preserve documented output invariants.
5. Native tests cover ordinary cases, edge cases, numerical properties/reference fixtures, and failure paths. PHPT covers success, invalid types/shapes, lifetime, and public error behavior.
6. Supported PHP 8.2–8.5 CI and required debug/sanitizer checks pass. Unverified platforms, including Windows until its build/test job passes, are clearly identified.
7. API contracts, numerical limitations, and the supported workflow are documented. README does not claim PHP-userland readiness before these gates pass.

## Execution order

The original implementation sequence is complete: [#147 — Minimal PHP-facing numerical API](https://github.com/ephpicman/Numerus/issues/147) was delivered in PRs #194–#197, and [#148 — rank-deficient/underdetermined least-squares semantics](https://github.com/ephpicman/Numerus/issues/148) was resolved and its PHP-visible behavior is covered by PHPTs.

The current release sequence is:

1. Keep the release-gate audit synchronized with the shipped PHP API and tests.
2. Verify post-merge main CI for the intended release-candidate commit: PHP 8.2–8.5, debug build, native coverage, and all PHPTs.
3. Confirm the executable OLS example and published PHP API documentation match the current implementation.
4. Keep Windows explicitly unverified until a Windows build and test job passes.
5. Only then decide whether the 1.0 release gate is satisfied; do not equate merged feature PRs with release approval.

## Relationship to the foundation roadmap

The [v1 statistical foundation roadmap](v1-statistical-foundation-roadmap.md) tracks a broader set of C-level capabilities, including optimization, RNG, and probability primitives. Its P0/P1 labels prioritize work within that foundation inventory; they do **not** mean that every P0 capability is mandatory for product release 1.0. The 1.0 release gate is the OLS-enabling PHP workflow above. Later milestones select the next coherent user-facing workflow from the broader inventory.

The [issue execution order](v1-issue-execution-order.md) records the historical foundation queue and its completion evidence. The [PHP 1.0 OLS release-gate audit](php-api-release-audit.md) tracks concrete userland evidence and remaining limitations. For new release work, follow the release milestone and execution order in this document, while preserving unresolved foundation requirements where they are dependencies or necessary correctness fixes.

## Non-goals

- Model-specific `fit_ols()`, `fit_wls()`, `fit_gls()`, or `fit_mle()` functions in Numerus.
- Recommending explicit matrix inversion as the normal numerical path.
- Adding features only to fill a version number.
- Declaring release readiness based on native tests alone or before post-merge release-candidate CI is green.
