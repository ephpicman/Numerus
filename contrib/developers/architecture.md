# Architecture and Ownership

This document describes implementation boundaries, not the PHP-visible feature list. Use the [user guide](../../docs/user-guide.md) for supported PHP methods.

## Subsystems

### Core

src/core/ contains checked size arithmetic and scalar numerical helpers. Shared low-level checks should remain small and reusable.

### Storage

src/storage/ implements opaque, immutable matrix storage. Supported representations include dense, triangular, diagonal, identity, constant, zero, scaled identity, sparse, symmetric, and banded storage. Constructor inputs are copied into storage-owned memory. Representation details must not leak into higher-level consumers.

### Matrix

src/matrix/ provides a logical matrix abstraction over Storage. The native API contains roots, lazy views, joins, arithmetic operations, analysis, and numerical methods. Internal views borrow parent Matrix references; callers must obey the lifetime contract documented by the relevant C API.

### PHP binding

src/numerus_matrix_php.c exposes a deliberately smaller surface as Numerus\Matrix. PHP-returned Matrix values must have safe independent lifetimes. The current binding materializes native results before returning them instead of exposing internal borrowed views.

The PHP object owns native storage. Cloning is unsupported to prevent a shallow object copy from creating ambiguous ownership or double-free risks.

### Statistical primitives

src/statistics/ contains reusable C-level primitives. A function existing in C does not automatically mean it is part of the supported PHP API. The boundary between generic primitives and model-specific estimators is deliberate: consumers compose models rather than depending on a built-in estimator for every model family.

## Design constraints

- Keep public structs opaque where implementation layout should remain private.
- Make ownership transfer and borrowed references explicit in names, headers, and comments.
- Preserve output arguments on failure where the API contract promises this.
- Check dimensions and allocation arithmetic before allocating or indexing.
- Ensure partial-allocation failure paths release resources.
- Prefer stable solves/factorizations to explicit inversion in numerical workflows.
- Keep PHP exception mapping consistent with the documented contract.
- Do not expose a new PHP method without PHPT coverage for success, invalid inputs, failure behavior, and lifecycle where relevant.

## Where to make changes

| Concern | Likely location |
|---|---|
| PHP class, method registration, arginfo, object handlers | src/numerus_matrix_php.c |
| Extension module initialization | src/numerus.c |
| Matrix storage representation | src/storage/ |
| Matrix operations and decompositions | src/matrix/ |
| Shared numerical helpers | src/core/ |
| RNG, probability, optimization primitives | src/statistics/ |
| PHP-visible regression tests | tests/*.phpt |
| Standalone C contract tests | tests/*_test.c |
| User-facing behavior and examples | docs/user-guide.md, docs/php-api.md, examples/ |
