# Numerus User Guide

Numerus is a native PHP extension for numerical-computing infrastructure. Its public PHP API currently focuses on matrix construction and operations that users can compose into workflows such as ordinary least squares (OLS).

**Important:** Numerus does not currently expose ready-made statistical model methods such as fit_ols(), fit_wls(), fit_gls(), or fit_mle(). Users compose general-purpose operations themselves.

## Requirements

- PHP 8.2 or later
- PHP development tools, including phpize and matching PHP headers
- Autoconf and a C compiler/build toolchain

Windows build configuration exists, but Windows builds are not currently covered by the CI matrix. Treat Windows support as unverified.

## Build from source

From the repository root on a Unix-like system:

~~~sh
phpize
./configure --enable-numerus
make
~~~

Load the built extension for a command:

~~~sh
php -d extension="$(pwd)/modules/numerus.so" -m
~~~

The module path and shared-library suffix depend on the platform and PHP build. To enable the extension persistently, configure the appropriate php.ini or additional INI file.

## Your first matrix

~~~php
<?php

use Numerus\Matrix;

$matrix = Matrix::fromRows([
    [1.0, 2.0],
    [3.0, 4.0],
]);

printf("%d rows, %d columns\n", $matrix->rows(), $matrix->columns());
printf("Element (0, 1): %.1f\n", $matrix->get(0, 1));
~~~

Dimensions are positive counts. Element coordinates are **zero-based**, so get(0, 1) reads the first row and second column.

## Public PHP API

| API | Description |
|---|---|
| Matrix::fromRows(array $rows) | Construct from a non-empty rectangular array of PHP integers/floats |
| Matrix::zeros(int $rows, int $columns) | Construct a zero-filled matrix with positive dimensions |
| $matrix->rows() | Get the row count |
| $matrix->columns() | Get the column count |
| $matrix->get(int $row, int $column) | Read an element using zero-based coordinates |
| $matrix->transpose() | Return a transposed matrix |
| $matrix->multiply(Matrix $other) | Matrix multiplication |
| $matrix->inverse() | Invert an invertible square matrix |
| $matrix->leastSquares(Matrix $rightHandSide) | Solve least squares using pivoted QR |

See the [PHP API reference](php-api.md) for exact signatures and error behavior.

Returned matrices own independent native storage. The PHP API does not expose mutable element assignment, lazy views, raw pointers, or factorization internals. Cloning a Matrix object is unsupported.

## Example: ordinary least squares

The design matrix X contains the intercept column and predictor values; y is the response vector. The coefficient vector beta is estimated as follows:

~~~php
<?php

use Numerus\Matrix;

$X = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);

$y = Matrix::fromRows([
    [3.0],
    [5.0],
    [7.0],
]);

// Recommended: solve least squares using pivoted QR.
$beta = $X->leastSquares($y);

printf("Intercept: %.6f\n", $beta->get(0, 0));
printf("Slope: %.6f\n", $beta->get(1, 0));
~~~

Expected output:

~~~text
Intercept: 1.000000
Slope: 2.000000
~~~

Run the repository's checked example after building the extension:

~~~sh
php -d extension="$(pwd)/modules/numerus.so" examples/ols.php
~~~

### Normal equations versus least squares

The explicit normal-equation expression (XᵀX)⁻¹Xᵀy can be composed from transpose, multiplication, and inverse. It is useful for teaching, but it is **not the recommended numerical default**: forming normal equations squares the condition number, and explicitly forming an inverse can amplify rounding errors. Prefer $X->leastSquares($y).

The least-squares API accepts one or more right-hand-side columns and requires the design matrix to have full column rank. Rank-deficient and underdetermined designs are rejected. A minimum-norm solution for such systems is not available through this initial PHP façade.

## Errors and input constraints

- fromRows() requires non-empty, equally sized row arrays. Elements must be PHP integers or floats.
- Matrix dimensions must be positive.
- Invalid dimensions, shapes, or coordinates raise ValueError.
- Invalid PHP argument types raise TypeError.
- Least-squares row-count mismatch raises ValueError; rank-deficient or underdetermined inputs and non-finite values encountered by the computation raise an exception.
- The current API does not promise a specialised exception class for numerical failures.

## Current scope: what is and is not exposed

### Available through PHP

- Matrix construction from rows and zero-filled construction
- Dimension and element access
- Transpose, multiplication, inverse
- Pivoted-QR least-squares solving
- Composition of an OLS coefficient workflow

### Not currently exposed as PHP-level APIs

The repository contains additional internal C capabilities, but they are not part of the current PHP-facing API. Do not assume PHP callers can use ready-made OLS, WLS, GLS, or MLE estimators; a PHP-facing pseudoinverse; a complete statistical inference API; or all internal matrix views, decompositions, probability, RNG, and optimizer primitives.

Consult the [release roadmap](release-roadmap.md) for intended milestones, not as a list of shipped PHP features.

## Running tests

From a configured extension build:

~~~sh
make test TESTS='tests/*.phpt'
~~~

Native C tests and debug/coverage workflows are documented in the [developer guide](developers/README.md).

## License

Numerus is distributed under the MIT License. See the repository's LICENSE file.
