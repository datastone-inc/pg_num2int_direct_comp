# Test Cases: pg_num2int_direct_comp

## Test Organization

Tests are organized by functionality in separate SQL files using the pg_regress framework.

## Test Categories

### 1. Operator Correctness Tests (`sql/numeric_int_ops.sql`, `sql/float_int_ops.sql`)

**Purpose**: Verify all comparison operators return correct results for basic cases

**Coverage**:
- All 54 operators with representative values
- Commutative property verification (both operand orders)
- Negation property verification (= vs <>, < vs >=, etc.)

**Example Test Cases**:

```sql
-- Exact equality: numeric with no fractional part equals integer
SELECT 10::numeric = 10::int4;  -- Expected: t

-- Fractional part prevents equality
SELECT 10.5::numeric = 10::int4;  -- Expected: f

-- Precision boundary (float4 mantissa limit at 2^24)
SELECT 16777216::bigint = 16777217::float4;  -- Expected: f (precision loss in float4)
SELECT 16777216::bigint = 16777216::float4;  -- Expected: t (exact representation)

-- Ordering operators
SELECT 10.5::numeric > 10::int4;  -- Expected: t
SELECT 10::int4 < 10.5::numeric;  -- Expected: t (commutative check)
SELECT 9.9::numeric <= 10::int4;  -- Expected: t
```

**Success Criteria**: All operators return expected boolean values for representative test values

---

### 2. Index Usage Tests (`sql/index_usage.sql`)

**Purpose**: Verify query planner uses indexes for predicates with exact comparison operators

**Setup**:
```sql
CREATE TABLE test_int4 (id SERIAL, value INT4);
CREATE INDEX idx_value ON test_int4(value);
INSERT INTO test_int4 (value) SELECT generate_series(1, 10000);
ANALYZE test_int4;
```

**Test Cases**:

```sql
-- Should use Index Scan or Index Only Scan
EXPLAIN (COSTS OFF) SELECT * FROM test_int4 WHERE value = 500::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM test_int4 WHERE value = 500.0::float8;

-- Should use Index Scan (not Seq Scan)
EXPLAIN (COSTS OFF) SELECT * FROM test_int4 WHERE value < 1000::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM test_int4 WHERE value >= 5000::float4;

-- Should NOT use index (fractional constant can't match integer)
EXPLAIN (COSTS OFF) SELECT * FROM test_int4 WHERE value = 500.5::numeric;
```

**Success Criteria**:
- Query plans show "Index Scan" or "Index Only Scan" for applicable predicates
- No "Seq Scan" when index is available and predicate is selective
- Execution time comparable to native integer index queries (<1ms for selective predicates)

---

### 2a. Hash Join Tests (`sql/hash_joins.sql`)

**Purpose**: Verify HASHES property enables hash joins for equality operators

**Setup**:
```sql
CREATE TABLE numeric_table (id SERIAL, num_val NUMERIC);
CREATE TABLE int_table (id SERIAL, int_val INT4);
INSERT INTO numeric_table (num_val) SELECT generate_series(1, 1000);
INSERT INTO int_table (int_val) SELECT generate_series(1, 1000);
ANALYZE numeric_table;
ANALYZE int_table;

-- Force hash join for testing
SET enable_nestloop = off;
SET enable_mergejoin = off;
```

**Test Cases**:

```sql
-- Should use Hash Join with exact comparison
EXPLAIN (COSTS OFF) 
SELECT * FROM numeric_table n 
JOIN int_table i ON n.num_val = i.int_val;

-- Verify hash join with float types
EXPLAIN (COSTS OFF)
SELECT * FROM numeric_table n
JOIN int_table i ON n.num_val::float8 = i.int_val;

-- Hash aggregate should work
EXPLAIN (COSTS OFF)
SELECT num_val, COUNT(*) 
FROM numeric_table n 
JOIN int_table i ON n.num_val = i.int_val
GROUP BY num_val;
```

