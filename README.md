# Numerus

Numerus is a native PHP extension for numerical-computing infrastructure.

The project is being built as a low-level foundation for numerical work in PHP,
including matrix computation, numerical methods, optimisation, and statistical
models. The current focus is on internal primitives rather than a broad
application-level API.

## Current status

The codebase contains the extension skeleton, an immutable **Storage** subsystem,
and an internal **Matrix** abstraction built on top of Storage. Matrix is
currently a C-level internal API, not a PHP-facing object API.

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

Storage is opaque and immutable. Its representation is private to
`numerus_storage.c`, and constructor inputs are copied into owned memory.

### Matrix

Matrix can be created as a storage-backed root or as a lazy view. Supported
views include transpose, row/column flips, 90-degree and 180-degree rotations,
and horizontal/vertical joins. Views do not copy element data: they map a read
to their parent Matrix (or, for joins, to one of two parents).

Parent references are **non-owning**. A caller must keep every parent alive
while a child or joined Matrix can be read. The eventual PHP object layer must
enforce this lifetime relationship.

General transform callbacks and their context are borrowed. The context must
remain valid and logically unchanged for the child's lifetime, and callbacks
must be deterministic and must not depend on mutable external state. Declaring
the context `const void *` prevents mutation through that pointer; it cannot
prevent mutation through another alias.

See [Storage documentation](docs/storage.md) and
[Matrix documentation](docs/matrix.md) for API contracts and design details.

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

## Tests

PHPT is the test mechanism for PHP-visible behaviour:

```sh
make test TESTS='tests/*.phpt'
```

The internal Storage and Matrix APIs also have standalone native C tests. Run
these from the repository root:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/storage_test tests/storage_test.c numerus_storage.c
./tests/storage_test

cc -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_test tests/matrix_test.c numerus_matrix.c numerus_storage.c
./tests/matrix_test
```

The allocator defines are for standalone test executables only. The extension
build uses the Zend Memory Manager. CI runs both native test binaries, PHPTs,
extension loading checks, and a separate debug-oriented build.

## Project structure

- `config.m4` — Unix Autoconf configuration.
- `config.w32` — Windows build configuration.
- `php_numerus.h` — module declarations and version.
- `numerus.c` — module implementation.
- `numerus_storage.h` / `numerus_storage.c` — opaque Storage API and implementation.
- `numerus_matrix.h` / `numerus_matrix.c` — internal Matrix API and implementation.
- `tests/storage_test.c` — native Storage tests.
- `tests/matrix_test.c` — native Matrix tests.
- `tests/` — PHPT and native C tests.
- `docs/storage.md` — Storage design and API documentation.
- `docs/matrix.md` — Matrix views, joins, ownership, and API documentation.
- `.github/workflows/tests.yml` — build and test matrix.

## Design principles

1. Keep low-level numerical primitives small and composable.
2. Hide representation details behind opaque C APIs.
3. Prefer specialised storage when it materially reduces memory or computation.
4. Keep Storage immutable so higher-level Matrix objects can share it safely.
5. Validate at API boundaries; use unchecked accessors only after establishing
   their preconditions.
6. Make ownership, overflow, allocation failure, and bounds behaviour explicit.
7. Add higher-level algorithms and PHP-facing APIs only when their semantics
   and lifecycle requirements are well understood.

## License

MIT
