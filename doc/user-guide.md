# User Guide

## Introduction

The `pg_num2int_direct_comp` extension provides exact comparison operators between numeric/float types and integer types. This guide covers common usage patterns and best practices.

## Core Concepts

### Exact vs Approximate Comparison

**PostgreSQL Default (Approximate)**:
```sql
SELECT 16777216::bigint = 16777217::float4;  -- true (WRONG - precision loss)
```

**With Extension (Exact)**:
```sql
CREATE EXTENSION pg_num2int_direct_comp;
SELECT 16777216::bigint = 16777217::float4;  -- false (CORRECT - detects mismatch)
```

### Why This Matters

Floating-point types (float4, float8) have limited precision:
- **float4**: 24-bit mantissa (~7 decimal digits)
- **float8**: 53-bit mantissa (~15 decimal digits)

Values outside this precision range cannot be represented exactly, leading to comparison errors.

## Usage Patterns

### Pattern 1: Data Validation

Detect when floating-point columns have drifted from integer precision:

```sql
-- Find rows where float value doesn't exactly match integer ID
SELECT id, score_float 
FROM metrics 
WHERE id::int4 <> score_float::float4;
```

### Pattern 2: Financial Calculations

Ensure decimal amounts match integer cent values:

```sql
-- Verify amount in dollars matches cents
SELECT * 
FROM transactions 
WHERE amount_dollars::numeric <> (cents::int4 / 100.0);
```

### Pattern 3: Range Queries with Exact Boundaries

Query integer columns with fractional boundaries:

```sql
-- Get all values strictly between 10.5 and 20.5
SELECT * FROM inventory 
WHERE quantity::int4 > 10.5::numeric 
  AND quantity::int4 < 20.5::numeric;
-- Returns: 11, 12, ..., 20
```

### Pattern 4: Index-Optimized Lookups

Use indexes efficiently with mixed-type comparisons:

```sql
CREATE TABLE large_table (id INT4 PRIMARY KEY, data TEXT);
CREATE INDEX ON large_table(id);

-- Uses index (fast)
SELECT * FROM large_table WHERE id = 12345.0::numeric;

-- Uses index (fast)
SELECT * FROM large_table WHERE id <= 10000.0::float8;
```

### Pattern 5: JOIN Operations

Join tables on mixed numeric/integer columns:

```sql
-- Exact equality join (uses indexed nested loop or hash join)
SELECT a.*, b.* 
FROM measurements a
JOIN thresholds b ON a.value_int = b.threshold_numeric;

-- Range join (uses indexed nested loop)
SELECT a.*, b.* 
FROM sensor_data a
JOIN alert_ranges b 
  ON a.reading_int > b.min_threshold_float 
 AND a.reading_int < b.max_threshold_float;
```

### Pattern 6: Hash Joins for Large Tables

For large table equijoins, planner automatically uses hash joins:

```sql
-- Large table join - planner chooses hash join
CREATE TABLE sales (id SERIAL, amount NUMERIC(10,2));
CREATE TABLE targets (id SERIAL, threshold INT4);

-- Planner uses Hash Join for large tables
EXPLAIN SELECT COUNT(*) 
FROM sales s 
JOIN targets t ON s.amount = t.threshold;
```

### Pattern 7: Commutator Operators

All operators work in both directions:

```sql
-- Both directions work identically
SELECT 10::int4 = 100.0::numeric;  -- int = numeric
SELECT 100.0::numeric = 10::int4;  -- numeric = int (commutator)

-- Works with all operators
SELECT 10::int4 < 20.5::float8;    -- int < float
SELECT 20.5::float8 > 10::int4;    -- float > int (commutator)
```

## Supported Operators

### Equality Operators

