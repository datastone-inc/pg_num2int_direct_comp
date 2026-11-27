# Project Template

> **Location**: Repository root

Templates for extension repository documentation for spec kit repos (specify init ...)
All SQL examples in documentation MUST have corresponding regression tests in `sql/doc_examples.sql`.

## Repository Structure

```
myextension/
├── .specify/
│   └── memory/
│       └── constitution.md    # Governing principles
├── assets/
│   └── datastone-logo.svg
├── code_review_checklist.md
├── project_template.md
├── LICENSE
├── README.md
├── CHANGELOG.md
├── doc/
│   ├── installation.md
│   └── operator-reference.md
├── sql/                       # pg_regress sql tests
│   └── doc_examples.sql       # documentation examples tests
├── expected/
│   └── doc_examples.out
└── ...
```

## README.md

```markdown
# Extension Name

One-line description of what this extension provides.

## Overview

Explanation of extension purpose and capabilities. What problems does it
solve? Who is it for?

## Installation

Requires PostgreSQL 12+ with development headers.

    make && sudo make install

## Quick Start

    CREATE EXTENSION myextension;
    SELECT my_function('example');

## Documentation

See [doc/operator-reference.md](doc/operator-reference.md) for detailed usage and operator reference.

---

[MIT License](LICENSE) | [Changelog](CHANGELOG.md)
```

For simpler extensions without many operators, a single `doc/user-guide.md` may be sufficient instead.

## CHANGELOG.md

Follow [Keep a Changelog](https://keepachangelog.com/) format with semantic versioning.

```markdown
# Changelog

## [Unreleased]

## [1.0.0] - 2026-01-07

### Added
- Initial release with core functionality
```

Categories: Added, Changed, Deprecated, Removed, Fixed, Security.

## doc/ Folder

Start minimal, expand as needed:

| File | When to create |
|------|----------------|
| `installation.md` | Always - setup and quick-start instructions |
| `operator-reference.md` | Primary documentation for extensions exposing operators/functions |
| `user-guide.md` | Alternative to `operator-reference.md` for simpler extensions |
| `benchmark.md` | Extensions with performance characteristics worth documenting |
| `development.md` | Contributor / build / test guidance |
| `migration.md` | Breaking changes between major versions |

### operator-reference.md

```markdown
# Operator Reference

## Operators

### `operator_name(type, type) → returntype`

Explanation with example:

    SELECT operation_example();

## Configuration

GUC parameters or setup requirements, if any.

## Troubleshooting

Common issues and solutions.
```

## Markdown Standards

- Code blocks specify language: ```sql, ```c, ```bash
- Heading hierarchy: # title, ## sections, ### subsections
- Tables use GFM pipe format
- Indented code blocks (4 spaces) acceptable for brief examples

---

**Version**: 1.0.0 | **Last Updated**: 2026-01-07