# Statistical Foundation API Boundary

Status: accepted design decision; initial PHP Matrix façade implementation is in progress on `feat/php-matrix-api`.

## Decision

**Implement and validate statistical foundation algorithms first as composable internal C APIs. Do not expose the current Matrix/Storage structs, internal status enum, factorization layout, or allocator details directly to PHP.** Keep C contracts authoritative for ownership, numerical failure, output preservation, and reproducibility.

This is a sequencing decision, not a claim that C-only APIs are sufficient for PHP userland packages. A PHP extension whose useful capabilities are entirely inaccessible from PHP cannot yet serve those packages. Before declaring the overall extension v1 usable for userland statistical packages, Numerus needs a separately designed, minimal PHP façade with stable value/ownership semantics. That façade should be designed against the actual primitive contracts rather than guessed while those contracts are still changing.

## Initial PHP façade scope

The first public surface is a value-owning `Numerus\\Matrix` object with these operations:

- `Matrix::fromRows(array $rows)` and `Matrix::zeros(int $rows, int $columns)`
- `rows()`, `columns()`, and zero-based `get(int $row, int $column)`
- `transpose()`, `multiply(Matrix $other)`, and `inverse()`
- `leastSquares(Matrix $rightHandSide)`

These are generic numerical primitives, not model-specific estimators. Both supported OLS paths should remain possible:

```php
// Explicit normal-equation composition, available to users who choose it:
$beta = $design->transpose()
    ->multiply($design)
    ->inverse()
    ->multiply($design->transpose()->multiply($response));

// Numerically preferable for ordinary use:
$beta = $design->leastSquares($response);
```

Forming `(XᵀX)⁻¹Xᵀy` is intentionally possible, but it is not the recommended default: forming normal equations squares the condition number, and explicit inversion adds avoidable numerical error. The direct least-squares path uses the existing pivoted-QR primitive and reports rank deficiency rather than silently choosing a solution.

For this initial API, returned matrices own independent native storage. `transpose()` materializes the C transpose view before returning, so the public API does not yet expose non-owning views or require parent-object retention. Inputs must be rectangular, non-empty nested arrays containing only PHP integers/floats; matrix coordinates are zero-based. Native statuses map to PHP exceptions rather than public integer codes.

Deferred from this first increment: arithmetic operator overloading, lazy public views, mutable element assignment, public factorization handles, RNG/optimizer callbacks, and broad statistical wrappers. These should be added only with explicit lifetime/error contracts and a demonstrated compositional need.

## Why this boundary fits the current code

- `numerus.c` currently registers an extension module but no PHP functions or classes.
- `numerus_matrix.h` exposes an opaque internal Matrix type with a broad C status enum. Matrix views retain non-owning parent references; the future PHP object layer is explicitly responsible for preserving parent lifetimes.
- Storage and Matrix use C allocation conventions in the extension, while standalone native tests compile with allocator switches. PHP wrappers must not leak allocator or lifetime assumptions across the language boundary.
- Existing PHPTs test extension loading/version only. Numerical behavior is exercised through native C tests.
- Statistical features such as RNG state, optimizer callbacks and factorization handles need lifecycle and error mapping contracts that should be settled before wrapper design.

## Requirement exposure classification

“C-internal” means the first implementation and authoritative contract live in C; it does **not** mean the capability is permanently forbidden from PHP. “PHP façade required before overall userland v1” identifies generic primitives that package authors must ultimately be able to compose from PHP.

| Requirement group | Foundation implementation | PHP userland exposure decision |
|---|---|---|
| LIN-01–LIN-09: matrix operations, solves, least squares, weighting, factorization, covariance helpers | C-internal APIs; opaque Matrix/factorization handles; native tests | Generic Matrix/value operations and solve/least-squares capabilities are candidates for the minimal PHP façade. Do not expose raw pointers or decomposition buffers. |
| OPT-01: stable scalar/log-domain helpers | C-internal pure numerical functions first | Expose selected scalar helpers only if useful as a stable standalone userland operation; keep a single documented numerical policy across C and PHP. |
| OPT-02–OPT-08: objective callbacks and optimizer | C-internal callback/result contract and native algorithm tests first | PHP callbacks are a separate design choice: exception propagation, reentrancy, callback cost, context lifetime and bailout behavior must be specified before wrappers are approved. Do not assume a C callback can be exposed directly. |
| RNG-01–RNG-10: PRNG, explicit state, variates and resampling | C-internal opaque state and deterministic vectors first | A PHP-visible RNG state object or equivalent explicit state handle is likely required for reproducible userland use. Do not use ambient global state as the implicit API. |
| PROB-01–PROB-04: Gaussian density/sampling and generic objective composition | C-internal composition of accepted factorization/scalar/RNG contracts | Generic density/sampling operations are façade candidates after the matrix value and RNG state models are settled. No model-specific estimator or inference object is implied. |
| PROB-05–PROB-09: MCMC, Bayesian diagnostics, GP fitting, broad distributions/special functions | Deferred or contract decision per roadmap | No PHP wrapper work until scope is explicitly accepted and there is a concrete reusable consumer. |
| ENG-01–ENG-08, ENG-10–ENG-11: ownership, statuses, checked sizes, numeric policy, tests, portability, docs | C contract is authoritative; native tests for internal APIs and PHPT for wrappers | PHP-visible APIs require PHPTs for type/shape validation, exception/error mapping, object lifetime, and user-visible behavior. C status codes must not be exposed as undocumented integers. |
| ENG-09: public PHP API shape | Decision recorded here | This issue does not authorize a broad public API or direct exposure of Matrix internals. A dedicated façade design must define the minimum stable PHP surface and is a prerequisite to claiming a userland-ready v1. |

## Contract and naming rules

1. Keep C symbols prefixed `numerus_` and declarations in focused internal headers. Do not promise a stable external C ABI in v1 unless separately versioned and documented.
2. Keep owning objects opaque. Every constructor documents ownership transfer and failure output state; every retained handle has an explicit release operation.
3. Borrowed callback contexts must have documented lifetime, mutability, reentrancy and thread-safety assumptions. C callbacks are not automatically PHP callbacks.
4. Keep failure categories distinct in C. PHP wrappers must map invalid arguments, non-finite input, singularity, non-convergence and allocation failure to documented PHP-visible failures; never rely on NaN alone to signal failure.
5. PHP objects that wrap Matrix views must retain their parent objects so the non-owning C parent pointer remains valid. A wrapper must not create dangling view parents.
6. Keep PHPT coverage for every PHP-visible contract. Native tests remain necessary for C-level ownership, numerical invariants, allocation failure and deterministic vectors.
7. Do not introduce model-specific `fit_ols()`, `fit_gls()`, `fit_mle()`, GP fitting, or Bayesian inference APIs.

## Release implication

The statistical foundation roadmap can track internal primitives as implemented, but **overall userland v1 readiness cannot be claimed until the PHP façade question is resolved and the accepted façade is implemented and tested**. Track that work separately so it does not silently expand every primitive issue or mix wrapper design with numerical algorithm development.
