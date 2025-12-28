# Feature Specification: Direct Exact Comparison for Numeric and Integer Types

**Feature Branch**: `001-num-int-direct-comp`  
**Created**: 2025-12-23  
**Status**: Draft  
**Input**: User description: "Create a PostgreSQL extension pg_num2int_direct_comp that adds comparison operators for the inexact numeric types (numeric, decimal, float4, float8) with integral types (int2, int4, int8, serial, serial8, etc). The comparisons are exact, e.g., 16777216::bigint = 16777217::float should return false even though it is true with the builtin PostgreSQL cast and compare. The comparisons should allow btree index SARG predicate access, e.g., intcol = 10.0, should allow index only SARG. In v1.0, operators are not added to btree operator families, so transitive inference is not enabled (deferred to v2.0 where operators will be added to numeric_ops, float4_ops, float8_ops families to enable merge joins)."

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

### User Story 4 - Indexed Nested Loop Join Optimization (Priority: P1)

The extension operators are added to btree operator families (`numeric_ops` and `float_ops`) in v1.0, enabling PostgreSQL to use indexed nested loop joins for cross-type predicates. When joining tables with conditions like `int_table.col = numeric_table.col`, the query planner can use the btree index on the numeric column to efficiently look up matching rows.

**Note**: Operators are added ONLY to the higher-precision families (numeric_ops, float_ops), NOT to integer_ops, to avoid problematic transitive inference from the integer side. Merge joins are NOT supported because PostgreSQL's merge join algorithm requires operators in the same family on both sides of the join.

**Why this priority**: Indexed nested loop optimization is critical for join performance. Without btree family membership, cross-type joins would use hash joins that cannot utilize indexes on the joined columns.

**Independent Test**: Can be tested by creating indexed tables with int and numeric columns, running EXPLAIN on join queries, and verifying the plan shows "Index Only Scan" with "Index Cond" on both sides rather than Hash Join.

**Acceptance Scenarios**:

1. **Given** indexed tables with int4 and numeric columns, **When** executing `SELECT * FROM int_table i JOIN numeric_table n ON i.val = n.val WHERE i.val < 100`, **Then** query plan shows Nested Loop with Index Only Scan on both sides
2. **Given** the same join query, **When** examining the numeric table's index scan, **Then** the plan shows `Index Cond: (val = i.val)` proving cross-type index usage
3. **Given** query results with exact comparisons, **When** comparing result sets, **Then** results match PostgreSQL's native exact comparison semantics (10.5::numeric = 10::numeric returns false)
4. **Given** complex multi-table joins with mixed-type comparisons, **When** executing queries, **Then** correct rows are returned based on exact comparison semantics and indexes are utilized

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
- **Hash function consistency**: How should hash functions be implemented to support hash joins while maintaining the invariant "if a = b then hash(a) = hash(b)" across different types? *(Implemented in v1.0: Hash functions cast integers to the higher-precision type (numeric/float) before hashing, leveraging PostgreSQL's existing hash functions. This ensures equal values hash identically.)*

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

## Implemented in v1.0

- **Btree operator family membership**: Operators added to `numeric_ops` and `float_ops` families, enabling indexed nested loop joins
  - Provides major performance benefit for selective joins with indexes
  - Safe because operators only in higher-precision families (not in `integer_ops`)

- **Hash operator family membership**: Operators added to `numeric_ops` and `float_ops` hash families, enabling hash joins
  - Hash functions cast integers to the higher-precision type before hashing
  - Ensures `hash(10::int4) = hash(10.0::numeric)` for equal values
  - Leverages PostgreSQL's existing hash functions (`hash_numeric`, `hashfloat4`, `hashfloat8`)
  - Operators have HASHES property, allowing planner to choose hash joins for large table joins

# Future Enhancements (Not Required)

- **FE-001**: Cost estimation functions to help the query planner choose optimal join strategies


# Merge Join Support (Planned for v1.0.0)

- **Merge joins**: Not yet implemented, but planned for v1.0.0. This will require adding operators to the `integer_ops` family with careful handling to avoid invalid transitive inference (see research.md section 4 for rationale and implementation path).
  - **Constraint**: Adding to `integer_ops` must avoid planner inferring `int_col = 10.5` from `int_col = 10 AND int_col = numeric_col AND numeric_col = 10.5`.
  - **Current alternative**: Indexed nested loop joins provide similar or better performance for selective queries.
  - See research.md section 4.3 for implementation details and planned safeguards.

##
## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Queries using exact comparison operators correctly identify precision mismatches (test case: 16777216::bigint = 16777217::float4 returns false)
- **SC-002**: Queries with exact comparison predicates achieve sub-millisecond execution time on tables with 1 million rows (comparable to standard integer comparison performance)
- **SC-003**: Query plans for predicates like `WHERE intcol = 10.0::numeric` utilize available indexes rather than requiring full table scans
- **SC-004**: Feature works correctly on PostgreSQL 12, 13, 14, 15, and 16
- **SC-005**: All defined behaviors pass automated verification (100% test pass rate)
- **SC-006**: Cross-type join queries (e.g., `int_table.col = numeric_table.col`) use indexed nested loop joins with Index Cond on both sides (enabled by btree family membership)
- **SC-006b**: Queries with chained comparisons produce correct results following exact comparison semantics
- **SC-007**: User documentation includes at least 5 practical examples demonstrating exact comparison behavior and query optimization
- **SC-008**: Performance overhead of exact comparison operators is within 10% of standard comparison operators for equivalent queries
