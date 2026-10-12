# Numerus Documentation

Numerus is a native PHP extension providing composable numerical-computing primitives. This documentation is split by audience so users can learn the public PHP API without confusing it with internal C capabilities.

## User guide

- [User guide](../docs/user-guide.md) — requirements, build instructions, Matrix API, OLS example, errors, and current limitations.
- [PHP API reference](../docs/php-api.md) — currently exposed PHP methods.
- [Runnable OLS example](../examples/ols.php) — executable example source.

## Developer and contributor guide

- [Developer guide](developers/README.md) — setup, architecture map, testing, and contribution workflow.
- [Architecture](developers/architecture.md) — subsystem boundaries and ownership model.
- [Testing](developers/testing.md) — PHPT, native C tests, debug builds, and coverage.
- [Contribution workflow](developers/contributing.md) — focused changes, tests, review, and compatibility.
- [Release engineering](developers/releases.md) — versioning and pre-release checks.

## Internal technical references

These documents describe implementation details and internal C APIs. They are not a promise that each capability is exposed to PHP.

- [Matrix internals](matrix.md)
- [Storage internals](storage.md)
- [Native test coverage](native-test-coverage.md)
- [Statistical API boundary](statistical-api-boundary.md)
- [Release roadmap](release-roadmap.md)
- [Statistical foundation roadmap](v1-statistical-foundation-roadmap.md)

## Documentation rule

The implemented PHP API and its tests define the public user-facing contract. Roadmaps describe intended work, while internal C documentation describes implementation capabilities; neither should be read as a guarantee that an unexposed feature is available from PHP.
