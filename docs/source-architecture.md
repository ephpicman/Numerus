# Source Architecture and Behavior-Preserving Refactor Plan

## Status

This document records the source layout observed on `main` at commit `437ba2c6f8d11c185d64c39b356a0772859aca0e` and the proposed target layout. It is a refactoring plan, not a claim that files have already moved.

The objective is to improve responsibility boundaries and maintainability without changing C APIs, numerical results, ownership rules, error contracts, or extension behavior.

## Current-state findings

- Most implementation `.c` files and internal `.h` files live in the repository root.
- `config.m4` enumerates all Unix extension source files explicitly.
- `config.w32` currently lists only `numerus.c` and `numerus_storage.c`, unlike the Unix manifest. Windows builds are documented as unverified. The refactor must not silently claim to fix or validate Windows support; the manifest discrepancy should be handled explicitly.
- `.github/workflows/tests.yml` contains many standalone native compiler commands that list source paths directly. Moving files requires updating all of those references, not only the extension manifest.
- Native tests and PHPT files are under `tests/`; examples and benchmarks have their own directories.
- The Matrix API has a public-to-C header (`numerus_matrix.h`) and a private helper header (`numerus_matrix_internal.h`), plus focused factorization headers (`numerus_matrix_lu.h`, `numerus_matrix_qr.h`). These boundaries must be retained or improved, not blurred.
- Matrix view objects borrow parent Matrix pointers. Refactoring must preserve that lifetime contract and the single authoritative private Matrix representation.
- No PHP-visible Matrix or statistical facade is registered yet. This refactor must not accidentally imply that the userland API blocker is resolved.

## Target top-level layout

Keep PHP extension build/configuration files at the repository root, with implementation under `src/`:

```text
Numerus/
├── src/
│   ├── numerus.c
│   ├── core/
│   │   ├── numerus_numeric.c
│   │   ├── numerus_numeric.h
│   │   └── numerus_size.h
│   ├── storage/
│   │   ├── numerus_storage.c
│   │   └── numerus_storage.h
│   ├── matrix/
│   │   ├── numerus_matrix.c
│   │   ├── numerus_matrix.h
│   │   ├── numerus_matrix_internal.h
│   │   ├── operations and view translation units grouped by responsibility
│   │   └── focused factorization headers/implementations
│   └── statistics/
│       ├── numerus_rng.c
│       ├── numerus_rng.h
│       ├── numerus_rng_sampling.c
│       ├── numerus_optimizer.c
│       ├── numerus_optimizer.h
│       ├── numerus_probability.c
│       └── numerus_probability.h
├── tests/
├── benchmarks/
├── examples/
├── docs/
├── config.m4
├── config.w32
├── php_numerus.h
└── README.md
```

This is a responsibility map, not permission to move each file mechanically into the nearest-looking directory. Final paths are settled in the implementation PR after checking every include and translation-unit dependency.

## Proposed file mapping

| Current file(s) | Responsibility | Target |
|---|---|---|
| `numerus.c` | PHP extension module entry point | `src/numerus.c` |
| `numerus_size.h`, `numerus_numeric.c`, `numerus_numeric.h` | Checked size arithmetic and scalar numeric helpers | `src/core/` |
| `numerus_storage.c`, `numerus_storage.h` | Opaque immutable storage representations and API | `src/storage/` |
| `numerus_matrix*.c`, `numerus_matrix*.h` | Matrix representation, views, operations, factorization and linear algebra | `src/matrix/`, with further cohesive grouping decided from the include/call graph |
| `numerus_rng.c`, `numerus_rng.h`, `numerus_rng_sampling.c` | Explicit-state random generation and sampling | `src/statistics/` |
| `numerus_probability.c`, `numerus_probability.h` | Probability primitives composed from Matrix and RNG | `src/statistics/` |
| `numerus_optimizer.c`, `numerus_optimizer.h` | Generic optimizer and result lifecycle | `src/statistics/` |
| `php_numerus.h`, `config.m4`, `config.w32` | PHP extension-facing declarations and build configuration | Keep at root |
| `tests/`, `benchmarks/`, `examples/`, `docs/` | Validation, benchmarks, examples and documentation | Keep top-level |

