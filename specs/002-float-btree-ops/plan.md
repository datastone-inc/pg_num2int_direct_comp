# Implementation Plan: Float-Integer Btree Operator Family Integration

**Branch**: `002-float-btree-ops` | **Date**: 2026-01-11 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/002-float-btree-ops/spec.md`

## Summary

Add int x float4/float8 comparison operators to btree operator families (`integer_ops`, `float4_ops`, `float8_ops`) to enable merge joins for cross-type joins. Extend existing event trigger cleanup to cover new btree family entries. Reduce documentation word count by 20%+ while addressing "non-standard operators" concern directly.

## Technical Context

**Language/Version**: C (PostgreSQL 12+), SQL (PL/pgSQL)
**Primary Dependencies**: PostgreSQL PGXS, existing pg_num2int_direct_comp v1.0.0 codebase
**Storage**: PostgreSQL system catalogs (pg_amop, pg_amproc)
**Testing**: pg_regress framework, sql/doc_examples.sql for documentation
**Target Platform**: PostgreSQL 12, 13, 14, 15, 16 on Linux/macOS
**Project Type**: PostgreSQL extension (single module)
**Performance Goals**: Merge join performance comparable to native same-type joins
**Constraints**: No new C code required (reuse existing comparison functions); SQL-only changes
**Scale/Scope**: 72 new btree family entries (6 operators x 6 type pairs x 2 families)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Rule | Status | Notes |
|------|--------|-------|
| I. C Code Style (K&R, 2-space) | N/A | No new C code—SQL only |
| I. SQL Style (UPPERCASE keywords) | REQUIRED | All ALTER OPERATOR FAMILY statements |
| II. File Headers | REQUIRED | Update copyright year if modifying files |
| III. Test-First Development | REQUIRED | Write regression tests before implementation |
| III. Extension Lifecycle Testing | REQUIRED | DROP/CREATE cycle must work |
| IV. PGXS Build System | PASS | Existing Makefile sufficient |
| VII. Performance (support functions) | REQUIRED | Must provide btree comparison support functions |
| VIII. Documentation Examples | REQUIRED | All SQL examples need sql/doc_examples.sql tests |

**Pre-Design Gate**: ✅ PASS (no violations)

## Project Structure

### Documentation (this feature)

```text
specs/002-float-btree-ops/
├── plan.md              # This file
├── research.md          # Phase 0: Technical decisions
├── data-model.md        # Phase 1: Operator family schema
├── quickstart.md        # Phase 1: Developer quick reference
├── contracts/           # Phase 1: API contracts
│   └── operators.md     # Operator definitions
└── tasks.md             # Phase 2: Implementation tasks (created by /speckit.tasks)
```

### Source Code (repository root)

```text
pg_num2int_direct_comp/
├── pg_num2int_direct_comp--1.0.0.sql  # Modified: Add btree family entries
├── pg_num2int_direct_comp.c           # Unchanged (reuse existing cmp functions)
├── pg_num2int_direct_comp.h           # Unchanged
├── sql/
│   ├── float_int_ops.sql              # New test: float x int btree families
│   ├── merge_joins.sql                # Existing: extend for float x int
│   └── doc_examples.sql               # Modified: add doc example tests
├── expected/
│   ├── float_int_ops.out              # New expected output
│   └── merge_joins.out                # Modified expected output
├── README.md                          # Modified: shortened, persuasive
├── doc/
│   ├── api-reference.md               # Modified: add float btree info
│   └── operator-reference.md          # Modified: update operator tables
└── Makefile                           # Modified: add new test files
```

## Complexity Tracking

> No constitution violations requiring justification.

| Item | Decision | Rationale |
|------|----------|-----------|
| Reuse existing cmp functions | Yes | float4_cmp_int2, etc. already exist and implement exact semantics |
| Event trigger extension | Extend existing | One cleanup function handles all operator families |
| Same-type float operators in integer_ops | Required | Merge join needs sorting within family context |

---

## Phase 0: Research

### Decision 1: Btree Family Registration Pattern

**Question**: How to register int x float operators in btree families?

**Decision**: Follow the existing int x numeric pattern from v1.0.0:
1. Add cross-type operators with appropriate strategy numbers (1-5)
2. Add comparison support functions (FUNCTION 1) for each type pair
3. Add same-type float operators to integer_ops for sort consistency
4. Extend event trigger cleanup array

**Rationale**: Proven pattern already working for int x numeric. Minimal risk.

**Alternatives Rejected**:
- Custom operator class: Unnecessary complexity; builtin families sufficient
- Dynamic tracking: Hardcoded list is auditable and tested by lifecycle tests

### Decision 2: Documentation Reduction Strategy

**Question**: How to achieve 20% word count reduction while improving persuasiveness?

**Decision**: 
1. Remove redundant explanations (say it once, not three ways)
2. Convert verbose prose to tables/lists where appropriate
3. Add direct "Why Custom Operators?" section addressing concerns upfront
4. Move detailed technical background to doc/development.md
5. Keep README focused on quick start and value proposition

**Baseline**: 11,234 words total (README.md + doc/*.md)
**Target**: ≤8,987 words (20% reduction)

**Rationale**: Users scanning documentation want quick answers. Persuasive framing upfront, details for those who want them.

### Decision 3: Comparison Function Reuse

**Question**: Are new C comparison functions needed for btree family support?

**Decision**: No. Existing functions can be reused:
- `float4_cmp_int2`, `float4_cmp_int4`, `float4_cmp_int8`
- `float8_cmp_int2`, `float8_cmp_int4`, `float8_cmp_int8`
- `int2_cmp_float4`, `int4_cmp_float4`, `int8_cmp_float4`
- `int2_cmp_float8`, `int4_cmp_float8`, `int8_cmp_float8`

**Rationale**: These functions already exist and implement exact comparison semantics. They return -1/0/+1 as required by btree.

### Decision 4: Cross-Type Comparison Equivalence Expressions

**Question**: How can users switch between extension and stock PostgreSQL comparison semantics?

**Decision**: Document equivalence expressions for int8 vs float4/float8 comparisons:

**With extension → emulate stock PostgreSQL** (both sides cast to float8):
```sql
int8_col::float8 = float4_col::float8  -- explicit cast bypasses extension operator
```

**Without extension → emulate exact comparison**:
```sql
int8_col::numeric = float4_col::numeric  -- both sides exact, no index support
```

**Summary table**:
| Scenario | With Extension | Stock PG (both → float8) |
|----------|----------------|--------------------------|
| int8 = float4 | `int8_col = float4_col` | `int8_col::float8 = float4_col::float8` |
| Exact w/o ext | `int8_col::numeric = float4_col::numeric` | N/A |

**Rationale**: Users migrating to/from the extension need clear guidance. Stock PostgreSQL casts int vs float4 to float8 (common supertype). The numeric cast approach trades index support for correctness.

---

## Phase 1: Design

### Data Model

See [data-model.md](data-model.md) for operator family schema.

Key entities:
- **pg_amop entries**: 72 operator registrations (6 ops x 6 type pairs x 2 families)
- **pg_amproc entries**: 12 support function registrations (6 type pairs x 2 families)
- **Same-type float operators in integer_ops**: 10 operators (float4 x 5 + float8 x 5)

### Contracts

See [contracts/operators.md](contracts/operators.md) for operator definitions.

### Quick Start

See [quickstart.md](quickstart.md) for developer quick reference.

---

## Phase 2: Implementation Scope

*Tasks will be generated by `/speckit.tasks` command.*

### Work Packages

1. **SQL Implementation** (pg_num2int_direct_comp--1.0.0.sql)
   - Add ALTER OPERATOR FAMILY statements for float4_ops btree
   - Add ALTER OPERATOR FAMILY statements for float8_ops btree
   - Add float operators to integer_ops btree (for sort consistency)
   - Extend event trigger cleanup array

2. **Regression Tests** (sql/, expected/)
   - float_int_ops.sql: btree family verification
   - merge_joins.sql: extend for float x int merge join tests
   - Verify DROP/CREATE extension cycle

3. **Documentation Updates**
   - README.md: reduce word count, add persuasive framing
   - doc/api-reference.md: add float btree info
   - doc/operator-reference.md: update tables
   - sql/doc_examples.sql: add tests for all doc examples

4. **Validation**
   - Run doc-example-reviewer to verify 100% coverage
   - Measure final word count (target: ≤8,987 words)
   - Run full regression suite

---

## Constitution Check (Post-Design)

| Rule | Status | Notes |
|------|--------|-------|
| I. SQL Style (UPPERCASE keywords) | ✅ PASS | All SQL follows convention |
| III. Test-First Development | ✅ PASS | Tests defined before implementation |
| III. Extension Lifecycle Testing | ✅ PASS | DROP/CREATE cycle covered |
| VII. Performance (support functions) | ✅ PASS | Reusing existing cmp functions |
| VIII. Documentation Examples | ✅ PASS | doc_examples.sql tests planned |

**Post-Design Gate**: ✅ PASS

---

## Next Steps

Run `/speckit.tasks` to generate implementation task breakdown.
