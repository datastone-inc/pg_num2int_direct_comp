# Implementation Plan: Direct Exact Comparison for Numeric and Integer Types

**Branch**: `001-num-int-direct-comp` | **Date**: 2025-12-28 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-num-int-direct-comp/spec.md`

## Summary

PostgreSQL extension providing 108 exact comparison operators for cross-type comparisons between inexact numeric types (numeric, float4, float8) and integer types (int2, int4, int8). Operators use C body functions that reuse PostgreSQL's truncation/bounds primitives. Index optimization via `SupportRequestIndexCondition` support functions. Hash join support via operator family membership with casting-based hash functions. Merge join support for int×numeric via btree family membership; int×float btree family integration deferred to reduce scope.

## Technical Context

**Language/Version**: C (C99) for PostgreSQL backend extension  
**Primary Dependencies**: PostgreSQL 12+ development headers, PGXS build system  
**Storage**: N/A (operators only, no data storage)  
**Testing**: pg_regress framework (constitution requirement)  
**Target Platform**: Linux, macOS, Windows (via MinGW) - PostgreSQL 12-16  
**Project Type**: Single PostgreSQL extension  
**Performance Goals**: Sub-millisecond index lookups on 1M+ row tables, <10% overhead vs native comparisons  
**Constraints**: Must compile with `-Wall -Wextra -Werror`, no C++ in backend code  
**Scale/Scope**: 108 operators (9 type combinations × 6 ops × 2 directions)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Requirement | Status |
|-----------|-------------|--------|
| I. Code Style | K&R C, 2-space indent, camelCase functions | ✅ PASS |
| II. Documentation | Doxygen headers, @param/@return, AI caveat | ✅ PASS |
| III. Test-First | pg_regress tests before implementation | ✅ PASS |
| IV. SQL Standards | UPPERCASE keywords, snake_case identifiers | ✅ PASS |
| V. PGXS Build | Makefile with EXTENSION, DATA, MODULES, REGRESS | ✅ PASS |
| VI. User Docs | README, doc/ folder, CHANGELOG.md | ✅ PASS |
| Backend C Purity | No C++ in PostgreSQL extension code | ✅ PASS |
| Memory Discipline | palloc/pfree only, no malloc/free | ✅ PASS |
| Error Handling | ereport/elog, no printf | ✅ PASS |

**Gate Status**: ✅ All gates pass. Proceed to implementation.

## Project Structure

### Documentation (this feature)

```text
specs/001-num-int-direct-comp/
├── plan.md              # This file
├── research.md          # Phase 0 output (complete)
├── data-model.md        # Phase 1 output (complete)
├── quickstart.md        # Phase 1 output (complete)
├── contracts/           # Phase 1 output
│   ├── operators.md     # Operator definitions
│   └── test-cases.md    # Test case specifications
└── tasks.md             # Phase 2 output
```

### Source Code (repository root)

```text
pg_num2int_direct_comp.c        # Main C implementation (comparison functions)
pg_num2int_direct_comp.h        # Header with function declarations
pg_num2int_direct_comp--1.0.0.sql  # Extension SQL (operators, families)
pg_num2int_direct_comp.control  # Extension metadata
Makefile                        # PGXS build configuration

sql/                            # Regression test inputs
├── numeric_int_ops.sql         # numeric × int tests
├── float_int_ops.sql           # float × int tests
├── index_usage.sql             # Index optimization tests
├── hash_joins.sql              # Hash join tests
├── merge_joins.sql             # Merge join tests
├── null_handling.sql           # NULL semantics tests
├── special_values.sql          # NaN, Infinity tests
├── edge_cases.sql              # Boundary conditions
├── range_boundary.sql          # Range operator boundaries
├── transitivity.sql            # Transitivity verification
├── index_nested_loop.sql       # Indexed nested loop tests
└── performance.sql             # Performance benchmarks

expected/                       # Expected test outputs

doc/                            # User documentation
├── installation.md
├── user-guide.md
├── api-reference.md
└── development.md
```

**Structure Decision**: Single-project PostgreSQL extension. All C code in single file with header. PGXS standard layout.

## Key Technical Decisions (from research.md)

### 1. Exact Comparison Strategy
- Use PostgreSQL's `numeric_trunc()` and bounds checking primitives
- Truncate to integer part, check range, compare
- Return -1/0/1 from core comparison functions

### 2. Index Optimization
- Single generic `num2int_support()` function via `SupportRequestIndexCondition`
- Both operator directions need real C functions (shell operators cannot have support functions)
- Support function returns `List *` via `list_make1()`

### 3. Hash Join Support
- Cast integers to higher-precision type before hashing
- Use PostgreSQL's native hash functions (`hash_numeric`, `hashfloat8`)
- All equality operators have HASHES property

### 4. Btree Family Strategy
- Add int×numeric operators to BOTH `integer_ops` AND `numeric_ops` (enables merge joins)
- Add int×float operators to `float_ops` only (deferred: btree family integration)
- Do NOT add int×float to `integer_ops` (preserves transitivity correctness)

### 5. Operator Coverage
- 108 operators: 9 type combinations × 6 ops × 2 directions
- int×numeric: 36 operators with full btree/hash family integration
- int×float: 72 operators with hash family, btree deferred

## Phase Status

| Phase | Artifact | Status |
|-------|----------|--------|
| Phase 0 | research.md | ✅ Complete |
| Phase 1 | data-model.md | ✅ Complete |
| Phase 1 | contracts/operators.md | ✅ Complete |
| Phase 1 | contracts/test-cases.md | ✅ Complete |
| Phase 1 | quickstart.md | ✅ Complete |
| Phase 2 | tasks.md | ✅ Complete |

## Complexity Tracking

No constitution violations requiring justification. Implementation follows all principles.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| [e.g., 4th project] | [current need] | [why 3 projects insufficient] |
| [e.g., Repository pattern] | [specific problem] | [why direct DB access insufficient] |
