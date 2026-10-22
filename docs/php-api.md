# PHP Matrix API

The PHP-facing API is intentionally small. It exposes general matrix operations rather than model-specific statistical estimators.

## Constructing matrices

```php
use Numerus\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);
$response = Matrix::fromRows([[3.0], [5.0], [7.0]]);
```

`fromRows()` accepts a non-empty array of non-empty, equally sized row arrays. Every element must be a PHP integer or float. `zeros($rows, $columns)` creates a matrix with positive dimensions.

Rows and columns are counted from one for dimensions; coordinates passed to `get($row, $column)` are zero-based.

## End-to-end OLS example

After building the extension, run the repository's [OLS example](../examples/ols.php):

```sh
php -d extension="$(pwd)/modules/numerus.so" examples/ols.php
```

The example estimates the intercept and slope for a small deterministic data set and checks the expected coefficients before printing them. It is intended as an executable API smoke test and a starting point for userland composition.

## End-to-end WLS example

Run the weighted least-squares example the same way:

```sh
php -d extension="$(pwd)/modules/numerus.so" examples/wls.php
```

Like the OLS example, this is an executable API smoke test: it uses a deterministic data set, applies one non-negative weight per observation, checks the expected intercept and slope, and prints the coefficients. It fails with a `RuntimeException` if the result differs from the fixture beyond the stated tolerance.

## Composing OLS

Numerus supports both a direct least-squares solve and the explicit normal-equation composition.

```php
// Recommended default: pivoted-QR least squares.
$beta = $design->leastSquares($response);

// Also supported: explicit (XᵀX)⁻¹Xᵀy composition.
$xt = $design->transpose();
$betaFromNormalEquations = $xt
    ->multiply($design)
    ->inverse()
    ->multiply($xt->multiply($response));
```

The explicit formula is useful for teaching and for users who deliberately need that composition. It is not the numerical default: forming normal equations squares the condition number, and explicitly forming an inverse can amplify rounding error. The QR-based least-squares operation accepts one or more right-hand-side columns and requires the design matrix to have full column rank. It rejects rank-deficient and underdetermined designs with a generic `Exception`; a right-hand-side row-count mismatch raises `ValueError`. Non-finite values encountered by the QR/least-squares computation raise a generic `Exception`. For minimum-norm solutions to rank-deficient or underdetermined systems, the C API provides a pseudoinverse primitive, but this initial PHP façade does not expose it.

## Composing weighted least squares (WLS)

Use the design matrix as the receiver and pass the response matrix plus one weight per observation:

```php
$beta = $design->weightedLeastSquares($response, [1.0, 0.5, 2.0]);
```

Weights must be finite, non-negative PHP integers or floats, and their count must equal the number of rows in both the design and response matrices. Numeric strings are not accepted. A zero weight excludes that observation from the objective, but values in that row are still validated. The weighted design (after square-root row scaling) must have full column rank; otherwise the operation raises an exception. The method returns an independently owned Matrix and supports multiple response columns. PHP named arguments are `rightHandSide` and `weights`; both names are part of the public API contract.

The current native implementation applies square-root row scaling and delegates to the existing pivoted-QR least-squares solver. It materializes the scaled design and response matrices, so WLS currently uses additional memory proportional to the input matrices. No model-specific `fit_wls()` function is provided. See the runnable [WLS example](../examples/wls.php).

## Public methods

- `Matrix::fromRows(array $rows): Matrix`
- `Matrix::zeros(int $rows, int $columns): Matrix`
- `$matrix->rows(): int`
- `$matrix->columns(): int`
- `$matrix->get(int $row, int $column): float`
- `$matrix->transpose(): Matrix`
- `$matrix->multiply(Matrix $other): Matrix`
- `$matrix->inverse(): Matrix`
- `$matrix->leastSquares(Matrix $rightHandSide): Matrix`
- `$matrix->weightedLeastSquares(Matrix $rightHandSide, array $weights): Matrix`

All returned matrices own independent native storage. In particular, `transpose()` materializes its result, so returned objects do not depend on the lifetime of their source object. Cloning is deliberately unsupported because the native object owns its storage.

Invalid shapes, coordinates, or dimensions raise `ValueError`. Invalid PHP argument types raise `TypeError`. Singular or rank-deficient operations raise an exception. Other numerical failures are also reported as exceptions rather than C status integers. This initial API does not expose mutable elements, lazy views, raw native pointers, or factorization internals.
