# Feature Specification: Float-Integer Btree Operator Family Integration

**Feature Branch**: `002-float-btree-ops`
**Created**: 2026-01-11
**Status**: Draft
**Input**: User description: "Add int x float operators to integer_ops and float_ops btree families to enable merge joins (FE-002 and FE-003 from v1.0 spec)"

## Clarifications

### Session 2026-01-11
- Q: How should DROP EXTENSION cleanup handle pg_amop entries added to builtin families? → A: Reuse existing event trigger cleanup (v1.0 already removes pg_amop entries for int×numeric; extend to cover int×float)
- Q: Should the 20% documentation reduction target apply to line count or word count? → A: Word count (measure actual content reduction)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Merge Joins for Integer x Float Columns (Priority: P1)

Database administrators need merge join optimization for queries joining tables on integer and float columns. Currently, int x float operators only support indexed nested loop joins and hash joins because they are not registered in btree operator families. Adding these operators to both `integer_ops` and `float_ops` btree families enables the query planner to choose merge joins when both sides have pre-sorted data available.

**Why this priority**: Merge joins are the most efficient join strategy when both inputs are already sorted (e.g., via btree index scans). For large tables with appropriate indexes, merge joins avoid building hash tables (hash join) or repeated index lookups (nested loop). This directly addresses FE-002 from v1.0 spec.

**Independent Test**: Can be tested by creating indexed tables with integer and float columns, executing join queries with `enable_hashjoin=off` and `enable_nestloop=off`, running EXPLAIN to verify merge join is selected, and confirming results are mathematically correct.

**Acceptance Scenarios**:

1. **Given** indexed integer and float4 tables with 100K+ rows, **When** executing `SELECT * FROM int_table i JOIN float4_table f ON i.val = f.val` with hash join and nested loop disabled, **Then** query plan shows Merge Join
2. **Given** indexed integer and float8 tables, **When** executing merge join query, **Then** both sides show Index Scan for pre-sorted input
3. **Given** cross-type merge join query, **When** EXPLAIN ANALYZE is run, **Then** execution time is comparable to or better than hash join for sorted data
4. **Given** equality operators for int x float4/float8, **When** querying `pg_amop` catalog, **Then** operators appear in BOTH `integer_ops` AND `float4_ops`/`float8_ops` btree families

---

### User Story 2 - Mathematical Transitivity Verification for Float Types (Priority: P1)

The extension operators satisfy transitivity for stored values: if `a = b` AND `b = c` both return TRUE, then `a = c` is TRUE. This holds because our exact comparison operators compare against actual stored float values, not user literals. Unlike PostgreSQL's lossy cast-and-compare, only ONE integer value equals any given stored float value.

**Why this priority**: Btree family membership requires operators to satisfy transitivity for PostgreSQL's query planner to safely make transitive inferences. This story validates that our exact operators meet this requirement for stored values, justifying their inclusion in btree families.

**Independent Test**: Can be tested by creating chained equality conditions, verifying planner transitive inference, and proving that exact comparison semantics preserve transitivity for any stored float value.

**Acceptance Scenarios**:

1. **Given** stored float4 value `16777216.0::float4`, **When** testing transitivity with integers, **Then** exactly one integer (16777216) equals this float, and chained equalities are consistent
2. **Given** NaN handling per PostgreSQL semantics, **When** comparing `NaN = NaN` and ordering `NaN > all non-NaN`, **Then** trichotomy and transitivity are preserved for NaN values
3. **Given** +/-Infinity values, **When** comparing with integers, **Then** ordering is consistent: `-Inf < all integers < +Inf`
4. **Given** query with chained conditions `int_col = float_val AND float_val = other_int`, **When** planner evaluates with btree family membership, **Then** transitive inference is valid and results are mathematically correct

---

### User Story 3 - Documentation Clarity and Persuasiveness (Priority: P2)

Users evaluating this extension need clear, concise documentation that addresses the "non-standard" nature of custom comparison operators. The documentation acknowledges this concern directly while demonstrating that the extension provides correct and performant behavior that PostgreSQL's default operators cannot achieve.

**Why this priority**: Documentation quality affects adoption. Addressing concerns proactively builds trust and helps users understand when and why to use the extension.

**Independent Test**: Documentation can be reviewed for conciseness, persuasive framing, and accuracy. SQL examples must have corresponding regression tests per constitution.md.

**Acceptance Scenarios**:

