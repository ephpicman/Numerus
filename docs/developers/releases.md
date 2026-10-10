# Release Engineering

This document describes release discipline. The [release roadmap](../release-roadmap.md) defines user-facing milestone scope; this document covers packaging and release checks.

## Versioning policy

Use Semantic Versioning for the public PHP API:
- **Patch**: backward-compatible bug fixes and documentation corrections.
- **Minor**: backward-compatible new public capabilities.
- **Major**: intentional incompatible changes to the public API or documented behavior.

Before 1.0, public API contracts may still evolve; nevertheless, avoid gratuitous breaking changes and document intentional ones. A 0.x version should not be represented as a stable 1.0 contract.

Keep the extension version macro in php_numerus.h, Git tag, release notes, and any package metadata synchronized. Do not bump versions merely because internal files were reorganized unless a release is being cut.

## Release checklist

- [ ] Confirm the milestone acceptance criteria in the release roadmap.
- [ ] Ensure public API documentation matches registered methods and PHPT contracts.
- [ ] Run PHPT and native C tests.
- [ ] Verify required CI jobs on the exact release commit.
- [ ] Review supported PHP versions and state unverified platforms honestly (Windows is currently unverified).
- [ ] Review ownership, error behavior, numerical limitations, and known issues.
- [ ] Verify extension version, Git tag, release notes, and package metadata agree.
- [ ] Build and test the distributable source archive in a clean environment.
- [ ] Confirm license and included files.
- [ ] Publish release notes with installation instructions and a concise API scope statement.

## Distribution options

PECL is the legacy PHP extension package registry and has been deprecated in favor of PIE. For a new distribution workflow, evaluate PIE and Packagist first, while preserving the ability to build Numerus from source. If PECL compatibility is explicitly desired, maintain and validate its package metadata separately rather than letting it dictate the development roadmap.

The packaging choice should not change release milestone scope or force a major-version bump by itself. Version numbers are governed by API compatibility and release maturity, not by the registry used to distribute the source.

## No premature stable release

Do not label Numerus 1.0.0 stable until the documented 1.0 gate has passed. A green build is necessary but not sufficient: the PHP API must be coherent, tested, documented, and usable in a clean installation workflow.
