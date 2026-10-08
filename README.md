# Numerus

Numerus is a native PHP extension for numerical computing infrastructure.

The project is being built as a low-level foundation for higher-level numerical
work in PHP, including matrix computation, numerical methods, optimisation,
statistical estimation, and related mathematical models. The current codebase
is intentionally focused on the internal primitives rather than exposing a
large application-level API.

## Current status

The project currently contains the extension skeleton, an internal immutable
**Storage** subsystem, and the first internal **Matrix** abstraction built on top of it.

Storage provides a common logical matrix interface over specialised
representations:

- dense rectangular
- upper triangular
- lower triangular
- diagonal
- identity
- constant
- zero
- scaled identity
- sparse with a default value and explicit overrides
- symmetric
- banded

Storage is deliberately opaque. Its representation is private to
`numerus_storage.c`, so higher-level components depend on the Storage API
rather than its memory layout.

Matrix supports storage-backed roots, lazy derived views (transpose, flips, and
rotations), and lazy joined views. Horizontal joins append a second Matrix to
the right; vertical joins append it below. Joins retain two non-owning parent
references and read from the appropriate parent on demand, without copying
element data. Join dimensions are validated and size addition is checked for
overflow.

All successful element reads return a `double`. Storage does not expose
mutation; constructor inputs are copied into owned memory.

See [Storage documentation](docs/storage.md) for the representation model,
semantics, API, and implementation notes. Matrix currently remains an internal C
abstraction; its PHP-facing construction API will be added after the internal
lifecycle and semantics are stabilised.

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

PHPT is the primary test mechanism for PHP-visible behaviour:

```sh
make test TESTS='tests/*.phpt'
```

The repository also contains native C tests for the internal Storage subsystem:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/storage_test tests/storage_test.c numerus_storage.c
./tests/storage_test
```

The native test build uses the libc allocator only because it runs as a
standalone executable. It currently covers both Storage and Matrix. The extension build itself uses Zend Memory Manager
(`emalloc`, `ecalloc`, and `efree`).

## CI

GitHub Actions verifies:

- PHP 8.2
- PHP 8.3
- PHP 8.4
- PHP 8.5
- extension compilation
- native Storage tests
- PHPT tests
- extension loading
- a separate debug-oriented build

## Project structure

- `config.m4` — Unix Autoconf configuration.
- `config.w32` — Windows build configuration.
- `php_numerus.h` — module declarations and version.
- `numerus.c` — module implementation.
- `numerus_storage.h` — opaque Storage API and public contracts.
- `numerus_storage.c` — Storage representations and access logic.
- `numerus_matrix.h` — internal Matrix API and construction contracts.
- `numerus_matrix.c` — Matrix construction, parent delegation, and access logic.
- `tests/matrix_test.c` — native Matrix tests.
- `tests/` — PHPT and native C tests.
- `docs/storage.md` — Storage design and API documentation.
- `.github/workflows/tests.yml` — build and test matrix.

## Design principles

Numerus is intentionally being built from the bottom up:

1. Keep low-level numerical primitives small and composable.
2. Keep representation details private behind stable C APIs.
3. Prefer specialised storage when it materially reduces memory or computation.
4. Keep Storage immutable so higher-level Matrix objects can safely share it.
5. Validate at public boundaries and provide unchecked hot paths only where the
   caller has already established the required invariants.
6. Make overflow, allocation failure, and bounds behaviour explicit.

Higher-level matrix algorithms and PHP-facing APIs will be introduced only when
the underlying numerical semantics justify them.

## License

MIT
