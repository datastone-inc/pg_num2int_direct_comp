# PostgreSQL Extension Development Constitution

> **Location**: `.specify/memory/constitution.md`
>
> For C++ client application repositories, use `constitution-cpp.md` instead.

## I. Code Style & Formatting

### C Code (PostgreSQL Backend Extensions)

C code for PostgreSQL backend extensions MUST follow K&R style with 2-space indentation.
Opening brace on same line as control statement, closing brace aligned with control
keyword. C++ is FORBIDDEN in PostgreSQL backend code.

Naming MUST use camelCase for functions/variables. PostgreSQL's conventional snake_case
is acceptable for public API functions integrating with existing PostgreSQL patterns.

```c
static char *
processData(const char *inputData, int maxLength) {
  char *result;
  int   len;

  if (inputData == NULL || maxLength <= 0) {
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("invalid input parameters")));
  }

  len = strlen(inputData);
  result = (char *)palloc(len + 1);
  memcpy(result, inputData, len);
  result[len] = '\0';

  return result;
}
```

### SQL Code

All SQL and PL/pgSQL MUST use UPPERCASE for keywords, lowercase for identifiers. Table
and column names MUST use snake_case. Statements MUST be formatted for readability with
aligned keywords.

```sql
CREATE OR REPLACE FUNCTION process_custom(input custom_type)
  RETURNS TEXT
  LANGUAGE plpgsql
  STABLE
AS $$
BEGIN
  IF input.id IS NULL THEN
    RAISE EXCEPTION 'id cannot be NULL';
  END IF;
  RETURN format('Processing: %s (ID: %s)', input.name, input.id);
END;
$$;
```

Extension SQL files MUST include:
- Copyright notice and SPDX identifier
- File header comment with filename and description
- psql execution guard (`\echo Use "CREATE EXTENSION..." \quit`)

## II. Documentation Standards

### File Headers

Every C and SQL source file MUST begin with copyright notice and doxygen-style header:

```c
/*
 * Copyright (c) 2026 dataStone Inc.
 * SPDX-License-Identifier: MIT
 */

/**
 * @file extension_utils.c
 * @brief Utility functions for extension data processing
 * @author Dave Sharpe
 * @date 2025-12-13
 * This file was developed with assistance from AI tools.
 */
```

The `@date` tag reflects original file creation; updates tracked via version control.

### Function Documentation

Function headers MUST document purpose, parameters (`@param`), and return values
(`@return`). For PG_FUNCTION_INFO_V1 functions, `@param` describes SQL-level arguments
(0, 1, etc.), not the C-level fcinfo parameter.

```c
/**
 * @brief Compare two custom types for equality
 * @param 0 First custom type value
 * @param 1 Second custom type value
 * @return Boolean true if equal
 */
PG_FUNCTION_INFO_V1(customtype_eq);
```

### Inline Comments

Inline comments MUST use C++ `//` style. Focus on explaining WHY decisions were made,
not WHAT the code does when self-evident. Avoid obvious comments.

## III. Test-First Development

TDD is MANDATORY using PostgreSQL's pg_regress framework. Development cycle:

1. Write regression test in `sql/` defining expected behavior
2. Create expected output in `expected/`
3. Verify test FAILS with current code
4. Implement feature
5. Verify test PASSES
6. Refactor with tests as safety net

Test coverage MUST include:
- Normal operation cases
- Boundary conditions
- NULL handling
- Error conditions with expected messages
- Index usage verification (EXPLAIN output)
- Extension lifecycle (CREATE/DROP cycles work without errors)

```sql
-- sql/custom_ops.sql
\pset format unaligned

SELECT 'basic_eq'::text AS test, customtype_eq('foo', 'foo');
SELECT 'null_lhs'::text AS test, customtype_eq(NULL, 'foo');
EXPLAIN (COSTS OFF) SELECT * FROM t WHERE col = 'target';
```

Extension lifecycle testing is MANDATORY. CREATE/ALTER EXTENSION ... UPDATE/DROP
EXTENSION cycles MUST work without errors. Upgrade paths MUST be tested for each version.

## IV. PGXS Build System

Extensions MUST use PostgreSQL's PGXS framework. Makefiles MUST define `EXTENSION`,
`DATA`, `MODULES`, and include `PGXS.mk`. Standard targets required: `all`, `install`,
`installcheck`, `clean`.

```makefile
EXTENSION = myextension
DATA = myextension--1.0.0.sql
MODULES = myextension
REGRESS = basic_ops edge_cases

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
```

Version upgrade scripts follow naming `extension--oldver--newver.sql`. Control files
MUST specify `default_version`, `module_pathname`, and `relocatable`.

Extension versions use semantic versioning (MAJOR.MINOR.PATCH):
- MAJOR: Incompatible API changes, breaking schema modifications
- MINOR: Backward-compatible functionality additions
- PATCH: Backward-compatible bug fixes

## V. Memory and Error Handling

Backend memory management MUST use PostgreSQL's palloc/pfree family exclusively.
Direct malloc/free is FORBIDDEN in backend code.

Error handling MUST use ereport/elog with appropriate levels (ERROR, WARNING, NOTICE).
Never use printf/fprintf for error output in backend code.

Extensions MUST compile cleanly with `-Wall -Wextra -Werror`.

## VI. Security

- User input MUST be validated before use
- SQL injection MUST be prevented (use SPI prepared statements)
- Privilege escalation paths MUST be analyzed and documented
- Resource exhaustion MUST be considered (memory, CPU, I/O)

## VII. Performance

- Index-compatible operators MUST define appropriate support functions
- Expensive operations SHOULD check for interrupts (CHECK_FOR_INTERRUPTS())
- Large palloc allocations MUST be justified
- Query planner integration MUST provide selectivity/cost estimates where applicable

## VIII. Project Structure

Every extension repository MUST include `project_template.md` which defines the repository structure.

See `project_template.md` for documentation templates and structure guidelines.

All SQL examples in user documentation MUST have corresponding regression tests in
`sql/doc_examples.sql` to ensure examples remain accurate as code evolves. Test
comments MUST reference the documentation location being validated:

```sql
-- README.md example: Basic usage (~line 45)
SELECT process_custom(ROW(1, 'test', NOW())::custom_type);
```

Automated compliance checking available via `pg-extension-review` skill.

Code submissions MUST pass all checks in `code_review_checklist.md` before merge.

## IX. Governance

This constitution supersedes all other development practices for this project.
Amendments require:

Deviations require documented approval with clear reasoning.

---

**Version**: 2.0.0 | **Ratified**: 2025-12-13 | **Last Amended**: 2026-01-07