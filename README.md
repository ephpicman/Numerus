# Numerus

[![Tests](https://github.com/ephpicman/Numerus/actions/workflows/tests.yml/badge.svg?branch=main)](https://github.com/ephpicman/Numerus/actions/workflows/tests.yml)
[![Native coverage](https://github.com/ephpicman/Numerus/actions/workflows/coverage.yml/badge.svg?branch=main)](https://github.com/ephpicman/Numerus/actions/workflows/coverage.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Numerus is a native PHP extension that provides reusable numerical-computing primitives. Its current PHP-facing API focuses on matrix operations that can be composed into workflows such as ordinary least squares (OLS).

The project intentionally exposes general-purpose primitives rather than model-specific functions such as `fit_ols()`, `fit_wls()`, `fit_gls()`, or `fit_mle()`.

## Current status

The initial PHP API and executable OLS workflow are implemented. The 1.0 release gate is tracked in the [release roadmap](docs/release-roadmap.md) and [PHP OLS release-gate audit](docs/php-api-release-audit.md). Linux CI currently covers PHP 8.2–8.5, a debug build, PHPTs, native tests, and native line/branch coverage. Windows build configuration exists, but Windows support remains unverified because Windows build/test CI is not yet present.

## Quick start

Requirements:

- PHP 8.2 or later, with matching development headers and `phpize`
- Autoconf
- A C compiler and build toolchain

Build the extension on Unix-like systems:

```sh
phpize
./configure --enable-numerus
make
```

Load it for a command and run the checked OLS example:

```sh
php -d extension="$(pwd)/modules/numerus.so" examples/ols.php
```

Expected output:

```text
intercept: 1.000000
slope: 2.000000
```

The shared-library suffix and module path depend on the platform and PHP build. For persistent use, enable the extension in the appropriate `php.ini` or additional INI file.

## PHP API at a glance

```php
<?php

use Numerus\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);

$response = Matrix::fromRows([
    [3.0],
    [5.0],
    [7.0],
]);

// Recommended: pivoted-QR least-squares solve.
$beta = $design->leastSquares($response);

printf("intercept: %.6f\n", $beta->get(0, 0));
printf("slope: %.6f\n", $beta->get(1, 0));
```

The PHP-facing API currently includes:

- `Matrix::fromRows(array $rows)` and `Matrix::zeros(int $rows, int $columns)`
- `rows()`, `columns()`, and zero-based `get($row, $column)`
- `transpose()`, `multiply(Matrix $other)`, and `inverse()`
- `leastSquares(Matrix $rightHandSide)`, using pivoted QR
- `weightedLeastSquares(Matrix $rightHandSide, array $weights)`, using square-root row scaling and pivoted QR

The least-squares operation supports one or more right-hand-side columns and requires a full-column-rank design matrix. Rank-deficient and underdetermined inputs are rejected explicitly; the initial PHP API does not expose a minimum-norm solution. Prefer `leastSquares()` to explicitly forming normal equations: forming (X^T X) squares the condition number, and explicitly forming an inverse can amplify rounding error.

See the [PHP API reference](docs/php-api.md) for exact contracts and exceptions, the [user guide](docs/user-guide.md) for examples and limitations, and [examples/ols.php](examples/ols.php) / [examples/wls.php](examples/wls.php) for executable OLS and WLS workflows.

## Architecture

Numerus separates opaque native storage, matrix operations, and reusable statistical primitives.

### Storage

Storage exposes one logical matrix interface over specialised representations:

- dense rectangular
- upper and lower triangular
- diagonal
- identity
- constant
- zero
- scaled identity
- sparse (default value plus explicit overrides)
- symmetric
- banded

Storage is opaque and immutable. Its representation is private to `src/storage/numerus_storage.c`, and constructor inputs are copied into owned memory.

### Matrix

The internal Matrix abstraction supports storage-backed roots and lazy coordinate views, including transpose, flips, rotations, row/column removal and swaps, joins, and callback-based views. Removal and swap operations are views, not mutations. Coordinates are zero-based; removing the only row or column is rejected because matrix dimensions must remain positive.

Internal C view-parent references are non-owning. The PHP façade materializes returned results so PHP Matrix objects own independent native storage and do not retain C view-parent references. General transform callbacks and their context are borrowed; the context must remain valid and logically unchanged for the child view's lifetime, and callbacks must be deterministic.

See [Storage documentation](docs/storage.md) and [Matrix documentation](docs/matrix.md) for the native contracts and implementation details.

### Internal statistical foundation

The source tree also contains reusable C-level primitives for triangular solves and log determinants, scalar/log-domain functions, explicit-state PCG32 random generation and index sampling, BFGS optimization, and multivariate Gaussian log density/sampling. These are internal composable capabilities, not PHP-facing model-fitting APIs. Do not assume that all internal primitives are available to PHP callers.

## Testing

Run PHPTs from a configured extension build:

```sh
make test TESTS='tests/*.phpt'
```

The native Storage and Matrix APIs also have standalone C tests. From the repository root:

```sh
cc -Isrc/core -Isrc/storage -Isrc/matrix -Isrc/statistics -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/storage_test tests/storage_test.c src/storage/numerus_storage.c
./tests/storage_test

cc -Isrc/core -Isrc/storage -Isrc/matrix -Isrc/statistics -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_test tests/matrix_test.c src/matrix/numerus_matrix.c src/matrix/numerus_matrix_view_nodes.c src/matrix/numerus_matrix_constructors.c src/storage/numerus_storage.c
./tests/matrix_test
```

The allocator defines are for standalone test executables only; the extension build uses the Zend Memory Manager. CI runs native tests, PHPTs, extension-loading checks, and a debug-oriented build. The separate native coverage workflow produces line/branch coverage reports; see the [native test coverage guide](docs/native-test-coverage.md).

## Project structure

- `config.m4` — Unix Autoconf configuration.
- `config.w32` — Windows build configuration (not yet validated in CI).
- `php_numerus.h` — module declarations and version.
- `src/core/` — checked size arithmetic and scalar numerical helpers.
- `src/storage/` — immutable storage representations.
- `src/matrix/` — Matrix core, lazy view nodes, factories, operations, and focused declaration headers; `numerus_matrix.h` remains the compatibility umbrella.
- `src/statistics/` — RNG, sampling, probability, and optimizer primitives.
- Root-level C headers are forwarding includes retained for existing native test include paths; declarations and implementations live under `src/`.
- `examples/ols.php` — runnable PHP OLS coefficient workflow.
- `tests/` — PHPT and native C tests.
- `.github/workflows/` — build/test matrix and native coverage workflows.

## Design principles

1. Keep low-level numerical primitives small and composable.
2. Hide representation details behind opaque C APIs.
3. Prefer specialised storage when it materially reduces memory or computation.
4. Keep Storage immutable so higher-level Matrix objects can share it safely.
5. Validate at API boundaries; use unchecked accessors only after establishing their preconditions.
6. Make ownership, overflow, allocation failure, and bounds behaviour explicit.
7. Add higher-level algorithms and PHP-facing APIs only when their semantics and lifecycle requirements are well understood.

## Documentation

Start at the [documentation index](docs/README.md):

- [User guide](docs/user-guide.md) — installation, current PHP API, OLS example, and limitations.
- [Developer guide](docs/developers/README.md) — architecture, testing, contribution workflow, and release engineering.
- [PHP API reference](docs/php-api.md) — exact public method contracts.
- [Release roadmap](docs/release-roadmap.md) — user-facing milestones and acceptance criteria.
- [Release-gate audit](docs/php-api-release-audit.md) — evidence and remaining limitations for 1.0.

The user guide documents only capabilities exposed through PHP. Internal C primitives and planned roadmap items are labelled separately and should not be mistaken for shipped PHP APIs.

## Contributing and community standards

Contributions are welcome. Start with the [contribution guide](CONTRIBUTING.md) and follow the project's [Code of Conduct](CODE_OF_CONDUCT.md). Security issues should be reported according to [SECURITY.md](SECURITY.md), not filed as public bug reports.

## License

Numerus is distributed under the [MIT License](LICENSE).
