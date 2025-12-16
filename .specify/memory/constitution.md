# PostgreSQL Extension Development Constitution

## Core Principles

### I. Code Style & Formatting

**Backend C Code (PostgreSQL Extensions)**

C code for PostgreSQL backend extensions MUST follow K&R style with 2-space indentation.
Blocks use opening brace on same line as control statement, closing brace aligned with
control keyword. C++ is FORBIDDEN in PostgreSQL backend code.

Example:

```c
/*
 * Copyright (c) 2026 dataStone Inc.
 * 
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

/**
 * @file extension_utils.c
 * @brief Utility functions for extension data processing
 * @author Dave Sharpe
 * @date 2025-12-13
 * 
 * This file was developed with assistance from AI tools.
 */

#include "postgres.h"
#include "fmgr.h"

/**
 * @brief Process input data and return formatted result
 * @param inputData Raw input requiring validation
 * @param maxLength Maximum allowed length for output
 * @return Processed data string or NULL on error
 */
static char *
processData(const char *inputData, int maxLength) {
  char *result;
  int   len;

  // Validate input parameters before processing
  if (inputData == NULL || maxLength <= 0) {
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("invalid input parameters")));
  }

  len = strlen(inputData);
  if (len > maxLength) {
    ereport(WARNING,
            (errmsg("input truncated from %d to %d chars", len, maxLength)));
    len = maxLength;
  }

  // Allocate and populate result buffer
  result = (char *)palloc(len + 1);
  memcpy(result, inputData, len);
  result[len] = '\0';

  return result;
}
```

Naming MUST use camelCase for functions/variables, keeping names meaningful but concise
(prefer `procData` over `processIncomingDataFromClient`). PostgreSQL's conventional
snake_case is acceptable for public API functions that integrate with existing
PostgreSQL naming patterns.

**Client C++ Code (Tools, Utilities, Applications)**

C++ code for client-side tools and utilities MUST use C++14 standard. Code MUST
emphasize clarity and modern idiomatic C++ using the standard library. RAII and smart
pointers are REQUIRED for resource management. Template metaprogramming complexity
(SFINAE, CRTP, expression templates) is FORBIDDEN unless clearly justified.

Encouraged C++ features:
- RAII for automatic resource management
- Smart pointers (unique_ptr, shared_ptr) instead of raw pointers
- Standard library containers and algorithms
- Range-based for loops
- `auto` keyword for type deduction
- Lambdas when needed (especially for std algorithms)

Discouraged/forbidden C++ features:
- Template metaprogramming tricks (SFINAE, enable_if)
- CRTP (Curiously Recurring Template Pattern)
- Expression templates
- Operator overloading without clear benefit
- Excessive template usage without clear advantage

Example client code:

```cpp
/*
 * Copyright (c) 2026 dataStone Inc.
 * 
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

/**
 * @file connection_manager.cpp
 * @brief PostgreSQL connection pool manager for client applications
 * @author Dave Sharpe
 * @date 2025-12-13
 * 
 * This file was developed with assistance from AI tools.
 */

#include <memory>
#include <string>
#include <vector>
#include <libpq-fe.h>

/**
 * @brief RAII wrapper for PostgreSQL connection
 */
class PgConnection {
  std::unique_ptr<PGconn, decltype(&PQfinish)> conn_;

public:
  explicit PgConnection(const std::string& connStr)
    : conn_(PQconnectdb(connStr.c_str()), PQfinish) {
    
    if (PQstatus(conn_.get()) != CONNECTION_OK) {
      throw std::runtime_error("Connection failed");
    }
  }

  // Execute query and return results
  auto execute(const std::string& query) {
    return std::unique_ptr<PGresult, decltype(&PQclear)>(
      PQexec(conn_.get(), query.c_str()),
      PQclear
    );
  }
};
```

Client C++ code MUST follow same K&R formatting as C (2-space indent, same-line braces).
Use camelCase for functions/variables, PascalCase for classes/types.

### II. Code Documentation Standards

Every C/C++/header/SQL file MUST begin with the copyright notice followed by doxygen
file header (see Principle I for complete file structure example). This applies to both
backend C files and client C++ files. File headers MUST include `@file`, `@brief`,
`@author`, and `@date` tags. An AI assistance caveat MUST be included when applicable.

