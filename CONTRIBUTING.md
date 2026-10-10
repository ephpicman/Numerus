# Contributing to Numerus

Thank you for considering a contribution.

Numerus aims to provide reliable, reusable numerical primitives for PHP through a native extension. Contributions should improve correctness, usability, reliability, performance, or maintainability without expanding the public API speculatively.

## Before opening a pull request

1. Read the [developer guide](docs/developers/README.md) and the relevant subsystem/API documentation.
2. Check the [release roadmap](docs/release-roadmap.md) and existing issues to understand scope and avoid duplicate work.
3. Keep each pull request focused on one coherent change.
4. Preserve existing behavior unless a deliberate compatibility change is justified and documented.

## Tests and documentation

- Add regression tests for bugs.
- Use PHPT for PHP-visible behavior and standalone C tests for native contracts.
- For numerical changes, include reference cases or mathematical properties with justified tolerances.
- Review error paths, ownership/lifetime, allocation failure, integer overflow, and numerical stability where relevant.
- Update user documentation for public behavior changes and developer documentation for internal contract changes.
- Report exactly which tests were run; do not imply that unrun tests passed.

## Pull request checklist

- [ ] The problem and intended behavior are clear.
- [ ] The change is limited to necessary scope.
- [ ] Success and failure paths are tested.
- [ ] Ownership, cleanup, overflow, and boundary conditions are reviewed.
- [ ] Numerical assumptions and limitations are documented.
- [ ] PHP argument metadata and named-argument behavior are correct when bindings change.
- [ ] Relevant documentation is updated.
- [ ] Required CI checks pass.

Do not merge while required checks are failing or pending. After merge, verify the workflows on the updated default branch.

## Public API changes

The PHP-facing API is a long-term compatibility commitment. Before adding or changing a method, review its naming, types, defaults, named arguments, exceptions, object ownership/cloning, PHP-version compatibility, and whether a general-purpose primitive is preferable to a model-specific wrapper.

## Community standards

All participants are expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md). For security-sensitive reports, follow [SECURITY.md](SECURITY.md) rather than opening a public issue.
