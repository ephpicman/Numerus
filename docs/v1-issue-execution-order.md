# v1 Statistical Foundation — Issue Execution Order

This file is the execution queue for the issues derived from [the v1 statistical-foundation roadmap](v1-statistical-foundation-roadmap.md).

The order is dependency-aware, not just numerical. Work on tests and documentation incrementally with implementation; do not defer quality work until the end.

## Working agreement

For each issue:
1. Confirm it is still open and its dependencies are satisfied.
2. Create a dedicated branch from the current `main`, named like `issue-<number>-short-description`.
3. Make the smallest correct change, with native tests and PHPT coverage when PHP-visible.
4. Run the relevant tests and CI; fix failures before requesting merge.
5. Open a PR that references the issue (prefer `Closes #N` when the PR fully satisfies it).
6. Merge only after review/readiness and green required checks.
7. Verify the PR is merged and the issue is closed; delete the issue branch after merge unless there is a concrete reason to keep it.
8. Move to the next issue only after checking repository state and any newly exposed dependencies.

Do not create one long-lived branch for the whole roadmap. One issue, one branch, one PR is the default.

## Ordered queue

### Stage 1 — Establish evidence and decisions

1. **[#107 — Audit requirements against source and tests](https://github.com/ephpicman/Numerus/issues/107)** — **Completed in PR #128**  
   Audited all LIN/OPT/RNG/PROB/ENG rows against source, tests and CI definitions. Evidence and status corrections are in [the audit report](v1-statistical-foundation-audit.md); #113/#114 are confirmed gaps subject to #111's contracts.

2. **[#108 — Decide the C/PHP API boundary](https://github.com/ephpicman/Numerus/issues/108)**  
   Record the internal C-first boundary and the separate PHP façade release gate before new public APIs are designed. See [API boundary decision](statistical-api-boundary.md).

3. **[#111 — Specify GLS whitening and log-determinant contracts](https://github.com/ephpicman/Numerus/issues/111)** — **Completed in PR #130**  
   See [GLS whitening and log-determinant contracts](gls-whitening-and-log-determinant-contracts.md). #113/#114 remain implementation issues and must follow these contracts.

4. **[#109 — Specify RNG algorithm and state contract](https://github.com/ephpicman/Numerus/issues/109)** — **Completed in PR #131**  
   PCG-XSH-RR 64/32, explicit seed/stream, clone lifecycle, raw vectors and portability/reproducibility limits are specified in [the RNG contract](reproducible-rng-contract.md). Raw state is implemented in #115; uniform/normal variates remain #116.

5. **[#110 — Define optimizer callback/result/termination contracts](https://github.com/ephpicman/Numerus/issues/110)** — **Completed in PR #132**  
   BFGS/Armijo, optional gradient with central finite differences, result ownership and termination semantics are specified in [optimizer contract](optimizer-contract.md). #119 remains implementation work.

6. **[#112 — Select scalar and log-domain utilities](https://github.com/ephpicman/Numerus/issues/112)** — **Completed in PR #133**  
   Select sigmoid, logit, softplus, log-sigmoid and log-sum-exp; defer wrappers for standard `log1p`/`expm1` and speculative special functions. See [scalar utility selection](scalar-log-domain-utilities.md).

7. **[#125 — Resolve portability and non-finite policies](https://github.com/ephpicman/Numerus/issues/125)**  
   Coordinate this decision with #108–#112; settle policies before APIs spread inconsistent assumptions.

**Stage gate:** implementation contracts and API-boundary decisions are documented. No algorithm or public API is selected implicitly during coding.

### Stage 2 — Close confirmed linear-algebra gaps

8. **[#113 — GLS triangular solves and whitening](https://github.com/ephpicman/Numerus/issues/113)** — **Completed in PR #135**  
   Audit #107 confirmed the gap; this change adds the contracted triangular solve and native coverage. GLS composition reference tests remain in #122.

9. **[#114 — Stable log-determinant](https://github.com/ephpicman/Numerus/issues/114)** — **Completed in PR #136**  
   The audit and #111 contract confirmed the gap. This adds signed log-absolute-determinant using LU factors, preserving outputs on failure.

10. **[#122 — OLS/WLS/GLS reference workflow tests](https://github.com/ephpicman/Numerus/issues/122)** — **Completed in PR #137**  
    The new native workflow test composes OLS, diagonal WLS, GLS whitening, Cholesky and log determinant, with ill-scaled/rank-deficient and invalid-covariance cases. These are composition tests, not model-fitting APIs.

### Stage 3 — Reproducible RNG and resampling

11. **[#115 — Opaque reproducible RNG state](https://github.com/ephpicman/Numerus/issues/115)** — **Completed in PR #138**  
    Requires the decisions in #109 and API-boundary decision in #108.

12. **[#116 — Uniform and normal variates](https://github.com/ephpicman/Numerus/issues/116)** — **Completed in PR #139**  
    Requires #115 and the distribution reproducibility policy.

13. **[#117 — Sampling indices with/without replacement](https://github.com/ephpicman/Numerus/issues/117)** — **Completed in PR #140**  
    Requires #115 and #116.

### Stage 4 — Scalar primitives and optimization

14. **[#118 — Implement selected scalar/log-domain primitives](https://github.com/ephpicman/Numerus/issues/118)** — **Completed in PR #141**  
    Requires the selection in #112 and relevant policies from #125.

15. **[#119 — Generic continuous optimizer](https://github.com/ephpicman/Numerus/issues/119)** — **Completed in PR #142**  
    Requires #110, #118 where needed, and the C/PHP exposure decision in #108.

### Stage 5 — Gaussian probability

16. **[#120 — Multivariate Gaussian log density](https://github.com/ephpicman/Numerus/issues/120)** — **Completed in PR #143**  
    Requires #111, #114 if a log-determinant gap is confirmed, and #118 where selected utilities are needed.

17. **[#121 — Multivariate Gaussian sampling](https://github.com/ephpicman/Numerus/issues/121)** — **Completed in PR #144**  
    Requires #115, #116, and a documented covariance/factorization contract.

### Stage 6 — Cross-cutting quality and documentation

18. **[#123 — Property tests and sanitizer coverage](https://github.com/ephpicman/Numerus/issues/123)** — **Completed in PR #145**  
    Add coverage incrementally as APIs land; final completion follows the implementation stages.

19. **[#124 — C contracts and composable examples](https://github.com/ephpicman/Numerus/issues/124)** — **Completed in PR #146**  
    See [the statistical foundation guide](statistical-foundation.md) and strict-C11 [composable example](../examples/statistical_foundation.c).

### Stage 7 — Final v1 gate

20. **[#126 — Final acceptance audit and release gate](https://github.com/ephpicman/Numerus/issues/126)** — **Audit in this PR; release gate BLOCKED**  
    See [the final audit](v1-statistical-foundation-final-audit.md). Core C primitives and configured Linux CI pass, but userland v1 cannot be declared ready until the PHP façade and PHPT contracts exist; LIN-02 also retains a rank-deficient/minimum-norm acceptance gap. Resolve or formally remove every P0 item before calling v1 ready.

## Parallelism and sequencing rules

- #107 is the first issue; do not assume documented “Existing” status proves source/test completeness.
- #108–#112 can be worked in a small number of independent branches after the initial audit starts, but avoid conflicting edits to the same contract documents.
- #113 and #114 are conditional. A duplicate API is not a valid deliverable.
- #122, #123, and #124 are continuous workstreams; their final acceptance must reflect the actual implementation, not planned features.
- RNG implementation must not begin before #109's reproducibility contract is accepted.
- Optimizer implementation must not begin before #110's callback/result contract is accepted.
- Gaussian probability work must build on the accepted solve, log-determinant, RNG, and scalar contracts.
- Keep MCMC, GP fitting, model-specific OLS/GLS/MLE APIs, and speculative general-purpose frameworks out of v1 unless scope is explicitly revised.

## Completion tracking

Update this file when an issue is merged, closed as unnecessary, or reordered because audit evidence changes the dependency graph. A closed issue is not automatically a completed capability: record the evidence or explicit scope decision in the issue/roadmap.


## Post-audit follow-ups

The original 20-issue statistical-foundation queue is audited and closed. The final audit uncovered two genuine remaining gaps; they are tracked separately rather than hidden by closing #126.

1. **[#148 — Rank-deficient and underdetermined least-squares semantics](https://github.com/ephpicman/Numerus/issues/148)** — resolve first, because the minimum-norm/shape contract should be settled before a PHP wrapper promises its behavior. Reuse the existing SVD/pseudoinverse where sufficient; add reference/property tests.
2. **[#147 — Minimal PHP-facing numerical API](https://github.com/ephpicman/Numerus/issues/147)** — principal release blocker. Design the minimum userland surface against the finalized C contracts, then implement ownership-safe wrappers and PHPT coverage. Keep model-specific estimator APIs out of scope.

The statistical C foundation CI is green, but the overall PHP userland v1 release gate remains **BLOCKED** until #147 and the accepted LIN-02 behavior are resolved. Windows remains unverified.
