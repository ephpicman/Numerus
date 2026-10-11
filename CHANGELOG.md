# Changelog

Notable user-facing changes are recorded here. This project follows Semantic Versioning for its public PHP API.

## 1.0.0 — Release candidate

### Added
- Public `Numerus\\Matrix` PHP API for constructing and inspecting matrices.
- Matrix transpose, multiplication, inverse, and pivoted-QR least-squares operations.
- A runnable ordinary least squares (OLS) example using the recommended least-squares path.
- PHP-visible contracts and tests for dimensions, invalid inputs, object lifecycle, named arguments, and rank-deficient or underdetermined least-squares inputs.
- User and developer documentation covering installation, API behavior, architecture, testing, contribution, and release engineering.

### Numerical and compatibility notes
- `leastSquares()` requires a full-column-rank design matrix. Rank-deficient and underdetermined inputs are rejected; a minimum-norm solution is not exposed by the initial PHP API.
- Explicit normal equations and matrix inversion are not the recommended least-squares path.
- Linux CI covers PHP 8.2–8.5, including debug builds, PHPTs, native tests, and native coverage.
- Windows build configuration exists, but Windows support remains unverified because Windows build/test CI is not yet present.
- Weighted least squares (WLS), generalized least squares (GLS), model inference, and MLE-enabling PHP workflows are outside the 1.0 scope and remain roadmap work.

### Release status
This entry is prepared for the 1.0.0 release. Replace the release-candidate heading with the publication date when the final release is cut. Do not publish until the exact release commit passes CI and the clean source-archive build/test gate.