Function headers MUST document purpose, parameters (`@param`), and return values
(`@return`). For PG_FUNCTION_INFO_V1 functions, `@param` MUST describe the SQL-level
arguments (0, 1, etc.) not the C-level fcinfo parameter. For complex functions,
include `@note` for implementation details or caveats.

Example function documentation:

```c
/**
 * @brief Compare two custom types for equality
 * @param 0 First custom type value to compare
 * @param 1 Second custom type value to compare
 * @return Boolean true if values are equal, false otherwise
 */
PG_FUNCTION_INFO_V1(customtype_eq);
Datum
customtype_eq(PG_FUNCTION_ARGS) {
  CustomType *a = PG_GETARG_CUSTOMTYPE_P(0);
  CustomType *b = PG_GETARG_CUSTOMTYPE_P(1);

  // Direct field comparison sufficient for this type
  PG_RETURN_BOOL(a->value == b->value);
}
```

Inline comments MUST use C++ `//` style, being explanatory but succinct. Avoid obvious
comments like `// increment counter` for `counter++`. Focus on explaining WHY
decisions were made, not WHAT the code does when it's self-evident.

The `@date` tag MUST reflect the original file creation date. Update comments should
be tracked via version control, not in the file header.

### III. Test-First Development (NON-NEGOTIABLE)

TDD is MANDATORY using PostgreSQL's pg_regress framework. Development cycle MUST follow:

1. Write regression test in `sql/` defining expected behavior
2. Create expected output in `expected/`
3. Verify test FAILS with current code
4. Implement feature in C/SQL
5. Verify test PASSES
6. Refactor with tests as safety net

Every feature, operator, function, or behavior change MUST have corresponding regression
tests. Test coverage MUST include:
- Normal operation cases
- Boundary conditions
- NULL handling
- Error conditions with appropriate error messages
- Index usage verification (EXPLAIN output)
- Catalog integration

Example test structure:
```sql
-- sql/custom_ops.sql
\set ECHO none
\pset format unaligned

-- Test basic equality operator
SELECT 'basic_eq'::text AS test, customtype_eq('foo', 'foo');
SELECT 'basic_ne'::text AS test, customtype_eq('foo', 'bar');

-- Test NULL handling
SELECT 'null_lhs'::text AS test, customtype_eq(NULL, 'foo');
SELECT 'null_rhs'::text AS test, customtype_eq('foo', NULL);

-- Test index usage
EXPLAIN (COSTS OFF) SELECT * FROM test_table WHERE custom_col = 'target';
```

### IV. SQL Coding Standards

All SQL and PL/pgSQL MUST use UPPERCASE for keywords, lowercase for identifiers. Table
and column names MUST use snake_case. SQL statements MUST be formatted for readability
with aligned keywords and proper indentation.

Extension SQL files MUST include:
- Copyright notice (same format as C files)
- File header comment with filename, description, and AI assistance caveat
- psql execution guard (`\echo Use "CREATE EXTENSION..." \quit`)
- Object definitions (types, functions, operators, indexes)

Example extension installation script:

```sql
/*
 * Copyright (c) 2026 dataStone Inc.
 * 
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

-- myextension--1.0.0.sql
--
-- Extension installation script for myextension version 1.0.0
-- This file defines custom types, functions, and operators.
--
-- This file was developed with assistance from AI tools.

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION myextension" to load this file. \quit

CREATE TYPE custom_type AS (
  id          INTEGER,
  name        TEXT,
  created_at  TIMESTAMP WITH TIME ZONE
);

CREATE OR REPLACE FUNCTION process_custom(input custom_type)
  RETURNS TEXT
  LANGUAGE plpgsql
  STABLE
AS $$
DECLARE
  result_text TEXT;
BEGIN
  -- Validate input before processing
  IF input.id IS NULL THEN
    RAISE EXCEPTION 'id cannot be NULL';
  END IF;

  SELECT format('Processing: %s (ID: %s)', input.name, input.id)
    INTO result_text;

  RETURN result_text;
END;
$$;

CREATE OPERATOR ~= (
  LEFTARG = custom_type,
  RIGHTARG = custom_type,
  FUNCTION = customtype_eq,
  COMMUTATOR = ~=,
  NEGATOR = ~<>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);
```

### V. PGXS Build System

Every extension MUST use PostgreSQL's PGXS framework for building and installation.
Makefiles MUST define `EXTENSION`, `DATA`, `MODULES`, and include `PGXS.mk`. Build
system MUST support standard targets: `all`, `install`, `installcheck`, `clean`.