**Success Criteria**:
- Query plans show "Hash Join" for equality predicates
- Hash aggregation uses HashAggregate node
- Join results are correct (matching rows only)
- Performance comparable to native integer hash joins

---

### 2b. Merge Join Tests (`sql/merge_joins.sql`)

**Purpose**: Verify MERGES property enables merge joins for ordering operators

**Setup**:
```sql
CREATE TABLE numeric_sorted (id SERIAL, num_val NUMERIC);
CREATE TABLE int_sorted (id SERIAL, int_val INT4);
INSERT INTO numeric_sorted (num_val) SELECT generate_series(1, 1000);
INSERT INTO int_sorted (int_val) SELECT generate_series(1, 1000);
CREATE INDEX idx_num_sorted ON numeric_sorted(num_val);
CREATE INDEX idx_int_sorted ON int_sorted(int_val);
ANALYZE numeric_sorted;
ANALYZE int_sorted;

-- Force merge join for testing
SET enable_nestloop = off;
SET enable_hashjoin = off;
```

**Test Cases**:

```sql
-- Should use Merge Join with < operator
EXPLAIN (COSTS OFF)
SELECT * FROM numeric_sorted n
JOIN int_sorted i ON n.num_val < i.int_val;

-- Should use Merge Join with >= operator
EXPLAIN (COSTS OFF)
SELECT * FROM numeric_sorted n
JOIN int_sorted i ON n.num_val >= i.int_val;

-- Mixed ordering operators
EXPLAIN (COSTS OFF)
SELECT * FROM numeric_sorted n
JOIN int_sorted i ON n.num_val <= i.int_val
WHERE n.num_val > 100::int4;
```

**Success Criteria**:
- Query plans show "Merge Join" for ordering predicates (<, >, <=, >=)
- Join results are correct
- Performance comparable to native integer merge joins

---

### 3. NULL Handling Tests (`sql/null_handling.sql`)

**Purpose**: Verify NULL semantics follow SQL standard (NULL compared to anything returns NULL)

**Test Cases**:

```sql
-- NULL on left side
SELECT NULL::numeric = 10::int4;  -- Expected: NULL
SELECT NULL::float8 < 5::int2;    -- Expected: NULL

-- NULL on right side
SELECT 10::numeric = NULL::int4;  -- Expected: NULL
SELECT 5::float4 > NULL::int8;    -- Expected: NULL

-- NULL on both sides
SELECT NULL::numeric = NULL::int4;  -- Expected: NULL

-- NULL propagation in WHERE clauses (should filter out rows)
CREATE TABLE test_nulls (n numeric, i int4);
INSERT INTO test_nulls VALUES (10, 10), (NULL, 10), (10, NULL), (NULL, NULL);
SELECT * FROM test_nulls WHERE n = i;  -- Expected: 1 row (10, 10 only)
```

**Success Criteria**: NULL handling consistent with SQL standard three-valued logic

---

### 4. Special Values Tests (`sql/special_values.sql`)

**Purpose**: Verify handling of IEEE 754 special float values (NaN, Infinity, -Infinity)

**Test Cases**:

```sql
-- NaN comparisons (always false for equality)
SELECT 'NaN'::float4 = 10::int4;     -- Expected: f
SELECT 'NaN'::float8 = 0::int8;      -- Expected: f
SELECT 'NaN'::float4 < 10::int4;     -- Expected: f
SELECT 'NaN'::float8 > -1000::int4;  -- Expected: f

-- Infinity comparisons
SELECT 'Infinity'::float4 = 10::int4;        -- Expected: f (infinity != finite)
SELECT 'Infinity'::float8 > 1000000::int8;   -- Expected: t (infinity > all finite)
SELECT '-Infinity'::float4 < -1000000::int4; -- Expected: t (-infinity < all finite)

-- Infinity ordering
SELECT '-Infinity'::float8 < 0::int4;  -- Expected: t
SELECT 'Infinity'::float4 > 0::int2;   -- Expected: t
```

