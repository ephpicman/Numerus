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

1. **[#107 — Audit requirements against source and tests](https://github.com/ephpicman/Numerus/issues/107)**  
   First. Verify every baseline status against actual headers, implementations, tests, and CI. Update the roadmap with evidence and split only confirmed gaps into implementation work.

2. **[#108 — Decide the C/PHP API boundary](https://github.com/ephpicman/Numerus/issues/108)**  
   Decide exposure and compatibility expectations before new public APIs are designed. Coordinate with #107.

3. **[#111 — Specify GLS whitening and log-determinant contracts](https://github.com/ephpicman/Numerus/issues/111)**  
   Use audit findings to define the contracts for GLS and Gaussian calculations.

4. **[#109 — Specify RNG algorithm and state contract](https://github.com/ephpicman/Numerus/issues/109)**  
   Decide algorithm, state lifecycle, test vectors, and reproducibility promises before implementation.

5. **[#110 — Define optimizer callback/result/termination contracts](https://github.com/ephpicman/Numerus/issues/110)**  
   Decide callback semantics, result structure, failure statuses, and initial algorithm candidates.

6. **[#112 — Select scalar and log-domain utilities](https://github.com/ephpicman/Numerus/issues/112)**  
   Select only functions justified by the optimizer, likelihood, and Gaussian use cases.

7. **[#125 — Resolve portability and non-finite policies](https://github.com/ephpicman/Numerus/issues/125)**  
   Coordinate this decision with #108–#112; settle policies before APIs spread inconsistent assumptions.

**Stage gate:** implementation contracts and API-boundary decisions are documented. No algorithm or public API is selected implicitly during coding.

### Stage 2 — Close confirmed linear-algebra gaps

8. **[#113 — GLS triangular solves and whitening](https://github.com/ephpicman/Numerus/issues/113)**  
   Proceed only if #107 confirms a real gap and #111 specifies the contract. If existing APIs already suffice, close this issue with evidence and do not add redundant functionality.

9. **[#114 — Stable log-determinant](https://github.com/ephpicman/Numerus/issues/114)**  
   Proceed only if #107 confirms the current API does not meet the #111 contract. Otherwise close with evidence.

10. **[#122 — OLS/WLS/GLS reference workflow tests](https://github.com/ephpicman/Numerus/issues/122)**  
    Add or extend these tests as the audit and any confirmed linear-algebra gaps permit. These are composition tests, not model-fitting APIs.

### Stage 3 — Reproducible RNG and resampling

11. **[#115 — Opaque reproducible RNG state](https://github.com/ephpicman/Numerus/issues/115)**  
    Requires the decisions in #109 and API-boundary decision in #108.

12. **[#116 — Uniform and normal variates](https://github.com/ephpicman/Numerus/issues/116)**  
    Requires #115 and the distribution reproducibility policy.

13. **[#117 — Sampling indices with/without replacement](https://github.com/ephpicman/Numerus/issues/117)**  
    Requires #115 and #116.

### Stage 4 — Scalar primitives and optimization

14. **[#118 — Implement selected scalar/log-domain primitives](https://github.com/ephpicman/Numerus/issues/118)**  
    Requires the selection in #112 and relevant policies from #125.

15. **[#119 — Generic continuous optimizer](https://github.com/ephpicman/Numerus/issues/119)**  
    Requires #110, #118 where needed, and the C/PHP exposure decision in #108.

### Stage 5 — Gaussian probability

16. **[#120 — Multivariate Gaussian log density](https://github.com/ephpicman/Numerus/issues/120)**  
    Requires #111, #114 if a log-determinant gap is confirmed, and #118 where selected utilities are needed.

17. **[#121 — Multivariate Gaussian sampling](https://github.com/ephpicman/Numerus/issues/121)**  
    Requires #115, #116, and a documented covariance/factorization contract.

### Stage 6 — Cross-cutting quality and documentation

18. **[#123 — Property tests and sanitizer coverage](https://github.com/ephpicman/Numerus/issues/123)**  
    Add coverage incrementally as APIs land; final completion follows the implementation stages.

19. **[#124 — C contracts and composable examples](https://github.com/ephpicman/Numerus/issues/124)**  
    Update incrementally alongside merged APIs. Examples must only use APIs that actually exist and must distinguish current capability from planned work.

### Stage 7 — Final v1 gate

20. **[#126 — Final acceptance audit and release gate](https://github.com/ephpicman/Numerus/issues/126)**  
    Last. Verify every P0 requirement against merged implementation, explicit contract, tests, docs, and required CI. Resolve or formally remove every P0 item before calling v1 ready.

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