| Operator | Description | Example | Properties |
|----------|-------------|---------|------------|
| `=` | Exact equality | `10::int4 = 10.0::numeric` → true | HASHES, COMMUTATOR, NEGATOR |
| `<>` | Exact inequality | `10::int4 <> 10.5::numeric` → true | HASHES, COMMUTATOR, NEGATOR |
| `<>` | Exact inequality | `10::int4 <> 10.5::numeric` → true | HASHES, COMMUTATOR, NEGATOR |

### Ordering Operators

| Operator | Description | Example | Properties |
|----------|-------------|---------|------------|
| `<` | Less than | `9::int4 < 10.5::numeric` → true | COMMUTATOR, NEGATOR |
| `>` | Greater than | `11::int4 > 10.5::numeric` → true | COMMUTATOR, NEGATOR |
| `<=` | Less than or equal | `10::int4 <= 10.0::numeric` → true | COMMUTATOR, NEGATOR |
| `>=` | Greater than or equal | `10::int4 >= 10.0::numeric` → true | COMMUTATOR, NEGATOR |

## Type Coverage

### Numeric Types

- `numeric` (arbitrary precision)
- `decimal` (alias for numeric)

### Float Types

- `float4` / `real` (single precision)
- `float8` / `double precision` (double precision)

### Integer Types

- `int2` / `smallint` (16-bit)
- `int4` / `integer` / `int` (32-bit)
- `int8` / `bigint` (64-bit)
- `serial` (auto-incrementing int4)
- `bigserial` (auto-incrementing int8)
- `smallserial` (auto-incrementing int2)

### All 9 Type Combinations

Each operator works with all combinations:
- numeric × (int2, int4, int8)
- float4 × (int2, int4, int8)
- float8 × (int2, int4, int8)

## Special Cases

### NULL Handling

NULL comparisons follow SQL standard:

```sql
SELECT NULL::int4 = 10.0::numeric;    -- NULL
SELECT 10::int4 = NULL::numeric;      -- NULL
SELECT NULL::int4 = NULL::numeric;    -- NULL
```

### NaN and Infinity (Float Types)

```sql
-- NaN never equals anything (including itself)
SELECT 'NaN'::float4 = 10::int4;      -- false
SELECT 'NaN'::float8 = 10::int8;      -- false

-- Infinity comparisons
SELECT 'Infinity'::float4 > 10::int4;   -- true
SELECT '-Infinity'::float8 < 10::int8;  -- true
```

### Overflow Behavior

Numeric values exceeding integer range:

```sql
-- Equality returns false for overflow
SELECT 9223372036854775808::numeric = 9223372036854775807::int8;  -- false

-- Ordering follows cast semantics
SELECT 9223372036854775808::numeric > 9223372036854775807::int8;  -- true
```

### Fractional Parts

Fractional numeric values never equal integers:

```sql
SELECT 10.0::numeric = 10::int4;    -- true (no fractional part)
SELECT 10.5::numeric = 10::int4;    -- false (has fractional part)
SELECT 10.001::numeric = 10::int4;  -- false (has fractional part)
```

## Query Optimization

### Constant Predicate Optimization

The extension's support functions (via `SupportRequestSimplify`) optimize constant predicates during query planning:

#### FR-015: Impossible Predicate Detection

When an integer column is compared to a fractional constant, the predicate is recognized as always-false:

```sql
-- int_col can never equal 10.5 (fractional value)
EXPLAIN (COSTS OFF) SELECT * FROM table WHERE int_col = 10.5::numeric;
-- Result:
--  Result
--    One-Time Filter: false

-- Benefits:
-- - Planner estimates rows=0
-- - May skip table scan entirely
-- - Faster query planning
```

#### FR-016: Exact Match Transformation

When a numeric constant exactly equals an integer, it's transformed to use the native integer operator:

```sql
-- 100::numeric exactly equals 100::int4
EXPLAIN (COSTS OFF) SELECT * FROM table WHERE int_col = 100::numeric;
-- Result:
--  Index Scan using idx on table
--    Index Cond: (int_col = 100)    ← Native int4=int4 operator

-- Benefits:
-- - Perfect selectivity estimation
-- - Uses most efficient comparison operator
-- - Better index condition pushdown
```