1. **Given** the README and doc files, **When** reviewing for length, **Then** content is reduced by at least 20% while preserving essential information
2. **Given** the "non-standard" concern, **When** reading documentation, **Then** the concern is addressed directly with explanation of why custom operators are necessary and how they are implemented correctly
3. **Given** SQL examples in documentation, **When** running doc-example-reviewer, **Then** all examples have corresponding regression tests in sql/doc_examples.sql
4. **Given** updated doc examples, **When** running doc-example-tester, **Then** all tests pass and output matches documented results

---

### Edge Cases

- **Float precision at boundary values**: float4 loses precision at 2^24, float8 at 2^53 - integer comparisons at these boundaries must be exact
- **NaN semantics**: PostgreSQL treats NaN = NaN as TRUE and NaN > all non-NaN values; operators must follow this
- **+/-Infinity handling**: -Infinity < all integers < +Infinity ordering must be maintained
- **Subnormal floats**: Very small float values near zero should compare correctly with integer 0
- **User literal confusion**: Users may expect `WHERE int_col = 16777217.0::float4` to match 16777217, but the literal rounds to 16777216.0 before comparison; documentation must clarify this is float representation, not extension behavior
- **Float precision loss is permanent**: Once a value is stored or passed as float, precision loss is irreversible. Float parameters lose precision at bind time. Use integer or numeric parameter types to preserve precision.
- **Equivalence expressions**: Users migrating to/from the extension need equivalent expressions: with extension use `int8_col::float8 = float4_col::float8` for stock behavior (both sides cast to float8); without extension use `int_col::numeric = float_col::numeric` for exact behavior (no index support).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Int x float4 comparison operators (=, <>, <, >, <=, >=) MUST be added to `integer_ops` btree family
- **FR-002**: Int x float4 comparison operators MUST be added to `float4_ops` btree family
- **FR-003**: Int x float8 comparison operators MUST be added to `integer_ops` btree family
- **FR-004**: Int x float8 comparison operators MUST be added to `float8_ops` btree family
- **FR-005**: All operators MUST have appropriate btree strategy numbers (1=<, 2=<=, 3==, 4=>=, 5=>)
- **FR-006**: Comparison support functions (FUNCTION 1) MUST be provided for each type pair
- **FR-007**: Builtin same-type float operators MUST be added to `integer_ops` to enable sorting within family context
- **FR-008**: Equality operators MUST have MERGES property to enable merge join optimization
- **FR-009**: Documentation MUST be shortened by at least 20% word count while maintaining accuracy
- **FR-010**: Documentation MUST directly address the "non-standard operators" concern with explanation of correctness guarantees
- **FR-011**: All SQL examples in user documentation MUST have corresponding tests in sql/doc_examples.sql per constitution.md
- **FR-012**: Documentation SQL examples MUST follow constitution.md formatting (UPPERCASE keywords, lowercase identifiers)
- **FR-013**: Documentation MUST include equivalence expressions: (a) with extension, emulate stock via `int8_col::float8 = float4_col::float8`; (b) without extension, emulate exact via `int_col::numeric = float_col::numeric`
- **FR-014**: Extension MUST provide GUC parameter `pg_num2int_direct_comp.enable_support_functions` (default: true) to disable SupportRequestSimplify and SupportRequestIndexCondition optimizations for testing/troubleshooting

### Assumptions

- PostgreSQL 12+ compatibility continues (btree family registration API stable)
- Extension remains at version 1.0.0; no upgrade script provided
- Test databases require manual DROP/CREATE EXTENSION to gain these changes
- Float comparison functions already implement exact semantics; only btree family registration is new
- Hash join support for int x float already exists; this change adds merge join capability

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Merge join queries between integer and float columns complete successfully with correct results
- **SC-002**: EXPLAIN output for cross-type joins shows "Merge Join" when hash join and nested loop are disabled
- **SC-003**: Query planner can perform transitive inference for chained int-float-int conditions (verified via EXPLAIN output showing predicate simplification or index usage)
- **SC-004**: All 130 btree family entries added (70 in integer_ops + 30 in float4_ops + 30 in float8_ops) verified via pg_amop query
- **SC-005**: Documentation word count reduced by at least 20% from v1.0.0
- **SC-006**: doc-example-reviewer reports 100% compliance (all examples have tests)
- **SC-007**: DROP EXTENSION cleanly removes all added pg_amop entries
- **SC-008**: GUC `pg_num2int_direct_comp.enable_support_functions` can be set to off/on to disable/enable query optimizations