```makefile
# Minimal PGXS-based Makefile
EXTENSION = myextension
DATA = myextension--1.0.0.sql
MODULES = myextension
REGRESS = basic_ops advanced_ops edge_cases

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
```

Version upgrade scripts MUST follow naming convention `extension--oldver--newver.sql`.
Control files MUST specify `default_version`, `module_pathname`, `relocatable`, and
`schema` where appropriate.

Extension versions MUST use semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Incompatible API changes, breaking schema modifications
- MINOR: Backward-compatible functionality additions, new features
- PATCH: Backward-compatible bug fixes, documentation updates

Example version progression: `1.0.0` → `1.1.0` (new function) → `1.1.1` (bugfix) →
`2.0.0` (breaking change).

### VI. User Documentation

Every extension repository MUST include comprehensive user-facing documentation. All
documentation MUST be kept current with the latest code and include an AI assistance
caveat where applicable.

**README.md Requirements**

The repository root MUST contain a README.md that includes:
- Extension name and brief description
- Installation instructions (prerequisites, build, install)
- Quick start guide with basic usage examples
- Link to detailed documentation in `doc/` folder
- Build requirements and dependencies
- License and copyright notice
- Link to CHANGELOG.md

Example README.md structure:

```markdown
# My Extension

Brief description of what this extension does.

This documentation was developed with assistance from AI tools.

## Installation

Prerequisites:
- PostgreSQL 12 or later
- Development headers installed

Build and install:
```bash
make
sudo make install
```

## Quick Start

```sql
CREATE EXTENSION myextension;
SELECT process_custom(ROW(1, 'test', NOW())::custom_type);
```

## Documentation

See [doc/](doc/) for detailed documentation:
- [User Guide](doc/user-guide.md)
- [API Reference](doc/api-reference.md)
- [Installation Guide](doc/installation.md)

## License

Copyright (c) 2026 dataStone Inc. Licensed under the MIT License.
See [LICENSE](LICENSE) file for details.

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history.
```

**doc/ Folder Organization**

The `doc/` folder SHOULD contain detailed documentation organized by topic. Recommended
structure (create files as needed for the extension):
- `installation.md` - Detailed setup, configuration, troubleshooting
- `user-guide.md` - Common tasks, usage patterns, examples
- `api-reference.md` - Complete reference for SQL functions, operators, types
- `development.md` - Contributor guide, build process, testing
- Additional topic-specific guides as needed

**Markdown Standards**