#### FR-017: Range Boundary Transformation

Range predicates with fractional boundaries are transformed to equivalent integer predicates:

```sql
-- "int_col > 10.5" means "int_col >= 11" for integers
EXPLAIN (COSTS OFF) SELECT * FROM table WHERE int_col > 10.5::numeric;
-- Semantically equivalent to: int_col >= 11

-- Transformations:
-- int_col > 10.5   →  int_col >= 11  (round up for >)
-- int_col >= 10.5  →  int_col >= 11  (round up for >=)
-- int_col < 10.5   →  int_col <= 10  (round down for <)
-- int_col <= 10.5  →  int_col <= 10  (round down for <=)

-- This ensures correct boundary handling and optimal selectivity
```

## Performance Considerations

### Index Usage

Extension operators are index-friendly via btree operator family membership:

```sql
-- Create index
CREATE INDEX idx_value ON measurements(value_int);

-- Query uses index via indexed nested loop join
EXPLAIN SELECT * FROM measurements WHERE value_int = 100.0::numeric;
-- Shows: Index Scan using idx_value with Index Cond
```

### Join Strategies

Operators support **indexed nested loop joins** and **hash joins**:

```sql
-- Indexed nested loop join (with selective predicate)
EXPLAIN SELECT * 
FROM table_a a 
JOIN table_b b ON a.int_col = b.numeric_col
WHERE a.int_col < 1000;
-- Shows: Nested Loop with Index Cond on both sides

-- Hash join (for large table equijoins)
SET enable_nestloop = off;
EXPLAIN SELECT * 
FROM large_table_a a 
JOIN large_table_b b ON a.int_col = b.numeric_col;
-- Shows: Hash Join with Hash Cond
```

**Note**: Merge joins are supported for int × numeric. For int × float, use hash joins or indexed nested loop joins.

### Overhead

- **Indexed joins**: Sub-millisecond on 1M+ row tables
- **Hash joins**: Efficient for large table equijoins
- **Direct comparisons**: <10% overhead vs native integer comparisons

## Best Practices

### 1. Use Explicit Type Casts

Always cast literals to avoid ambiguity:

```sql
-- Good: explicit type
WHERE value = 10.0::numeric

-- Avoid: ambiguous literal
WHERE value = 10.0  -- PostgreSQL picks type
```

### 2. Leverage Indexes

Create indexes on integer columns for mixed-type queries:

```sql
CREATE INDEX ON table(int_column);
-- Enables fast lookups: WHERE int_column = numeric_value
```

### 3. Validate Data Integrity

Use exact comparisons to detect precision issues:

```sql
-- Audit query
SELECT COUNT(*) 
FROM data_sync 
WHERE source_id::int4 <> replica_id::float4;
```

### 4. Document Behavior

Add comments explaining exact comparison usage:

```sql
-- Uses pg_num2int_direct_comp for exact precision
-- Returns false if float cannot exactly represent integer
WHERE sensor_id::int4 = reading::float4
```

### 5. Test Edge Cases

Test boundary values in application code:

```sql
-- Test precision boundaries
SELECT 16777216::int4 = 16777216::float4;   -- true
SELECT 16777217::int4 = 16777217::float4;   -- false (float4 limit)
```

## Troubleshooting

### Unexpected Results

**Issue**: Comparison returns unexpected false

**Check**: Verify float precision limits
```sql
-- float4 loses precision beyond 2^24
SELECT 16777217::float4::text;  -- Shows: 1.67772e+07
```

**Issue**: Query not using index

**Check**: Ensure explicit cast and ANALYZE:
```sql
ANALYZE table_name;
EXPLAIN SELECT * FROM table_name WHERE int_col = numeric_literal;
```

## Next Steps

- [API Reference](api-reference.md) - Complete operator list
- [Development Guide](development.md) - Contributing and testing
