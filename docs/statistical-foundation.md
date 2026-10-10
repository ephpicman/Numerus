# Statistical Foundation C API: Current Capabilities and Composition

Status: implementation reference for the v1 C-internal statistical foundation. This document describes APIs present in the source tree; it does not claim that Numerus exposes them to PHP userland.

## API boundary

The statistical and Matrix APIs are internal C APIs. `numerus.c` currently registers the extension but does not register PHP-visible Matrix, RNG, optimizer, scalar or probability functions/classes. Native C callers can compose these primitives; PHP packages cannot yet call them through a stable userland API. The PHP façade remains a separate release gate described in [the API-boundary decision](statistical-api-boundary.md).

No model-specific estimator API is provided. In particular, Numerus does not implement `fit_ols()`, `fit_wls()`, `fit_gls()`, `fit_mle()`, Gaussian-process fitting or MCMC.

## Available primitives

| Area | Current C capability | Important limitation |
|---|---|---|
| Linear algebra | Matrix multiplication, solve, pivoted-QR least squares, weighted least squares, Cholesky, triangular solve, LU, signed log-absolute-determinant, SVD, pseudoinverse, rank and related analysis | Least-squares API requires full column rank; rank-deficient minimum-norm behavior is not implied. |
| GLS composition | Cholesky factorization, triangular solves and log determinant can be composed to whiten a system and account for covariance determinant terms | There is no model-specific GLS estimator or automatic covariance handling. |
| Scalar numerical functions | Stable sigmoid, logit, softplus, log-sigmoid and log-sum-exp | Finite-input and domain policies apply; this is not a general special-functions library. |
| Optimization | Unconstrained BFGS with Armijo backtracking; optional caller gradient or central finite differences; opaque result, evaluation counters and explicit termination reasons | Smooth deterministic objectives only; no bounds/constraints, Hessian API, automatic differentiation, L-BFGS or PHP callback bridge. |
| RNG and resampling | Explicit opaque PCG-XSH-RR 64/32 state, clone, deterministic raw values, uniform `[0,1)`, normal variates, index sampling with and without replacement | Not cryptographic. Raw integer outputs are bit-reproducible under the contract; floating-point distribution transforms are not promised bit-identical across different `libm` implementations. |
| Gaussian probability | Multivariate Gaussian log density and one-sample generation for finite column vectors and symmetric positive-definite covariance | Requires SPD covariance; singular/PSD covariance is rejected. No distribution hierarchy or catalog is provided. |
| Quality | Focused native reference/failure tests, cross-API property tests and ASan/UBSan test commands in CI | The CI matrix currently validates Ubuntu with PHP 8.2–8.5 plus a debug build; Windows remains unverified. |

## Ownership, errors and numerical policy

- Matrix, RNG and optimizer result handles are opaque. Release owned handles with their matching destroy functions; release index arrays with `numerus_rng_free_indices()`.
- Matrix inputs to the probability functions are borrowed and unchanged. The returned Gaussian sample Matrix is caller-owned.
- The RNG is explicit state: no ambient global RNG is used. Cloning duplicates the exact current state and stream increment.
- Gaussian log density writes its scalar output only on success. Gaussian sampling sets `*sample` to `NULL` before work and publishes the new Matrix only after successful construction.
- Gaussian sampling requires an SPD covariance accepted by Cholesky. It computes `mean + Lz` from `LL^T = covariance`; it does not form a covariance inverse.
- Each normal variate consumes exactly four raw PCG32 outputs (Box–Muller without caching). A dimension-`d` Gaussian sample therefore consumes `4d` raw outputs when successful.
- Optimizer API status and optimizer termination are different: a valid request can return a result whose termination is an iteration limit, line-search failure or callback failure. Do not treat every non-converged result as an initialization/API error.
- Non-finite inputs, invalid domains, shape errors, allocation failures and numerical breakdowns use explicit status/result paths. Do not infer success from a NaN or a partially written output.
- C callbacks and their context are borrowed. The caller must keep context alive and logically unchanged during the call; callbacks may be called repeatedly and must not retain temporary parameter/output pointers.

See [the non-finite and portability policy](numerical-portability-and-nonfinite-policy.md), [RNG contract](reproducible-rng-contract.md), [optimizer contract](optimizer-contract.md), [GLS/log-determinant contract](gls-whitening-and-log-determinant-contracts.md), and [scalar utility selection](scalar-log-domain-utilities.md) for full preconditions and failure semantics.

## Executable composition example

[`examples/statistical_foundation.c`](../examples/statistical_foundation.c) demonstrates composition of:

1. a stable scalar operation;
2. multivariate Gaussian log density;
3. reproducible Gaussian sampling with an explicit RNG;
4. index sampling without replacement;
5. generic minimization of a caller-defined quadratic objective.

It uses only APIs present in the C source tree and checks statuses before consuming results. CI compiles it with strict C11 warnings and executes it on the supported PHP build matrix. The compile command is registered in `.github/workflows/tests.yml`.

Run the example's build/run step manually from the repository root by copying the command under the workflow step named **Build and run composable statistical C example**. The allocator switches select libc for this standalone executable; they are not extension build flags.

## Reproducibility

- Use explicit 64-bit seed and stream inputs for PCG32.
- Exact raw PCG32 vectors are part of the RNG contract and native tests.
- Uniform conversion uses a fixed 53-bit mapping to `[0,1)).
- Normal values use Box–Muller without caching. The algorithm and call count are fixed, but floating-point math can vary across platforms and C math libraries.
- Different stream selectors define distinct PCG streams; this does not establish a general stream-splitting/jump-ahead API or a blanket statistical-independence guarantee.
- Numerus RNG is for simulation and reproducible computation, not cryptography.

## Explicitly not implemented in v1

- PHP-visible Matrix/RNG/optimizer/probability classes or functions.
- Model-specific OLS/WLS/GLS/MLE estimator APIs.
- MCMC engines, Bayesian diagnostics and GP fitting.
- General-purpose distribution hierarchy, broad special functions, GPU/tensor or arbitrary-precision backends.
- Cryptographic random generation.

These are scope decisions or follow-up work, not implied by the presence of the low-level C primitives.
