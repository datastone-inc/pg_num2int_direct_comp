# Implementation Plan: Direct Exact Comparison for Numeric and Integer Types

**Branch**: `001-num-int-direct-comp` | **Date**: 2025-12-23 | **Spec**: [spec.md](spec.md)  
**Input**: Feature specification from `/specs/001-num-int-direct-comp/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Create PostgreSQL extension `pg_num2int_direct_comp` that provides exact comparison operators (=, <>, <, >, <=, >=) for inexact numeric types (numeric, float4, float8) with integer types (int2, int4, int8). Comparisons detect precision mismatches (e.g., 16777216::bigint = 16777217::float4 returns false) and support btree index SARG predicates for efficient query execution. Implementation uses C body functions with PostgreSQL truncation/bounds checking, SupportRequestIndexCondition for index integration (similar to LIKE operator), and lazy OID caching for operator lookups.

## Technical Context

**Language/Version**: C (C99 minimum) for PostgreSQL backend extension  
**Primary Dependencies**: PostgreSQL 12+ headers (fmgr.h, utils/numeric.h, utils/float.h, optimizer/optimizer.h)  
**Storage**: N/A (operators only, no data storage)  
**Testing**: PostgreSQL pg_regress framework  
**Target Platform**: PostgreSQL 12+ on Linux/macOS/Windows  
**Project Type**: Single PostgreSQL extension (PGXS build)  
**Performance Goals**: Sub-millisecond comparison on indexed columns, overhead <10% vs standard operators  
**Constraints**: Must integrate with PostgreSQL query optimizer, support btree index access paths, no transitivity inference  
**Scale/Scope**: 54 operators total (6 comparison types × 9 type combinations), 9 core comparison functions (`_cmp_internal` pattern) + 54 operator wrappers, 1 generic support function for index integration

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**✓ PASS**: Extension follows constitutional requirements:
- Pure C backend code (no C++), K&R style, 2-space indentation
- Test-first development with pg_regress framework mandatory
- PGXS build system for compilation and installation
- Documentation requirements (README, CHANGELOG, doc/ folder)
- Copyright notices and doxygen headers required
- SQL files use UPPERCASE keywords, lowercase identifiers
- Semantic versioning for releases (starting 1.0.0)

**No complexity violations**: Standard single-extension structure, no additional projects or architectural complexity beyond PostgreSQL operator definition patterns.

## Project Structure

### Documentation (this feature)

```text
specs/001-num-int-direct-comp/
├── spec.md              # Feature specification (user requirements)
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   ├── operators.yaml   # Operator definitions matrix
│   └── test-cases.yaml  # Test case specifications
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
pg-num2int-direct-comp/
├── pg_num2int_direct_comp.c           # Main C implementation file
├── pg_num2int_direct_comp.h           # Header with function declarations
├── pg_num2int_direct_comp--1.0.0.sql  # Extension installation script
├── pg_num2int_direct_comp.control     # Extension control file
├── Makefile                            # PGXS-based build configuration
├── LICENSE                             # MIT License
├── README.md                           # User-facing documentation
├── CHANGELOG.md                        # Version history
├── doc/                                # Detailed documentation
│   ├── installation.md                 # Setup and build instructions
│   ├── user-guide.md                   # Usage examples and patterns
│   ├── api-reference.md                # Complete operator reference
│   └── development.md                  # Contributor guide
├── sql/                                # pg_regress test SQL scripts
│   ├── numeric_int_ops.sql             # Tests for numeric vs int comparisons
│   ├── float_int_ops.sql               # Tests for float vs int comparisons
│   ├── index_usage.sql                 # EXPLAIN tests for index optimization
│   ├── null_handling.sql               # NULL behavior tests
│   ├── special_values.sql              # NaN, Infinity tests
│   └── edge_cases.sql                  # Boundary and overflow tests
└── expected/                           # Expected test outputs
    ├── numeric_int_ops.out
    ├── float_int_ops.out
    ├── index_usage.out
    ├── null_handling.out
    ├── special_values.out
    └── edge_cases.out
```

**Structure Decision**: Standard PostgreSQL extension layout using PGXS. Single C implementation file with header (following constitution requirement for simple projects). Test suite uses pg_regress with separate test files by functionality category. Documentation follows constitutional requirements with README, CHANGELOG, and doc/ folder structure.

## Phase Completion Status

### Phase 0: Research ✅ COMPLETE

**Output**: [research.md](research.md)

**Key Decisions**:
1. **Exact comparison strategy**: Reuse PostgreSQL's `numeric_trunc()` and bounds checking primitives
2. **Index integration**: Implement via `SupportRequestIndexCondition` (modeled after LIKE operator)
3. **Performance optimization**: Lazy per-backend OID cache for operator lookups
4. **Transitivity prevention**: Keep operators independent (not added to cross-type btree operator families)
5. **Test organization**: pg_regress with categorized test suites (operators, index, NULL, special values, edges)
6. **Type coverage**: Full 9-combination matrix with type-specific precision handling

All unknowns from Technical Context resolved. No [NEEDS CLARIFICATION] markers remain.

### Phase 1: Design ✅ COMPLETE

**Outputs**:
- [data-model.md](data-model.md) - Operator catalog structures, function signatures, type combination matrix
- [contracts/operators.md](contracts/operators.md) - Complete 54-operator specification with properties
- [contracts/test-cases.md](contracts/test-cases.md) - 7 test categories with ~200+ test assertions
- [quickstart.md](quickstart.md) - Developer workflow guide and quick reference

**Design Highlights**:
- 54 operators (6 types × 9 combinations) with commutator/negator relationships
- 9 core `_cmp_internal` functions (PostgreSQL standard pattern returning -1/0/1)
- 54 operator wrapper functions (thin wrappers calling core cmp functions)
- 1 generic support function (`num2int_support`) handling all operators via OID inspection
- Lazy OID cache structure for per-backend caching
- Comprehensive test strategy covering correctness, index usage, NULL handling, special values, edge cases, transitivity, and performance

**Constitution Re-Check**: ✅ PASS
- Design maintains pure C backend code requirement
- Test-first approach with pg_regress framework defined
- PGXS build system structure confirmed
- Documentation structure aligns with constitution (README, CHANGELOG, doc/)
- No architectural complexity violations introduced

### Phase 2: Agent Context Update ✅ COMPLETE

**Updated File**: [.github/agents/copilot-instructions.md](../../.github/agents/copilot-instructions.md)

**Context Added**:
- Language: C (C99 minimum) for PostgreSQL backend extension
- Framework: PostgreSQL 12+ headers (fmgr.h, utils/numeric.h, utils/float.h, optimizer/optimizer.h)
- Database: N/A (operators only, no data storage)
- Project Type: Single PostgreSQL extension (PGXS build)

Agent context now reflects the technology stack for AI-assisted development.

## Next Steps

Planning phase complete. Proceed to implementation with:

```bash
/speckit.tasks
```

This will generate the task breakdown in `tasks.md` for execution via `/speckit.implement`.