## File-splitting decisions

### Matrix implementation

`numerus_matrix.c` is approximately 77 KB. The file defines the opaque Matrix representation and also implements many behaviors. Its private struct includes storage and parent links, lazy-view transforms, composition state, cached flags, determinant/inverse/LU cache state, and specialized view metadata.

Do not move this struct into multiple headers or copy its definition between translation units. The preferred approach is:

1. Keep the concrete `struct numerus_matrix` definition in one private implementation header or one implementation unit.
2. Separate cohesive behavior into implementation units only where private-state access can be expressed through a small set of documented internal helpers.
3. Keep allocation/free policy, parent lifetime assumptions, cache invalidation, and output-on-failure behavior unchanged.
4. Prefer an explicitly internal header over exporting helper symbols as part of the supported C API.
5. Do not create one-function files or split code solely to meet a line-count target.

Candidate responsibilities to validate against the actual call graph:
- lifecycle, storage ownership and element access;
- view/transform creation and composition;
- cached structural analysis and determinant/inverse/LU caches;
- arithmetic, materialization and element-wise operations;
- decomposition and solve algorithms.

### Matrix declarations

`numerus_matrix.h` is approximately 50 KB. Review declaration groups and downstream includes before splitting. If focused headers reduce compile-time coupling, split declarations by cohesive domain while preserving a compatibility umbrella header (`numerus_matrix.h`) that includes the focused headers. Do not force all consumers to migrate at once, and do not introduce include cycles.

### Other large files

Review `numerus_optimizer.c`, `numerus_storage.c`, `numerus_matrix_analysis.c`, `numerus_matrix_svd.c`, `numerus_matrix_exponential.c`, and `numerus_probability.c` for mixed responsibilities. Their size alone is not sufficient reason to split them; only make additional splits when code and dependency evidence supports a clear boundary.

## Build and test impact checklist

Every implementation PR must account for:

- `config.m4` source list and include paths;
- `config.w32` manifest, while clearly retaining the existing Windows-validation caveat unless Windows CI is added;
- every source path in `.github/workflows/tests.yml`, including native test compile lines, property tests, sanitizer jobs, and the strict-C11 example;
- standalone native-test commands in README and docs;
- benchmark compile commands and examples;
- quoted includes between implementation and internal headers;
- `phpize`, `./configure --enable-numerus`, `make`, extension loading and PHPT;
- strict native builds using `-std=c11 -Wall -Wextra -Wpedantic -Werror`;
- ASan/UBSan and property tests;
- PHP 8.2–8.5 and debug CI jobs.

A successful textual path search is necessary but not sufficient. Build and test results are the acceptance evidence.

## Execution sequence

1. **#152 — Architecture and dependency map.** Document current layout, file responsibilities, manifests, and the target mapping. No behavior changes.
2. **#153 — Mechanical source relocation.** Move implementation and internal headers into the approved `src/` hierarchy; update build/test paths only. Do not split algorithms in the same diff.
3. **#154 — Responsibility-level decomposition.** Split the Matrix core and declaration surface based on call graph and private-state boundaries. Preserve the umbrella header and C signatures where feasible.
4. Re-audit remaining large files and add follow-up issues only for justified responsibility splits. Avoid a broad refactor without measurable maintainability benefit.

Each issue uses its own branch from current `main`, one PR, required green CI before merge, then verify merge and issue closure. If any baseline test fails, record it before moving files so that it is not misattributed to the refactor.

## Non-goals

- No new numerical algorithms, API features, PHP facade, or model-fitting functions.
- No renaming of exported C symbols or changes to status/error contracts.
- No algorithmic rewrites, allocation changes, cache-policy changes, or numerical tolerance changes.
- No claim of Windows compatibility without a passing Windows build/test job.
