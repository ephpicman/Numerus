# PHP Extension Template

A minimal, production-oriented starting point for building PHP extensions in C.

The repository is intentionally small. It provides the build system, PHPT test infrastructure, cross-platform build metadata, and GitHub Actions CI that should be common to a PHP extension project.

## Requirements

- PHP 8.2 or later
- Autoconf
- A C compiler and build toolchain

## Build

On Unix-like systems:

```sh
phpize
./configure --enable-numerus
make
```

Load the built extension:

```sh
php -d extension="$(pwd)/modules/numerus.so" -m
```

On Windows, use the standard PHP build tools and `config.w32`.

## Test

PHPT is the primary test mechanism:

```sh
make test TESTS='tests/*.phpt'
```

The test suite is deliberately independent of PHPUnit. PHP extensions should normally test their public PHP-visible behaviour through PHPT.

## CI

GitHub Actions currently verifies:

- PHP 8.2
- PHP 8.3
- PHP 8.4
- PHP 8.5
- extension compilation
- PHPT tests
- extension loading
- a separate debug-oriented build

The workflow is designed to be extended as an individual extension acquires additional platform or runtime requirements.

## Project structure

- `config.m4` — Unix Autoconf configuration.
- `config.w32` — Windows build configuration.
- `php_numerus.h` — module declarations and version.
- `numerus.c` — minimal module implementation.
- `tests/` — PHPT tests.
- `.github/workflows/tests.yml` — build and test matrix.
- `.gitignore` — generated build artefacts.
- `.editorconfig` — basic repository conventions.

## Using this as a template

When creating a new extension from this repository:

1. Rename the repository.
2. Rename the extension identifiers from `numerus` to the new extension name.
3. Update `config.m4` and `config.w32`.
4. Update the module declaration and version.
5. Replace the example PHPT tests.
6. Keep the CI structure unless the extension has additional requirements.

The template deliberately does not prescribe an application architecture. The internal C structure should be chosen according to the extension's actual domain and performance requirements.

## License

MIT
