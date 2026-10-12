# PHP 1.0 OLS Release-Gate Audit

This audit evaluates the user-facing 1.0 target in [the release roadmap](release-roadmap.md), not every item in the broader statistical-foundation inventory.

## Acceptance evidence

| Gate | Evidence | Assessment |
|---|---|---|
| Construct design and response matrices from PHP | `Numerus\\Matrix::fromRows()`, `zeros()`; PHPT construction/access tests | Implemented |
| Compose the explicit normal-equation identity | `tests/004-ols-composition.phpt` checks `(XᵀX)⁻¹Xᵀy` | Implemented; documented as a non-default numerical path |
| Recommended stable least-squares path | `Matrix::leastSquares()` uses the pivoted-QR solver; `examples/ols.php` is an executable example and `tests/008-ols-example.phpt` invokes it | Implemented for full-column-rank designs |
| Numerical property/reference coverage | Native least-squares tests cover multiple RHS, noisy overdetermined residuals, rank/shape failures, and non-finite inputs; PHP-visible contract tests cover the boundary | Implemented; latest main CI is green |
| Rank-deficient and underdetermined behavior | The QR least-squares API reports rank-deficient status; the PHP façade maps solver failures to `Exception`. Minimum-norm solutions are not exposed in PHP | Explicit rejection is tested; consistent with the 1.0 contract |
| PHP ownership and lifecycle | Returned operations own independent native matrices; transpose lifetime and clone prohibition have dedicated PHPTs | Implemented and tested |
| Public error behavior | PHPTs cover invalid shapes/coordinates, incompatible multiplication, singular inverse, invalid types, rank/dimension failures, and non-finite least-squares inputs | Sufficient for the current public surface; extend with new methods |
| PHP 8 named-argument metadata | `leastSquares(rightHandSide: $response)` is exercised by `tests/010-least-squares-named-argument.phpt`, including Reflection parameter names | Implemented |
| Supported Linux CI | PHP 8.2–8.5, debug build, PHPTs, native tests, and native coverage | Passed on post-merge main commit `648ea9be77f538da85ec21df2d16973d5c5b35d5`; see [Tests run](https://github.com/ephpicman/Numerus/actions/runs/38096728970) and [Native coverage run](https://github.com/ephpicman/Numerus/actions/runs/38096728974) |
| README and PHP documentation | README quick start, API summary, example, platform caveat, and documentation links agree with the PHP API/user guide | Reviewed and aligned in this change |
| Windows | `config.w32` exists, but Windows build/test CI is absent | Unverified; README and user guide must not claim validated Windows support |

## Remaining limitations and decisions

1. `leastSquares()` deliberately requires full column rank. Rank-deficient and underdetermined minimum-norm solutions are not silently selected. A future PHP pseudoinverse/SVD surface needs its own public contract and PHPTs.
2. The current façade has no mutable element assignment or general elementwise API. Neither is needed for the current OLS example; do not expand the API without a demonstrated composition gap.
3. PHP errors distinguish invalid arguments/shapes (`ValueError`) from numerical solver failures (`Exception`). More precise numerical exception types would be a deliberate public-API compatibility decision.
4. Windows portability remains unverified and is not represented as supported-and-tested.
5. This audit does not claim a release has been published. It records whether the current source meets the agreed 1.0 acceptance criteria.

## Current CI evidence and release decision

The post-merge main commit `648ea9be77f538da85ec21df2d16973d5c5b35d5` passed the supported Linux release gate after PR #200 was merged:

- PHP 8.2, 8.3, 8.4, and 8.5 test jobs
- Debug build
- PHPTs and extension-loading checks
- Native Storage/Matrix/statistical tests, including sanitizer-oriented jobs
- Native line/branch coverage workflow
- The executable OLS example, exercised by PHPT

See [Tests workflow run](https://github.com/ephpicman/Numerus/actions/runs/38096728970) and [Native coverage workflow run](https://github.com/ephpicman/Numerus/actions/runs/38096728974).

**Assessment: the documented Linux 1.0 OLS-enabling acceptance gate is satisfied for commit `648ea9be77f538da85ec21df2d16973d5c5b35d5`.** This does not mean a version has been tagged or published. Windows remains unverified and must stay clearly labelled as such. Before publishing, follow the [release engineering guide](developers/releases.md), review the final release diff, and run the gate on the exact commit intended for release.
