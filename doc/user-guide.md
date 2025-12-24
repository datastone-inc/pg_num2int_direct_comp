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
-- Exact equality join
SELECT a.*, b.* 
FROM measurements a
JOIN thresholds b ON a.value_int = b.threshold_numeric;

-- Range join
SELECT a.*, b.* 
FROM sensor_data a
JOIN alert_ranges b 
  ON a.reading_int > b.min_threshold_float 
 AND a.reading_int < b.max_threshold_float;
```

## Supported Operators

### Equality Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `=` | Exact equality | `10::int4 = 10.0::numeric` → true |
| `<>` | Exact inequality | `10::int4 <> 10.5::numeric` → true |

### Ordering Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `<` | Less than | `9::int4 < 10.5::numeric` → true |
| `>` | Greater than | `11::int4 > 10.5::numeric` → true |
| `<=` | Less than or equal | `10::int4 <= 10.0::numeric` → true |
| `>=` | Greater than or equal | `10::int4 >= 10.0::numeric` → true |

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

## Performance Considerations

### Index Usage

Extension operators are index-friendly:

```sql
-- Create index
CREATE INDEX idx_value ON measurements(value_int);

-- Query uses index
EXPLAIN SELECT * FROM measurements WHERE value_int = 100.0::numeric;
-- Shows: Index Scan using idx_value
```

### Join Strategies

Operators support hash joins and merge joins:

```sql
-- Hash join (equality)
SELECT /*+ HashJoin(a b) */ * 
FROM table_a a 
JOIN table_b b ON a.int_col = b.numeric_col;

-- Merge join (ordering)
SELECT /*+ MergeJoin(a b) */ * 
FROM table_a a 
JOIN table_b b ON a.int_col >= b.float_col;
```

### Overhead

- **Index scans**: <1% overhead vs native integer comparisons
- **Table scans**: <10% overhead vs native integer comparisons
- **Large tables**: Sub-millisecond response for selective predicates

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
