# PHP Matrix API

The initial PHP-facing API is intentionally small. It exposes general matrix operations rather than statistical estimators.

## Constructing matrices

```php
use Numerus\\Matrix;

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

The explicit formula is useful for teaching and for users who deliberately need that composition. It is not the numerical default: forming normal equations squares the condition number, and explicitly forming an inverse can amplify rounding error. The QR-based least-squares operation requires full column rank and reports rank deficiency.

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

All returned matrices own independent native storage. In particular, `transpose()` materializes its result, so returned objects do not depend on the lifetime of their source object. Cloning is deliberately unsupported because the native object owns its storage.

Invalid shapes, coordinates, or dimensions raise `ValueError`. Invalid PHP argument types raise `TypeError`. Singular or rank-deficient operations raise an exception. Other numerical failures are also reported as exceptions rather than C status integers. This initial API does not expose mutable elements, lazy views, raw native pointers, or factorization internals.
