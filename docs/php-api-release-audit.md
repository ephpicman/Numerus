# PHP 1.0 OLS Release-Gate Audit

This audit evaluates the user-facing 1.0 target in [the release roadmap](release-roadmap.md), not every item in the broader statistical-foundation inventory.

## Acceptance evidence

| Gate | Evidence | Assessment |
|---|---|---|
| Construct design and response matrices from PHP | `Numerus\\Matrix::fromRows()`, `zeros()`; PHPT construction/access tests | Implemented |
| Compose the explicit normal-equation identity | `tests/004-ols-composition.phpt` checks `(XᵀX)⁻¹Xᵀy` | Implemented; documented as a non-default numerical path |
| Recommended stable least-squares path | `Matrix::leastSquares()` uses the existing pivoted-QR solver; `examples/ols.php` is an executable example and `tests/008-ols-example.phpt` invokes it | Implemented for full-column-rank designs |
| Numerical property/reference coverage | Native least-squares tests cover exact multiple RHS, a noisy overdetermined case, residuals, rank-deficient/underdetermined failures, and non-finite inputs. PHP-visible tests cover the main successful composition; `tests/009-least-squares-contracts.phpt` extends the PHP boundary checks. | Implemented; CI evidence must remain green |
| Rank-deficient and underdetermined behavior | The QR least-squares API returns rank-deficient status; the PHP façade maps this to `Exception`. Minimum-norm solutions can be composed at C level with the pseudoinverse, but that primitive is not currently exposed in PHP. | Explicit rejection is tested; PHP minimum-norm composition is not exposed |
| PHP ownership and lifecycle | Returned operations own independent native matrices; transpose lifetime and clone prohibition have dedicated PHPTs | Implemented and tested |
| Public error behavior | PHPTs cover invalid shapes/coordinates, incompatible multiplication, singular inverse, invalid types, rank/dimension failures, and non-finite least-squares inputs | Broad core coverage; continue pinning down new public methods as added |
| PHP 8 named-argument metadata | leastSquares(rightHandSide: $response) is exercised by tests/010-least-squares-named-argument.phpt, including Reflection parameter names | Implemented; PR #197 |
| Supported Linux CI | Tests workflow covers PHP 8.2–8.5 and debug build; native coverage is a separate workflow | Verify green status on the release candidate commit |
| Windows | `config.w32` exists, but Windows build/test CI is absent | Unverified; do not claim Windows support is validated |

## Remaining limitations and decisions

1. `leastSquares()` deliberately requires full column rank. Rank-deficient and underdetermined minimum-norm solutions are not silently selected. If the PHP API later exposes a pseudoinverse or SVD composition, it needs its own public contract and PHPTs.
2. The current façade has no mutable element assignment or general elementwise API. Neither is needed for the current OLS example; do not expand the API without a demonstrated composition gap.
3. The PHP exception mapping distinguishes invalid arguments/shapes (`ValueError`) from numerical solver failures (`Exception`). If more precise numerical exception types are introduced, treat that as a deliberate public-API compatibility decision.
4. Windows portability remains unverified.

## Current CI evidence and release decision

The feature PRs #194–#197 passed their pull-request CI, including the PHP 8.2–8.5 matrix, debug build, and native coverage. PR #197's final head passed both required workflows before merge. Post-merge main runs must still be checked on the resulting commit before treating the release-candidate gate as satisfied.

**Do not infer release readiness from native C tests alone or from feature-PR CI alone.** The 1.0 gate can be considered satisfied only after the supported PHP 8.2–8.5 matrix, debug build, native coverage workflow, executable example, and all PHPTs pass on the intended release-candidate commit. Windows remains explicitly unverified until a Windows build and test job is added and passes.
