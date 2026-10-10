# Numerus

Numerus is a native PHP extension for numerical-computing infrastructure.

The project is being built as a low-level foundation for numerical work in PHP,
including matrix computation, numerical methods, optimisation, and statistical
models. The current focus is on internal primitives rather than a broad
application-level API.

## Current status

The user-facing release milestones and the 1.0 acceptance gate are defined in the [release roadmap](docs/release-roadmap.md). The broader [v1 statistical foundation roadmap](docs/v1-statistical-foundation-roadmap.md) tracks reusable capabilities and gaps; its P0 labels do not mean every listed capability is required for 1.0.

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
`src/storage/numerus_storage.c`, and constructor inputs are copied into owned memory.

### Matrix

Matrix can be created as a storage-backed root or as a lazy view.
Supported coordinate views include transpose, row/column flips, 90-degree and
180-degree rotations, removing one row or column, and swapping two selected
rows or columns. Horizontal and vertical joins combine two parent Matrices.
Views do not copy element data: they map reads to their parent Matrix (or, for
joins, to one of two parents). The Matrix API also supports generic coordinate
and value callbacks for custom views.

Removal and swap operations are views, not mutations. Coordinates are
zero-based. Removing the only row or only column is rejected because Matrix
dimensions must remain positive; invalid indices return an error status.

Parent references are **non-owning**. A caller must keep every parent alive
while a child or joined Matrix can be read. The eventual PHP object layer must
enforce this lifetime relationship.

General transform callbacks and their context are borrowed. The context must
remain valid and logically unchanged for the child's lifetime, and callbacks
must be deterministic and must not depend on mutable external state. Declaring
the context `const void *` prevents mutation through that pointer; it cannot
prevent mutation through another alias.

See [Storage documentation](docs/storage.md) and
[Matrix documentation](docs/matrix.md) for API contracts, design details, and
cached Matrix analysis.

### Statistical foundation (internal C API)

The source tree also contains reusable statistical primitives: triangular solves
and stable log determinants for GLS composition, stable scalar/log-domain
functions, explicit-state PCG32 random generation and index sampling, generic
unconstrained BFGS optimization, and multivariate Gaussian log density/sampling.
These are composable C APIs, not model-fitting functions; the project does not
provide `fit_ols()`, `fit_gls()`, `fit_mle()`, GP fitting, or MCMC.

**These primitives are not currently exposed to PHP userland.** A minimal,
ownership-safe PHP façade remains a separate prerequisite before Numerus can be
described as usable by PHP statistical packages. See the
[statistical foundation overview](docs/statistical-foundation.md) for the
current capability inventory, contracts, executable example, and explicit gaps.
The final P0 evidence review is in the [v1 acceptance audit](docs/v1-statistical-foundation-final-audit.md); it records that overall userland v1 remains blocked until a PHP façade is implemented and tested.

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

On Windows, `config.w32` is provided, but Windows builds are not currently covered by the CI matrix; treat Windows support as unverified until a Windows build and test job passes.

## Tests

PHPT is the test mechanism for PHP-visible behaviour:

```sh
make test TESTS='tests/*.phpt'
```

The internal Storage and Matrix APIs also have standalone native C tests. Run
these from the repository root:

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

The allocator defines are for standalone test executables only. The extension
build uses the Zend Memory Manager. CI runs both native test binaries, PHPTs,
extension loading checks, and a separate debug-oriented build.

CI also generates a line/branch coverage report for native source files. See the
[native test coverage guide](docs/native-test-coverage.md) for report artifacts
and guidance on interpreting uncovered code.

## Project structure

- `config.m4` — Unix Autoconf configuration.
- `config.w32` — Windows build configuration.
- `php_numerus.h` — module declarations and version.
- `src/core/` — checked size arithmetic and scalar numerical helpers.
- `src/storage/` — immutable storage representations.
- `src/matrix/` — Matrix core, lazy view nodes, factories, operations, and focused declaration headers; `numerus_matrix.h` remains the compatibility umbrella.
- `src/statistics/` — RNG, sampling, probability, and optimizer primitives.
- Root-level C headers are forwarding includes retained for existing native test include paths; declarations and implementations live under `src/`.
- `php_numerus.h`, `config.m4`, and `config.w32` remain extension-facing build entry points at the repository root.
- `tests/storage_test.c` — native Storage tests.
- `tests/matrix_test.c` — native Matrix tests.
- `tests/` — PHPT and native C tests.
- `docs/storage.md` — Storage design and API documentation.
- `docs/matrix.md` — Matrix views, joins, ownership, cached analysis, and API documentation.
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
