# Feature Specification: Direct Exact Comparison for Numeric and Integer Types

**Feature Branch**: `001-num-int-direct-comp`  
**Created**: 2025-12-23  
**Status**: Draft  
**Input**: User description: "Create a PostgreSQL extension pg_num2int_direct_comp that adds comparison operators for the inexact numeric types (numeric, decimal, float4, float8) with integral types (int2, int4, int8, serial, serial8, etc). The comparisons are exact, e.g., 16777216::bigint = 16777217::float should return false even though it is true with the builtin PostgreSQL cast and compare. The comparisons should allow btree index SARG predicate access, e.g., intcol = 10.0, should allow index only SARG. The comparisons should not allow transitivity, e.g., floatcol = intcol AND intcol = 10.0 does not imply by transitivity that floatcol = 10.0."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Exact Equality Comparison Between Float and Integer (Priority: P1)

Database users need to compare floating-point values with integer values without the precision loss inherent in PostgreSQL's default cast-then-compare behavior. When comparing float4/float8 with int2/int4/int8, the comparison must detect when the floating-point value cannot exactly represent the integer value.

**Why this priority**: This is the core value proposition of the extension. Without exact comparison operators, users cannot reliably detect data integrity issues where floating-point values have drifted from their intended integer representations. This is critical for financial calculations, scientific data validation, and audit trails.

**Independent Test**: Can be fully tested by creating tables with float and integer columns, inserting values that differ only at precision boundaries (e.g., 16777216::bigint vs 16777217::float4), and verifying the exact comparison returns false while the standard PostgreSQL comparison would return true after lossy casting.

**Acceptance Scenarios**:

