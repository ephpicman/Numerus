# Numerus Developer Guide

This guide is for contributors working on the C implementation, PHP binding, tests, and release process.

## Development setup

Numerus is a PHP extension written in C. You need PHP development tools (phpize and matching PHP headers), Autoconf, a C compiler, and build tools.

~~~sh
phpize
./configure --enable-numerus
make
make test TESTS='tests/*.phpt'
~~~

A successful compilation alone does not prove the PHP-visible contract is correct.

## Architecture at a glance

- src/core/ — checked size arithmetic and scalar numerical helpers.
- src/storage/ — opaque immutable matrix storage representations.
- src/matrix/ — matrix operations, decompositions, analysis, and view implementation.
- src/statistics/ — internal statistical primitives such as random generation, probability, and optimization.
- src/numerus_matrix_php.c — PHP-facing Matrix object and method bindings.
- src/numerus.c — extension module entry point.
- tests/*.phpt — PHP-visible behavior and lifecycle contracts.
- tests/*_test.c — standalone native C tests.
- examples/ — runnable user-facing examples.
- docs/ — public guide, developer notes, API contracts, and roadmap.

Read [Architecture](architecture.md) for subsystem boundaries and ownership. For the public PHP surface, start with the [user guide](../user-guide.md) and [PHP API reference](../php-api.md).

## Development principles

1. Make the smallest correct change that fixes the identified problem.
2. Preserve public behavior unless a deliberate, documented change is required.
3. Keep ownership and lifetime rules explicit. Do not return borrowed C views to PHP unless parent lifetime is safely managed.
4. Validate dimensions, indices, and allocation sizes; account for overflow and allocation failure.
5. Prefer stable numerical algorithms and document assumptions such as rank, symmetry, and positive definiteness.
6. Add focused regression tests for bugs and contract tests for public behavior.
7. Do not describe an internal C primitive as a supported PHP feature unless the PHP API exposes it.
8. Avoid model-specific convenience APIs when composable primitives are sufficient.

## Typical change workflow

1. Identify the affected subsystem and existing contracts.
2. Reproduce the issue or define the intended behavior.
3. Add a focused regression test where practical.
4. Implement the smallest change that satisfies the contract.
5. Run relevant native tests and PHPT; run broader CI when possible.
6. Review error paths, allocation cleanup, overflow, aliasing, ownership, numerical edge cases, and PHP-version compatibility.
7. Update user documentation for public API changes and developer documentation for internal contract changes.
8. Open a pull request with the problem, approach, tests run, and known limitations.

## Testing and review

See [Testing](testing.md). Do not merge a pull request while required checks are failing or still running. After merge, verify main-branch workflows too; PR checks alone do not prove the merged commit is green.

## Contribution and release references

- [Contribution workflow](contributing.md)
- [Testing guide](testing.md)
- [Release engineering](releases.md)
- [Release roadmap](../release-roadmap.md)
- [Statistical API boundary](../statistical-api-boundary.md)