All markdown documentation MUST follow these conventions:
- Code blocks MUST specify language tags (```sql, ```c, ```cpp, ```bash)
- Use consistent heading hierarchy (# for title, ## for sections, ### for subsections)
- Tables MUST use GitHub-flavored markdown pipe format
- External links in reference style at document bottom where practical
- Include AI assistance caveat at document start when applicable

Example markdown structure:

```markdown
# User Guide

This guide was developed with assistance from AI tools.

## Basic Operations

Common tasks and usage patterns.

### Creating Custom Types

```sql
CREATE TYPE custom_type AS (
  id    INTEGER,
  name  TEXT
);
```

### Using Operators

The `~=` operator compares custom types:

```sql
SELECT ROW(1, 'test')::custom_type ~= ROW(1, 'test')::custom_type;
```

## Advanced Topics

Complex scenarios and edge cases.
```

**CHANGELOG.md Requirements**

The repository root MUST contain CHANGELOG.md following semantic versioning and
documenting all releases. Format:

```markdown
# Changelog

All notable changes to this project will be documented in this file.

This changelog was developed with assistance from AI tools.

The format is based on [Keep a Changelog](https://keepachangelog.com/),
and this project adheres to [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- Features in development

## [1.1.0] - 2025-12-13

### Added
- New process_custom() function
- Support for custom_type comparisons

### Changed
- Improved error messages in validation

### Fixed
- NULL handling in equality operator

## [1.0.0] - 2025-12-01

### Added
- Initial release
- Custom type definitions
- Basic operators
```

**Documentation Maintenance Requirements**

Documentation updates are MANDATORY for:
- New features MUST update relevant user documentation before PR merge
- Breaking changes MUST update migration guide and CHANGELOG
- API changes MUST update api-reference.md
- All code examples in documentation MUST be tested and verified working
- Version bumps MUST update CHANGELOG.md with changes categorized as Added,
  Changed, Deprecated, Removed, Fixed, Security

Documentation updates SHOULD be part of the same commit/PR as the code changes they
describe.

## Development Environment

Every extension project MUST include a LICENSE file at the repository root containing
the full MIT License text:

```
MIT License
Copyright (c) 2026 dataStone Inc.
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

Extensions MUST compile cleanly against PostgreSQL headers with NO warnings when using:
```bash
CFLAGS="-Wall -Wextra -Werror"
```

For client C++ code, MUST compile with C++14 standard and similar warnings:
```bash
CXXFLAGS="-std=c++14 -Wall -Wextra -Werror"
```

Required environment components:
- PostgreSQL 12+ development headers installed
- pg_config accessible in PATH
- C compiler supporting C99 minimum (for backend code)
- C++ compiler supporting C++14 (for client code)
- make utility
- pg_regress test framework

Backend memory management MUST use PostgreSQL's palloc/pfree family exclusively. Direct
malloc/free usage is FORBIDDEN in backend code. Client C++ code SHOULD use RAII and
smart pointers instead of manual memory management.

Backend error handling MUST use ereport/elog with appropriate error levels (ERROR,
WARNING, NOTICE). Never use printf/fprintf for error output in backend code. Client
C++ code MAY use exceptions or standard error reporting mechanisms.

## Code Review & Quality

All code submissions MUST:
- Pass regression tests (`make installcheck`)
- Compile without warnings (backend C and client C++ separately)
- Include test coverage for new functionality
- Update code documentation (doxygen comments) for public APIs
- Update user documentation (README, doc/, CHANGELOG) for user-facing changes
- Follow memory discipline (palloc/pfree for backend, RAII/smart pointers for client)

**Automated Compliance Checking**

The `pg-extension-review` skill provides automated constitution compliance checking.
Run before submitting code for review:

```bash
python scripts/review.py /path/to/extension
```

This generates a compliance report (`CONSTITUTION_REVIEW.md`) checking:
- Copyright and doxygen headers in all source files
- Backend C purity (no C++ in PostgreSQL extension code)
- C++14 standards in client code
- SQL keyword capitalization
- PGXS Makefile structure
- Documentation requirements (README, CHANGELOG, doc/ folder)

While automated checks catch many issues, manual review remains required for code
style nuances, naming conventions, test coverage adequacy, and documentation quality.

Backend performance considerations:
- Index-compatible operators MUST define appropriate support functions
- Expensive operations SHOULD check for interrupts (CHECK_FOR_INTERRUPTS())
- Large palloc allocations MUST be justified and reviewed
- Query planner integration MUST provide selectivity/cost estimates where applicable

Client C++ code quality:
- Resource management MUST use RAII (no naked new/delete)
- Prefer smart pointers over raw pointers for ownership
- Use standard library containers instead of manual memory management
- Templates and operator overloading MUST have clear justification
- Avoid template metaprogramming complexity

Security requirements:
- User input MUST be validated before use
- SQL injection vectors MUST be prevented (use SPI prepared statements)
- Privilege escalation paths MUST be analyzed and documented
- Resource exhaustion MUST be considered (memory, CPU, I/O)

## Governance

This constitution supersedes all other development practices and style guides for this
project. Amendments require:
1. Documented rationale for change
2. Review by project maintainers
3. Version bump according to semantic versioning:
   - MAJOR: Backward-incompatible principle changes
   - MINOR: New principle additions or material expansions
   - PATCH: Clarifications, wording fixes, non-semantic updates

All pull requests MUST verify compliance with these principles. Complexity beyond these
guidelines MUST be explicitly justified in PR descriptions. Deviations require
documented approval with clear reasoning.

Reviewers MUST verify:
- Code style adherence (K&R, naming, indentation)
- Backend code is pure C (no C++ in PostgreSQL extensions)
- Client C++ code uses C++14 standard with RAII and smart pointers
- Test coverage and pg_regress integration
- Code documentation completeness (doxygen comments)
- User documentation updates (README, doc/, CHANGELOG when applicable)
- Examples in documentation are tested and working
- PGXS Makefile correctness
- SQL keyword capitalization
- No template metaprogramming complexity in C++ code

**Version**: 1.0.0 | **Ratified**: 2025-12-13 | **Last Amended**: 2025-12-13