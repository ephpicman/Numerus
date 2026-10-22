# PHP 1.1 WLS Release-Gate Audit

This audit tracks the user-facing 1.1 milestone in the [release roadmap](release-roadmap.md). The prior [1.0 OLS audit](php-api-release-audit.md) remains historical evidence for the initial API.

## Acceptance evidence

| Gate | Evidence | Assessment |
|---|---|---|
| Public PHP API | `Matrix::weightedLeastSquares(Matrix $rightHandSide, array $weights): Matrix` is registered with named-argument metadata | Implemented; named arguments are covered by `tests/013-weighted-least-squares-named-arguments.phpt` |
| Numerical algorithm | Native implementation scales design and RHS rows by square-root weights and delegates to pivoted QR; it does not form `XᵀWX` | Implemented |
| OLS equivalence | Equal weights reproduce `leastSquares()` on a deterministic fixture | Covered by `tests/011-weighted-least-squares.phpt` |
| Weight semantics | Unequal weights, zero weight, negative weight, non-finite weight, wrong count, and non-numeric type | Covered by PHP and native WLS tests |
| Shape and data validation | Multiple RHS, RHS row mismatch, non-finite matrix values, infinite weights, and rank/solver failures | Covered by native tests and PHP-facing validation paths |
| Runnable user workflow | `examples/wls.php` checks expected coefficients; `tests/012-wls-example.phpt` executes it end to end | Implemented |
| User-facing documentation | README, user guide, API reference, documentation index, changelog, and roadmap describe WLS and its limits | Updated for 1.1 |
| Version and clean archive | `php_numerus.h` identifies version 1.1.0; source-archive workflow verifies that version and runs OLS and WLS examples | Workflow updated; final result must be green on the exact release commit |
| Supported Linux CI | PHP 8.2–8.5 matrix, debug build, PHPT suite, native tests, sanitizer-oriented jobs, native coverage | Must be verified on the exact release-preparation commit and again after merge to `main` |
| Windows | `config.w32` exists, but Windows build/test CI is not present | Unverified; do not claim Windows support is validated |

## Public contract and known limitations

- The design matrix has shape `n × p`; the RHS has `n` rows and may contain one or more columns. The result has shape `p × k`.
- Supply exactly one PHP integer or float weight per observation. Weights must be finite and non-negative. Numeric strings are rejected.
- A zero weight removes that observation's contribution to the weighted objective. The current implementation still validates matrix entries in zero-weight rows; zero weight does not promise to ignore non-finite input data.
- The square-root-scaled design must have full column rank. Rank-deficient and underdetermined problems are rejected; the PHP API does not return a minimum-norm solution.
- The native implementation materializes scaled design and RHS matrices, requiring additional memory proportional to their sizes.
- Numerical solver failures are reported as exceptions; invalid dimensions and negative weights are reported as `ValueError`, non-numeric PHP weight values as `TypeError`, and non-finite weights as numerical exceptions.
- This release adds a reusable matrix primitive, not a model-specific `fit_wls()` API. GLS, inference workflows, and MLE workflows remain outside the 1.1 scope.

## Release decision

**Status: release preparation; not yet published.** Do not create tag `1.1.0` or publish a GitHub Release until the source-archive smoke test, the PHP 8.2–8.5 test matrix, native coverage, and post-merge `main` checks are green on the exact intended release commit. Review the final diff and changelog before tagging.
