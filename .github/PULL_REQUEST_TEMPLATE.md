## Summary

Describe the problem and the smallest change that solves it.

## Validation

List the exact commands or CI workflows run and their results. Distinguish tests that passed from tests that were not run.

## Compatibility and numerical impact

- Public PHP API or behavior changes:
- Ownership/lifetime and failure-path considerations:
- Numerical assumptions or stability impact:
- Documentation updated:

## Checklist

- [ ] The change is focused and follows the project's established scope.
- [ ] Regression tests or contract tests cover the change.
- [ ] Relevant PHPT and native C tests were run.
- [ ] Error paths, allocation cleanup, overflow, and ownership were reviewed where applicable.
- [ ] Numerical behavior and tolerances are justified where applicable.
- [ ] User/developer documentation is updated where needed.
- [ ] Required CI checks pass; unverified platforms are identified honestly.
- [ ] No secrets, generated binaries, or unrelated changes are included.