1. **Given** a float4 value 16777217.0 and a bigint value 16777216, **When** user executes `SELECT 16777216::bigint = 16777217::float4`, **Then** the extension operator returns false (while PostgreSQL's default returns true due to float precision loss)
2. **Given** a numeric value 10.0 and an int4 value 10, **When** user executes `SELECT 10::int4 = 10.0::numeric`, **Then** returns true (exact match with no fractional part)
3. **Given** a float8 value 10.5 and an int4 value 10, **When** user executes `SELECT 10::int4 = 10.5::float8`, **Then** returns false (fractional part prevents exact equality)
4. **Given** NULL values on either side, **When** comparison is performed, **Then** returns NULL per SQL standard

---

### User Story 2 - Index-Optimized Query Execution with SARG Predicates (Priority: P1)

Database administrators need queries using exact numeric-to-integer comparisons to utilize btree indexes efficiently. When querying `WHERE intcol = 10.0::numeric`, the query planner should recognize this as a Search ARGument (SARG) predicate that can use an index on `intcol` directly, without requiring an index scan on the entire table.

**Why this priority**: Without index support, exact comparisons would require full table scans on large tables, making the extension impractical for production use. Index optimization is essential for performance and directly affects the feature's usability in real-world databases with millions of rows.

**Independent Test**: Can be tested by creating a table with an indexed integer column, populating it with test data, running EXPLAIN ANALYZE on queries like `SELECT * FROM table WHERE intcol = 10.0`, and verifying the query plan shows "Index Only Scan" or "Index Scan" rather than "Seq Scan".

**Acceptance Scenarios**:

1. **Given** a table with a btree index on an int4 column, **When** user executes `EXPLAIN SELECT * FROM table WHERE intcol = 10.0::numeric`, **Then** query plan shows Index Scan or Index Only Scan
2. **Given** a table with 1 million rows and an indexed bigint column, **When** querying with exact float8 comparison, **Then** execution time is comparable to standard integer index lookup (sub-millisecond range)
3. **Given** multiple comparison predicates in a WHERE clause, **When** query optimizer evaluates the plan, **Then** the exact comparison operators are recognized as indexable predicates
4. **Given** an impossible equality condition like `int4col = 10.5::numeric`, **When** query is executed, **Then** returns empty result set efficiently using index (transformed to always-false condition)
5. **Given** all comparison operators (=, <>, <, <=, >, >=), **When** used with indexed columns, **Then** all operators utilize btree index with appropriate transformations

---

### User Story 3 - Range Comparisons (<, >, <=, >=) for Mixed Types (Priority: P2)

Users need to perform range queries comparing floating-point and integer values with exact semantics. Queries like `WHERE intcol > 10.5::float8` should return only integer values greater than 10 (since 10 < 10.5 and 11 > 10.5), not performing lossy casting that might include incorrect boundary values.

**Why this priority**: Range queries are essential for filtering data, but less critical than equality (P1) since users can work around missing range operators with explicit bounds. However, range operators complete the comparison operator family required for general-purpose querying.

**Independent Test**: Can be tested independently by executing range queries with boundary values that expose precision issues (e.g., `WHERE intcol < 16777217::float4`) and verifying correct boundary handling compared to PostgreSQL's default cast behavior.

**Acceptance Scenarios**:

1. **Given** integer values [10, 11, 12] and a float8 value 10.5, **When** executing `SELECT * FROM table WHERE intcol > 10.5::float8`, **Then** returns only [11, 12]
2. **Given** integer values around float precision boundaries, **When** executing range queries with float4 operands, **Then** boundary inclusion/exclusion is exact (no precision-related errors)
3. **Given** an indexed integer column, **When** executing range queries like `intcol <= 100.0::numeric`, **Then** query uses index range scan

---

### User Story 4 - Non-Transitive Comparison Semantics (Priority: P2)

The extension must prevent the query optimizer from incorrectly applying transitivity rules that would break exact comparison semantics. When a query contains `floatcol = intcol AND intcol = 10.0`, the optimizer must NOT infer `floatcol = 10.0` because float-to-int and int-to-numeric comparisons use different exact comparison logic.

**Why this priority**: Query optimizer correctness is critical for data integrity, but this is a defensive requirement (preventing incorrect optimization) rather than enabling new functionality. Most queries won't trigger transitive inference, so this affects correctness in edge cases rather than common usage.

**Independent Test**: Can be tested by constructing queries with chained comparisons involving multiple type combinations, examining EXPLAIN output to verify no transitive predicates are inferred, and running queries to confirm results match non-optimized execution.

**Acceptance Scenarios**:

1. **Given** a query `SELECT * FROM t WHERE floatcol = intcol AND intcol = 10.0`, **When** examining the query plan, **Then** the optimizer does not introduce an inferred predicate `floatcol = 10.0`
2. **Given** query results with and without optimizer hints disabling transitivity, **When** comparing result sets, **Then** both produce identical results
3. **Given** complex multi-table joins with mixed-type comparisons, **When** executing queries, **Then** no incorrect rows are returned due to transitive inference

---

### User Story 5 - Comprehensive Type Coverage for All Numeric/Integer Pairs (Priority: P3)

The extension provides exact comparison operators for all meaningful combinations of inexact numeric types (numeric/decimal, float4, float8) with all integer types (int2/smallint, int4/integer, int8/bigint, serial, bigserial, smallserial).

**Why this priority**: Completeness improves user experience by avoiding "why doesn't this type combination work?" questions, but users can work around missing combinations with explicit casts to supported types. This is important for polish but not blocking for core functionality.

**Independent Test**: Can be tested by generating a matrix of all type combinations, executing equality tests for each pair, and verifying all combinations compile and execute correctly.

**Acceptance Scenarios**:

1. **Given** all combinations of (numeric, float4, float8) × (int2, int4, int8), **When** testing equality operators, **Then** all 9 combinations are supported
2. **Given** serial types (serial, bigserial, smallserial), **When** using them in comparisons, **Then** they behave identically to their underlying integer types
3. **Given** the `decimal` type (alias for numeric), **When** using it in comparisons, **Then** it behaves identically to `numeric`

---

### Edge Cases

- **Floating-point special values**: What happens when comparing integers with NaN, Infinity, or -Infinity? (Standard: return false for equality, handle ordering per IEEE 754)
- **Numeric precision boundaries**: How does the system handle numeric values with scale/precision that exceeds integer range? (e.g., numeric(20,0) vs int4)
- **Overflow scenarios**: What happens when a numeric value exceeds the maximum representable integer value for the comparison type? (e.g., 9223372036854775808::numeric = int8_max)
- **Performance with large numeric values**: How efficiently does the extension handle numeric values with high precision (e.g., numeric(1000,500))? *(Performance with high-precision numeric is bounded by PostgreSQL's numeric type implementation. Extension adds minimal overhead (<10%) regardless of precision.)*
- **NULL handling consistency**: Are NULL semantics consistent across all operator types (=, <>, <, >, <=, >=)?
- **Type coercion ambiguity**: How does the system handle cases where type resolution might be ambiguous (e.g., literal `10.0` could be numeric or float8)? *(Note: Type resolution for literals is controlled by PostgreSQL's core type system, not the extension. Users can explicitly cast literals to desired types if needed.)*
- **Hash function consistency**: How should hash functions be implemented to support hash joins while maintaining the invariant "if a = b then hash(a) = hash(b)" across different types? *(Future Enhancement: Hash functions for cross-type equality are not yet implemented. Operators currently do not support HASHES property, so hash joins will fall back to nested loop joins. This can be added later without breaking compatibility.)*

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Extension MUST provide exact equality operator (=) for all combinations of (numeric, float4, float8) with (int2, int4, int8)
- **FR-002**: Extension MUST provide exact inequality operator (<>) for all combinations of inexact numeric types with integer types
- **FR-003**: Extension MUST provide exact comparison operators (<, >, <=, >=) for all combinations of inexact numeric types with integer types
- **FR-004**: Equality comparisons MUST return false when the floating-point value cannot exactly represent the integer value (e.g., 16777216::bigint = 16777217::float4 returns false)
- **FR-005**: Comparisons MUST preserve exact semantics: a numeric value with fractional part (e.g., 10.5) MUST NOT equal an integer value (10)
- **FR-006**: All operators MUST handle NULL values per SQL standard (NULL compared to any value returns NULL)
- **FR-007**: Operators MUST handle floating-point special values (NaN, Infinity, -Infinity) per IEEE 754 semantics
- **FR-008**: Extension MUST support type aliases (serial, bigserial, smallserial for integer types; decimal for numeric) identically to their underlying base types
- **FR-009**: Comparisons involving numeric values outside the representable range of the integer type MUST return false for equality. For ordering operators, values exceeding the integer type's maximum are treated as greater than any integer, and values below the minimum are treated as less than any integer (following PostgreSQL's numeric-to-integer cast semantics)
- **FR-010**: Feature MUST be compatible with PostgreSQL 12 and later versions
- **FR-011**: All comparison behaviors MUST be verifiable through automated tests covering normal cases, boundary conditions, NULL handling, and special values
# Future Enhancements (Not Required for v1.0)

- **FE-001**: Hash functions for cross-type equality operators to enable hash join optimization (currently operators do not have HASHES property, hash joins fall back to nested loop joins)
- **FE-002**: B-tree operator family registration to enable merge joins (deferred to v2.0)
  - **Rationale**: Operators are mathematically transitive and can be safely added to btree operator families (e.g., numeric_ops)
  - **Why Deferred**: Adds implementation complexity without immediate benefit since index optimization via support functions already provides excellent performance
  - **v2.0 Benefit**: Would enable merge join strategy for very large table joins where indexes aren't selective
- **FE-003**: Cost estimation functions to help the query planner choose optimal join strategies

##
## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Queries using exact comparison operators correctly identify precision mismatches (test case: 16777216::bigint = 16777217::float4 returns false)
- **SC-002**: Queries with exact comparison predicates achieve sub-millisecond execution time on tables with 1 million rows (comparable to standard integer comparison performance)
- **SC-003**: Query plans for predicates like `WHERE intcol = 10.0::numeric` utilize available indexes rather than requiring full table scans
- **SC-004**: Feature works correctly on PostgreSQL 12, 13, 14, 15, and 16
- **SC-005**: All defined behaviors pass automated verification (100% test pass rate)
- **SC-006**: Queries with chained comparisons (e.g., `floatcol = intcol AND intcol = 10.0`) produce correct results without incorrect transitive inference
- **SC-007**: User documentation includes at least 5 practical examples demonstrating exact comparison behavior and query optimization
- **SC-008**: Performance overhead of exact comparison operators is within 10% of standard comparison operators for equivalent queries
