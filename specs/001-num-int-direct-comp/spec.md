# Feature Specification: Direct Exact Comparison for Numeric and Integer Types

**Feature Branch**: `001-num-int-direct-comp`  
**Created**: 2025-12-23  
**Status**: Approved  
**Input**: User description: "Create a PostgreSQL extension pg_num2int_direct_comp that adds comparison operators for the inexact numeric types (numeric, decimal, float4, float8) with integral types (int2, int4, int8, serial, serial8, etc). The comparisons are exact, e.g., 16777216::bigint = 16777217::float should return false even though it is true with the builtin PostgreSQL cast and compare. The comparisons should allow btree index SARG predicate access, e.g., intkeycol = 10.0, should allow index only SARG.

## Clarifications

### Session 2025-12-28
- Q: Should the extension provide observability signals for debugging/monitoring cross-type comparisons? → A: None needed - rely on PostgreSQL's built-in EXPLAIN/pg_stat_statements

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Exact Equality Comparison Between Numeric Types (Priority: P1)

Database users need to compare values across all numeric type combinations (integers, decimals, and floating-point) without the precision loss inherent in PostgreSQL's default cast-then-compare behavior. The comparison must detect when values cannot be exactly represented across type boundaries.

**Why this priority**: This is the core value proposition of the extension. Without exact comparison operators, users cannot reliably detect data integrity issues where values have drifted for cross type comparisons. This is critical for financial calculations, scientific data validation, and audit trails across all numeric types.

**Independent Test**: Can be fully tested by creating tables with various numeric type columns, inserting values that differ only at precision boundaries, and verifying exact comparisons return mathematically correct (reflexive, transitive, symmetric, etc) results while PostgreSQL's default comparisons fail due to lossy implicit casting.

**Acceptance Scenarios**:

