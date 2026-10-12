# Testing Numerus

Numerus has two complementary test layers: PHPT tests for PHP-visible behavior and standalone C tests for native subsystem contracts. CI adds PHP-version matrix and debug-oriented builds, plus native coverage reporting.

## PHPT tests

Build the extension first, then run:

~~~sh
make test TESTS='tests/*.phpt'
~~~

PHPT tests should cover observable behavior, including return values, argument validation, exception classes/messages where contractual, named arguments and reflection metadata, and object lifecycle.

When adding a PHP method, include tests for:
- the ordinary successful case;
- invalid dimensions, shapes, and indices relevant to the method;
- type errors and argument mismatch;
- numerical failure cases such as singularity, rank deficiency, or non-finite input where applicable;
- object lifetime/ownership behavior when native memory is involved;
- reflection or named arguments if method arginfo is affected.

Test the user-observable contract, not merely implementation details.

## Standalone native tests

Run from the repository root:

~~~sh
cc -Isrc/core -Isrc/storage -Isrc/matrix -Isrc/statistics -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/storage_test tests/storage_test.c src/storage/numerus_storage.c
./tests/storage_test

cc -Isrc/core -Isrc/storage -Isrc/matrix -Isrc/statistics -std=c11 -Wall -Wextra -Wpedantic -Werror \
  -DNUMERUS_MATRIX_USE_LIBC_ALLOC \
  -DNUMERUS_STORAGE_USE_LIBC_ALLOC \
  -o tests/matrix_test tests/matrix_test.c src/matrix/numerus_matrix.c src/matrix/numerus_matrix_view_nodes.c src/matrix/numerus_matrix_constructors.c src/storage/numerus_storage.c
./tests/matrix_test
~~~

The allocator defines are for standalone test executables only. Extension builds use the Zend Memory Manager.

## Numerical tests

Prefer reference solutions and properties over testing only a few fixed outputs. Useful properties include:
- A multiplied by its inverse is close to identity for well-conditioned matrices;
- residual orthogonality for least squares;
- supported inputs produce finite results;
- symmetry or positive-definiteness constraints are respected;
- dimension and rank failures are explicit;
- equivalent formulations agree within a justified tolerance.

Choose tolerances based on floating-point behavior and conditioning. One well-conditioned fixture does not establish universal accuracy.

## CI and coverage

CI runs PHPT and native tests on supported PHP versions and includes a debug-oriented build. Native line/branch coverage helps identify untested C paths, but coverage percentage alone does not establish correctness.

See [Native Test Coverage](../native-test-coverage.md) for artifacts and interpretation.

## Before requesting review

- Run focused tests for the changed contract.
- Run PHPT and native tests where relevant.
- Check debug build output and compiler warnings.
- Review failure cleanup, allocation errors, overflow, invalid pointers, and aliasing/lifetime assumptions.
- State exactly which commands passed and which were not run.
