# Changelog

Notable user-facing changes are recorded here. This project follows Semantic Versioning for its public PHP API.

## 1.1.0 — Unreleased

### Added
- Public `Matrix::weightedLeastSquares(Matrix $rightHandSide, array $weights): Matrix` API for weighted least-squares solving.
- WLS uses square-root row scaling and the existing pivoted-QR least-squares solver; it does not form weighted normal equations.
- Support for multiple right-hand-side columns and zero-weight observations.
- PHP and native regression tests for equal/unequal/zero weights, invalid weights, dimension mismatches, non-finite values, and solver failure cases.
- A runnable, checked WLS example and updated user/API documentation.

### Numerical and compatibility notes
- Weights must be numeric, finite, non-negative, and supplied one per observation. A zero weight removes that observation's contribution to the objective, but input matrix values are still validated by the solver.
- The square-root-scaled design matrix must have full column rank. Rank-deficient and underdetermined weighted designs are rejected; minimum-norm solutions are not exposed through the PHP API.
- WLS materializes scaled design and response matrices, using additional memory proportional to their sizes.
- OLS remains available through `leastSquares()`; neither `fit_wls()` nor other model-specific estimator APIs are introduced.
- Linux CI targets PHP 8.2–8.5. Windows build configuration exists, but Windows support remains unverified because Windows build/test CI is not yet present.

## 1.0.0

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