1. **Given** a float4 value 16777217.0 and a bigint value 16777216, **When** user executes `SELECT 16777216::bigint = 16777217::float4`, **Then** the extension operator returns false (while PostgreSQL's default returns true due to float precision loss)
2. **Given** a numeric value 10.0 and an int4 value 10, **When** user executes `SELECT 10::int4 = 10.0::numeric`, **Then** returns true (exact match with no fractional part)
3. **Given** a float8 value 10.5 and an int4 value 10, **When** user executes `SELECT 10::int4 = 10.5::float8`, **Then** returns false (fractional part prevents exact equality)
4. **Given** a numeric value 0.1 and a float8 value 0.1, **When** user executes `SELECT 0.1::numeric = 0.1::float8`, **Then** returns false (float8 cannot exactly represent 0.1)
5. **Given** NULL values on either side, **When** comparison is performed, **Then** returns NULL per SQL standard

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

### User Story 4 - Mathematically Valid Operators for Planner Inference (Priority: P1)

The extension ensures int × numeric operators satisfy all mathematical properties required for PostgreSQL's query planner to perform transitive inference: equality operators are symmetric, reflexive, and transitive; ordering operators are irreflexive and satisfy trichotomy. By adding operators to BOTH `integer_ops` and `numeric_ops` btree families with these guarantees, the planner can safely infer relationships (e.g., `a=c` from `a=b AND b=c`) and leverage native btree index access and merge joins without requiring custom support functions.

**Note**: Mixed type comparisons involving float types are excluded from this story due to inherent precision issues that would violate mathematical properties.

**Why this priority**: Mathematical validity enables PostgreSQL's optimizer to use advanced join strategies that depend on operator properties. The planner can make transitive inferences, use native btree index machinery, choose merge joins when both sides can be pre-sorted via btree indexes, and leverage indexed nested loop joins—all critical for complex queries with multiple join conditions.

**Independent Test**: Can be tested by creating indexed tables with int and numeric columns, executing queries with chained equality conditions, running EXPLAIN to verify the planner infers transitive relationships and uses merge joins or indexed nested loops, and confirming results are mathematically correct.

**Acceptance Scenarios**:

1. **Given** tables with int and numeric columns and query `WHERE int_col = 10 AND int_col = numeric_col AND numeric_col = 10`, **When** planner evaluates, **Then** correctly infers all three conditions are equivalent (transitivity holds)
2. **Given** reflexivity test, **When** executing `SELECT 10::int4 = 10::int4`, **Then** returns true (a = a)
3. **Given** symmetry test, **When** executing `SELECT (10::int4 = 10.0::numeric) = (10.0::numeric = 10::int4)`, **Then** both return same result (a = b ⟺ b = a)
4. **Given** transitivity test with exact values, **When** `10::int4 = 10.0::numeric` is true AND `10.0::numeric = 10::int8` is true, **Then** `10::int4 = 10::int8` is true (a = b AND b = c ⟹ a = c)
5. **Given** fractional value test, **When** `10::int4 = 10.5::numeric` is false, **Then** planner does not infer invalid transitive equality (no violation of mathematical properties)
6. **Given** indexed tables with merge join query, **When** executing `EXPLAIN SELECT * FROM int_table i JOIN numeric_table n ON i.val = n.val`, **Then** query plan shows Merge Join using native btree index access (no custom support function needed)
7. **Given** indexed nested loop join query, **When** executing `SELECT * FROM int_table i JOIN numeric_table n ON i.val = n.val WHERE i.val < 100`, **Then** query plan shows Nested Loop with Index Scan/Index Only Scan on both sides with `Index Cond: (val = i.val)` proving cross-type index usage
8. **Given** trichotomy for ordering, **When** comparing any two values, **Then** exactly one of `a < b`, `a = b`, or `a > b` is true
9. **Given** int × numeric operators in btree families, **When** querying `pg_amop` catalog for operator family membership, **Then** operators appear in BOTH `integer_ops` AND `numeric_ops` btree families (required for merge join from either side)

---

### User Story 5 - Index-Optimized Operators for Float Types (Priority: P2)

The extension provides exact comparison operators for int × float4/float8 combinations. Due to floating-point precision limitations, these operators cannot satisfy the strict mathematical properties required for planner transitive inference or merge join support. Instead, they use custom index support functions for index optimization, enabling indexed nested loop joins.

**Why this priority**: Completing int × float comparisons is important for comprehensive coverage of integer-to-floating-point scenarios, but float precision issues prevent these operators from achieving the same level of planner integration as int × numeric. These operators are still valuable for exact comparisons and index-optimized queries, just with different optimization characteristics.

**Independent Test**: Can be tested by creating indexed tables with int and float columns, running EXPLAIN on queries to verify custom support function provides index optimization, and verifying merge joins are NOT used (as expected).

**Acceptance Scenarios**:

1. **Given** indexed int and float tables, **When** executing `SELECT * FROM int_table WHERE int_col = 10.0::float4`, **Then** query uses index via custom support function transformation
2. **Given** int value 16777217 and float value 16777216.0, **When** comparing, **Then** returns false (detects float4 precision boundary)
3. **Given** indexed columns, **When** executing range queries like `int_col < 10.5::float8`, **Then** uses index with correct boundary handling via support function
4. **Given** operators NOT in both btree families, **When** attempting merge join, **Then** planner uses hash join or nested loop instead (expected behavior)

**Btree Family Integration Analysis**:

Our exact cross-type operators (int × float) DO satisfy mathematical transitivity when reasoning about actual stored values. If `a = b` and `b = c` both return TRUE with our exact semantics, then `a = c` is also TRUE — because our operators compare against the actual stored float value, not the user's original literal. Unlike PostgreSQL's native lossy cast-and-compare, our exact operators ensure only ONE integer value equals any given float value (e.g., `16777217::int4 = 16777216.0::float4` returns FALSE because 16777217 ≠ 16777216).

5. **Given** our exact int × float operators satisfy transitivity on stored values, **When** added to float_ops btree family, **Then** merge joins would be enabled; however, this is deferred to a future change to limit current scope and complexity
6. **Given** float precision limits where literals like `16777217.0::float4` are stored as `16777216.0`, **When** user writes `WHERE int_col = 16777217.0::float4`, **Then** query matches `int_col = 16777216` (the actual stored value); this is inherent to float representation, not a transitivity violation
7. **Given** NaN handling per PostgreSQL (NaN = NaN and NaN > all non-NaN), **When** operators follow this convention, **Then** mathematical properties are preserved for NaN comparisons

**Operator Coverage**:
- int2/int4/int8 × float4/float8: 6 type combinations × 6 comparison operators = **36 operators**

**Note**: These operators are NOT added to btree families in v1.0 (deferred to reduce scope). Index optimization is provided via support functions. Future versions may add btree family integration to enable merge joins.

---

### User Story 6 - Hash Join Support for Large Table Joins (Priority: P2)

The extension provides hash operator family support for all comparison operators (int × numeric and int × float), enabling PostgreSQL to use hash joins for large table joins. Hash joins are critical when neither table has useful indexes or when the join involves large datasets where hash join's memory-based approach outperforms other strategies.

**Why this priority**: Hash joins are essential for query performance on large datasets without selective predicates or useful indexes. Unlike merge joins (which require sorted input) or nested loop joins (which require indexes), hash joins work efficiently for any dataset size by building an in-memory hash table. The only requirement is the hash function invariant: if `a = b` then `hash(a) = hash(b)`.

**Independent Test**: Can be tested by creating large tables without indexes, executing join queries with `enable_nestloop=off` and `enable_mergejoin=off`, running EXPLAIN to verify hash join is selected, and confirming correct results.

**Acceptance Scenarios**:

1. **Given** large tables (1M+ rows) without indexes, **When** executing `SELECT * FROM int_table i JOIN numeric_table n ON i.val = n.val` with merge joins and nested loops disabled, **Then** query plan shows Hash Join
2. **Given** hash function implementation, **When** comparing equal values across types, **Then** `hash(10::int4) = hash(10.0::numeric)` and `hash(10::int4) = hash(10.0::float8)`
3. **Given** hash function for integers, **When** hashing any integer value, **Then** hash is computed by casting to the target type (numeric/float) and using PostgreSQL's native hash function (`hash_numeric`, `hashfloat4`, `hashfloat8`)
4. **Given** operators with HASHES property, **When** query planner evaluates join strategies, **Then** hash join is considered as a viable option alongside nested loop and merge join
5. **Given** non-selective join predicates, **When** executing cross-type joins on large tables, **Then** hash join provides comparable or better performance than nested loop joins

**Operator Coverage**:
- All 108 operators support hash joins via hash operator families
- int × numeric: Added to `integer_hash_ops` and `numeric_hash_ops`
- int × float: Added to `integer_hash_ops`, `float4_hash_ops`, and `float8_hash_ops`

**Implementation Notes**:
- Hash functions leverage PostgreSQL's existing hash implementations
- Integer values are cast to the higher-precision type before hashing
- This ensures hash consistency: equal values always produce the same hash
- No custom hash function implementation needed

---

### User Story 7 - Comprehensive Type Coverage for All Numeric/Integer Pairs (Priority: P3)

The extension provides exact comparison operators for all meaningful combinations of inexact numeric types (numeric/decimal, float4, float8) with all integer types (int2/smallint, int4/integer, int8/bigint, serial, bigserial, smallserial).

**Why this priority**: Completeness improves user experience by avoiding "why doesn't this type combination work?" questions, but users can work around missing combinations with explicit casts to supported types. This is important for polish but not blocking for core functionality.

**Independent Test**: Can be tested by generating a matrix of all type combinations, executing equality tests for each pair, and verifying all combinations compile and execute correctly.

**Acceptance Scenarios**:

1. **Given** all combinations of (numeric, float4, float8) × (int2, int4, int8), **When** testing equality operators, **Then** all combinations are supported (including both mathematically-valid int × numeric and float-involved operators)
2. **Given** serial types (serial, bigserial, smallserial), **When** using them in comparisons, **Then** they behave identically to their underlying integer types
3. **Given** the `decimal` type (alias for numeric), **When** using it in comparisons, **Then** it behaves identically to `numeric`

**Type Coverage Matrix**:
- **Mathematically-valid (User Story 4)**: int2/int4/int8 × numeric: 3 type combinations × 6 ops × 2 directions = 36 operators
- **Float-involved (User Story 5)**: int2/int4/int8 × float4/float8: 6 type combinations × 6 ops × 2 directions = 72 operators  
- **Hash join support (User Story 6)**: All 108 operators
- **Total**: 9 type combinations × 6 comparison operators × 2 directions = **108 operators**

---

### Edge Cases

- **Floating-point special values**: What happens when comparing integers with NaN, Infinity, or -Infinity? (This extension will follow the default PostgreSQL behavior, e.g., NaN is larger than any other number, including Infinity and equal to NaN)
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
- **FR-012**: int × numeric operators MUST be added to BOTH `integer_ops` and `numeric_ops` btree operator families to enable merge join optimization
- **FR-013**: int × numeric operators MUST NOT cause invalid transitive inference (e.g., planner must not infer `int_col = 10.5` from `int_col = 10 AND 10 = 10.5::numeric`)
- **FR-014**: int × float operators MUST support hash joins via hash operator families but MUST NOT be added to both btree operator families (to avoid precision-related transitivity issues)
- **FR-015**: Constant predicates with impossible equality conditions (e.g., `int_col = 10.5::numeric`) SHOULD be recognized as always-false during query planning, enabling optimal row estimates (rows=0) and potential scan elimination
- **FR-016**: Constant predicates with exact integer matches (e.g., `int_col = 100::numeric` where 100.0 has no fractional part) SHOULD be transformed to use native same-type operators for optimal selectivity estimation
- **FR-017**: Range predicates with fractional boundaries (e.g., `int_col > 10.5::numeric`) SHOULD be transformed to equivalent integer predicates with correct boundary handling (e.g., `int_col >= 11`)

## Implemented in v1.0

- **Merge join support for int × numeric**: Operators added to BOTH `integer_ops` and `numeric_ops` btree families
  - Enables PostgreSQL to use merge joins for int × numeric comparisons
  - Both sides can use btree indexes for pre-sorted input
  - Transitivity is preserved: operators correctly handle fractional values, preventing invalid inferences
  - Operators have MERGES property for merge join optimization

- **Index optimization for int × float**: Via `SupportRequestIndexCondition` support functions
  - Provides index scan optimization for predicates like `WHERE int_col = 10.0::float4`
  - NOT added to btree families (deferred to reduce scope per US5)
  - Indexed nested loop joins still work via support function index transformation

- **Hash operator family membership**: All operators added to appropriate hash families, enabling hash joins
  - Hash functions cast to the higher-precision type before hashing
  - Ensures `hash(10::int4) = hash(10.0::numeric)` for equal values
  - Leverages PostgreSQL's existing hash functions (`hash_numeric`, `hashfloat4`, `hashfloat8`)
  - Operators have HASHES property, allowing planner to choose hash joins for large table joins

- **Constant predicate optimization**: Transform predicates at query simplification time (before index planning)
  - `int_col = 10.5::numeric` → `FALSE` (impossible equality, rows=0 estimate)
  - `int_col = 100::numeric` → `int_col = 100` (use native operator, perfect selectivity)
  - `int_col > 10.5::numeric` → `int_col >= 11` (correct integer boundary)
  - `int_col < 10.5::numeric` → `int_col <= 10` (correct integer boundary)
  - Applies to constant predicates in WHERE clauses; join conditions use btree family membership
  - Rationale: When operators are in btree families, index condition transformation is bypassed; simplification at an earlier planning stage ensures optimal selectivity estimates while preserving join optimization

# Future Enhancements

- **Cost estimation functions**: Custom cost functions to help query planner choose optimal join strategies based on data distribution
- **Merge join support for int × float**: Currently not supported; would require operators in both integer_ops and float_ops simultaneously, creating precision boundary complexity
- **numeric × float comparisons**: May be added in future version if there's sufficient demand; would provide exact comparisons between numeric and float types similar to int × float approach

##
## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Queries using exact comparison operators correctly identify precision mismatches (test case: 16777216::bigint = 16777217::float4 returns false)
- **SC-002**: Queries with exact comparison predicates achieve sub-millisecond execution time on tables with 1 million rows (comparable to standard integer comparison performance)
- **SC-003**: Query plans for predicates like `WHERE intcol = 10.0::numeric` utilize available indexes rather than requiring full table scans
- **SC-004**: Feature works correctly on PostgreSQL 12, 13, 14, 15, and 16
- **SC-005**: All defined behaviors pass automated verification (100% test pass rate)
- **SC-006**: Cross-type join queries for int × numeric (e.g., `int_table.col = numeric_table.col`) use merge joins when appropriate, with both sides using index scans
- **SC-006b**: Cross-type join queries for int × float use indexed nested loop joins or hash joins (merge joins not supported)
- **SC-006c**: Queries with chained comparisons produce correct results following exact comparison semantics, with no invalid transitive inferences
- **SC-007**: User documentation includes at least 5 practical examples demonstrating exact comparison behavior and query optimization
- **SC-008**: Performance overhead of exact comparison operators is within 10% of standard comparison operators for equivalent queries
- **SC-009**: Query plans for impossible predicates (e.g., `WHERE int_col = 10.5::numeric`) show rows=0 estimate, indicating the planner recognizes the predicate as always-false