**Success Criteria**: Special values follow IEEE 754 semantics and never equal finite integers

---

### 5. Edge Cases Tests (`sql/edge_cases.sql`)

**Purpose**: Verify correct behavior at precision boundaries, range limits, and overflow scenarios

**Test Cases**:

```sql
-- Float4 precision boundary (mantissa limit at 2^24 = 16,777,216)
SELECT 16777216::bigint = 16777216::float4;  -- Expected: t (exactly representable)
SELECT 16777217::bigint = 16777217::float4;  -- Expected: f (precision loss)
SELECT 16777215::bigint = 16777215::float4;  -- Expected: t (within precision)

-- Float8 precision boundary (mantissa limit at 2^53)
SELECT 9007199254740992::bigint = 9007199254740992::float8;  -- Expected: t
SELECT 9007199254740993::bigint = 9007199254740993::float8;  -- Expected: f

-- Integer type range limits
SELECT 32767::numeric = 32767::int2;    -- Expected: t (int2 max)
SELECT -32768::numeric = -32768::int2;  -- Expected: t (int2 min)
SELECT 32768::numeric = 32768::int2;    -- Expected: f (overflow int2, wraps to -32768)

-- Numeric overflow beyond integer range
SELECT 2147483648::numeric = 2147483647::int4;  -- Expected: f (exceeds int4 max)
SELECT 9223372036854775808::numeric = 9223372036854775807::int8;  -- Expected: f (exceeds int8 max)

-- Very large numeric values (should return false for equality)
SELECT 999999999999999999999999::numeric = 1000000::int8;  -- Expected: f

-- Type coercion ambiguity (literal 10.0 could be numeric or float8)
SELECT 10::int4 = 10.0;  -- Should resolve to numeric comparison (PostgreSQL default)

-- Serial type aliases (verify they work identically to underlying integer types)
CREATE TABLE serial_test (
  id_small SMALLSERIAL,
  id_normal SERIAL,
  id_big BIGSERIAL,
  value INT4
);

INSERT INTO serial_test (value) VALUES (1), (100), (10000);

-- SMALLSERIAL should behave like INT2
SELECT id_small = 1.0::numeric FROM serial_test WHERE id_small = 1;     -- Expected: t
SELECT id_small = 1.5::float8 FROM serial_test WHERE id_small = 1;      -- Expected: f
SELECT id_small < 2.0::numeric FROM serial_test WHERE id_small = 1;     -- Expected: t

-- SERIAL should behave like INT4
SELECT id_normal = 2.0::numeric FROM serial_test WHERE id_normal = 2;   -- Expected: t
SELECT id_normal = 2.5::float4 FROM serial_test WHERE id_normal = 2;    -- Expected: f
SELECT id_normal >= 2.0::numeric FROM serial_test WHERE id_normal = 2;  -- Expected: t

-- BIGSERIAL should behave like INT8
SELECT id_big = 3.0::numeric FROM serial_test WHERE id_big = 3;         -- Expected: t
SELECT id_big = 3.1::float8 FROM serial_test WHERE id_big = 3;          -- Expected: f
SELECT id_big <= 4.0::float8 FROM serial_test WHERE id_big = 3;         -- Expected: t
```

**Success Criteria**:
- Precision boundaries correctly detected
- Overflow/underflow cases return false for equality
- No crashes or unexpected errors for extreme values
- Serial type aliases work identically to their underlying integer types (smallserial=int2, serial=int4, bigserial=int8)

---

### 6. Transitivity Tests (`sql/transitivity.sql`)

**Purpose**: Verify query optimizer does not apply transitive inference across exact comparison operators

**Test Setup**:
```sql
CREATE TABLE test_trans (f float8, i int4, n numeric);
INSERT INTO test_trans VALUES (10.0, 10, 10.0), (10.1, 10, 10.0), (10.0, 11, 10.0);
```

