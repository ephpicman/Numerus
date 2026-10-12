# Contribution Workflow

Numerus aims to provide general-purpose, composable numerical primitives for PHP. Contributions should solve real correctness, usability, reliability, or maintainability problems without widening the public API unnecessarily.

## Before changing code

1. Read the relevant API header and implementation.
2. Read nearby tests and existing documentation.
3. Identify whether the change affects the public PHP API, internal C API, or both.
4. Check the release roadmap and existing issues to avoid duplicating work or conflicting with established scope.
5. State intended behavior and important edge cases before choosing an implementation.

## Keep changes focused

- Prefer one coherent change per pull request.
- Avoid unrelated formatting or broad refactors mixed with bug fixes.
- Preserve compatibility unless a deliberate API change is justified and documented.
- Do not add a model-specific estimator when existing generic primitives can express the workflow.
- Avoid speculative features without a concrete general use case.

## Tests and documentation

Add regression tests for bugs. For public PHP behavior, use PHPT. For native subsystem behavior, use standalone C tests. Numerical changes should include suitable reference cases or mathematical properties and justified tolerances.

Update documentation in the right place:
- docs/user-guide.md for user workflows and public capabilities;
- docs/php-api.md for PHP method contracts;
- docs/developers/ for implementation and contributor workflows;
- subsystem documents for internal C contracts;
- the roadmap only when milestone scope or ordering genuinely changes.

## Pull request checklist

- [ ] The problem and intended behavior are clear.
- [ ] The change is limited to necessary scope.
- [ ] Success and failure paths have tests.
- [ ] Allocation cleanup, overflow, bounds, and ownership have been reviewed.
- [ ] Numerical assumptions and stability implications are documented.
- [ ] PHP argument metadata and named-argument behavior are correct when bindings change.
- [ ] User and developer docs are updated as applicable.
- [ ] CI checks are green; unresolved platform limitations are stated honestly.

Do not merge while required checks are failing or pending. After merge, check main-branch workflows.

## Public API changes

A PHP-facing API is a long-term compatibility commitment. Before adding or changing one, review method naming, argument types/defaults, named-argument compatibility, exceptions, ownership/cloning, PHP 8.2+ compatibility, and whether the API exposes a reusable primitive rather than an unnecessary model-specific wrapper.

## Reporting test results

In pull request descriptions, distinguish tests run locally, checks run by CI, and tests not run. Compilation or coverage alone is not proof that numerical behavior is correct.