**Test Cases**:

```sql
-- Query with chained comparisons
EXPLAIN (VERBOSE ON) 
SELECT * FROM test_trans WHERE f = i AND i = n;

-- Verify no inferred predicate "f = n" appears in plan
-- (Should see only the two explicit predicates)

-- Verify correct results (no incorrect rows from transitive inference)
SELECT * FROM test_trans WHERE f = i AND i = n;
-- Expected: 1 row (10.0, 10, 10.0) only
-- NOT (10.1, 10, 10.0) even though 10.1::float might round to 10::int

-- Multi-table join test
CREATE TABLE t1 (f float4);
CREATE TABLE t2 (i int4);
CREATE TABLE t3 (n numeric);
INSERT INTO t1 VALUES (10.0), (10.1);
INSERT INTO t2 VALUES (10);
INSERT INTO t3 VALUES (10.0);

SELECT * FROM t1, t2, t3 WHERE t1.f = t2.i AND t2.i = t3.n;
-- Should return (10.0, 10, 10.0) only, not (10.1, 10, 10.0)
```

**Success Criteria**:
- Query plans do not show inferred transitive predicates
- Result sets are correct (no extra rows from incorrect transitive optimization)

---

### 7. Performance Benchmark Tests (`sql/performance.sql`)

**Purpose**: Verify comparison overhead is within acceptable bounds (<10% vs native operators)

**Test Setup**:
```sql
CREATE TABLE bench_native (id SERIAL, value INT4);
CREATE TABLE bench_exact (id SERIAL, value INT4);
CREATE INDEX idx_native ON bench_native(value);
CREATE INDEX idx_exact ON bench_exact(value);
INSERT INTO bench_native SELECT generate_series(1, 1000000);
INSERT INTO bench_exact SELECT generate_series(1, 1000000);
ANALYZE bench_native;
ANALYZE bench_exact;
```

**Test Cases**:

```sql
-- Baseline: native integer comparison
\timing on
SELECT COUNT(*) FROM bench_native WHERE value = 500000;
SELECT COUNT(*) FROM bench_native WHERE value < 100000;

-- Exact comparison with numeric constant
SELECT COUNT(*) FROM bench_exact WHERE value = 500000::numeric;
SELECT COUNT(*) FROM bench_exact WHERE value < 100000::numeric;

-- Exact comparison with float constant
SELECT COUNT(*) FROM bench_exact WHERE value = 500000::float8;
SELECT COUNT(*) FROM bench_exact WHERE value < 100000::float8;
\timing off
```

**Success Criteria**:
- Exact comparison queries complete in <1ms for selective predicates (comparable to native)
- Execution time within 10% of native integer comparison
- Index scans used consistently (verified via EXPLAIN)

---

## Test Execution

All tests are run via pg_regress:

```bash
make installcheck
```

Expected output files in `expected/` directory must match actual output exactly.

## Test Coverage Summary

| Category | Test File | Operators Covered | Success Metric |
|----------|-----------|-------------------|----------------|
| Operator Correctness | numeric_int_ops.sql, float_int_ops.sql | All 54 operators | Correct boolean results |
| Index Usage | index_usage.sql | Equality and range ops | Index Scan in EXPLAIN |
| Hash Joins | hash_joins.sql | Equality ops (18) | Hash Join in EXPLAIN |
| Merge Joins | merge_joins.sql | Ordering ops (36) | Merge Join in EXPLAIN |
| NULL Handling | null_handling.sql | All 54 operators | NULL propagation per SQL standard |
| Special Values | special_values.sql | Float operators (36) | IEEE 754 semantics |
| Edge Cases | edge_cases.sql | All 54 operators | Correct at boundaries |
| Transitivity | transitivity.sql | Chained comparisons | No inferred predicates |
| Performance | performance.sql | Equality and range ops | <10% overhead vs native |

**Total Test Count**: ~250+ individual test assertions across all categories
