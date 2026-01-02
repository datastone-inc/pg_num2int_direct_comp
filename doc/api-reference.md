# API Reference

Complete reference for all 108 comparison operators provided by pg_num2int_direct_comp.

This includes 6 comparison types (=, <>, <, >, <=, >=) × 9 type pairs (numeric/float4/float8 with int2/int4/int8) × 2 directions (forward and commutator), for a total of 108 operators.

## Operator Matrix

### Equality Operators (=)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_eq_int2` | `int2 = numeric` | `numeric <> int2` |
| numeric | int4 | `numeric_eq_int4` | `int4 = numeric` | `numeric <> int4` |
| numeric | int8 | `numeric_eq_int8` | `int8 = numeric` | `numeric <> int8` |
| float4 | int2 | `float4_eq_int2` | `int2 = float4` | `float4 <> int2` |
| float4 | int4 | `float4_eq_int4` | `int4 = float4` | `float4 <> int4` |
| float4 | int8 | `float4_eq_int8` | `int8 = float4` | `float4 <> int8` |
| float8 | int2 | `float8_eq_int2` | `int2 = float8` | `float8 <> int2` |
| float8 | int4 | `float8_eq_int4` | `int4 = float8` | `float8 <> int4` |
| float8 | int8 | `float8_eq_int8` | `int8 = float8` | `float8 <> int8` |

**Properties**: HASHES (enables hash joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

### Inequality Operators (<>)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_ne_int2` | `int2 <> numeric` | `numeric = int2` |
| numeric | int4 | `numeric_ne_int4` | `int4 <> numeric` | `numeric = int4` |
| numeric | int8 | `numeric_ne_int8` | `int8 <> numeric` | `numeric = int8` |
| float4 | int2 | `float4_ne_int2` | `int2 <> float4` | `float4 = int2` |
| float4 | int4 | `float4_ne_int4` | `int4 <> float4` | `float4 = int4` |
| float4 | int8 | `float4_ne_int8` | `int8 <> float4` | `float4 = int8` |
| float8 | int2 | `float8_ne_int2` | `int2 <> float8` | `float8 = int2` |
| float8 | int4 | `float8_ne_int4` | `int4 <> float8` | `float8 = int4` |
| float8 | int8 | `float8_ne_int8` | `int8 <> float8` | `float8 = int8` |

**Properties**: HASHES (enables hash joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

### Less Than Operators (<)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_lt_int2` | `int2 > numeric` | `numeric >= int2` |
| numeric | int4 | `numeric_lt_int4` | `int4 > numeric` | `numeric >= int4` |
| numeric | int8 | `numeric_lt_int8` | `int8 > numeric` | `numeric >= int8` |
| float4 | int2 | `float4_lt_int2` | `int2 > float4` | `float4 >= int2` |
| float4 | int4 | `float4_lt_int4` | `int4 > float4` | `float4 >= int4` |
| float4 | int8 | `float4_lt_int8` | `int8 > float4` | `float4 >= int8` |
| float8 | int2 | `float8_lt_int2` | `int2 > float8` | `float8 >= int2` |
| float8 | int4 | `float8_lt_int4` | `int4 > float8` | `float8 >= int4` |
| float8 | int8 | `float8_lt_int8` | `int8 > float8` | `float8 >= int8` |

**Properties**: COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

### Greater Than Operators (>)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_gt_int2` | `int2 < numeric` | `numeric <= int2` |
| numeric | int4 | `numeric_gt_int4` | `int4 < numeric` | `numeric <= int4` |
| numeric | int8 | `numeric_gt_int8` | `int8 < numeric` | `numeric <= int8` |
| float4 | int2 | `float4_gt_int2` | `int2 < float4` | `float4 <= int2` |
| float4 | int4 | `float4_gt_int4` | `int4 < float4` | `float4 <= int4` |
| float4 | int8 | `float4_gt_int8` | `int8 < float4` | `float4 <= int8` |
| float8 | int2 | `float8_gt_int2` | `int2 < float8` | `float8 <= int2` |
| float8 | int4 | `float8_gt_int4` | `int4 < float8` | `float8 <= int4` |
| float8 | int8 | `float8_gt_int8` | `int8 < float8` | `float8 <= int8` |

**Properties**: COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

### Less Than or Equal Operators (<=)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_le_int2` | `int2 >= numeric` | `numeric > int2` |
| numeric | int4 | `numeric_le_int4` | `int4 >= numeric` | `numeric > int4` |
| numeric | int8 | `numeric_le_int8` | `int8 >= numeric` | `numeric > int8` |
| float4 | int2 | `float4_le_int2` | `int2 >= float4` | `float4 > int2` |
| float4 | int4 | `float4_le_int4` | `int4 >= float4` | `float4 > int4` |
| float4 | int8 | `float4_le_int8` | `int8 >= float4` | `float4 > int8` |
| float8 | int2 | `float8_le_int2` | `int2 >= float8` | `float8 > int2` |
| float8 | int4 | `float8_le_int4` | `int4 >= float8` | `float8 > int4` |
| float8 | int8 | `float8_le_int8` | `int8 >= float8` | `float8 > int8` |

**Properties**: COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

### Greater Than or Equal Operators (>=)

| Left Type | Right Type | C Function | Commutator | Negator |
|-----------|------------|------------|------------|---------|
| numeric | int2 | `numeric_ge_int2` | `int2 <= numeric` | `numeric < int2` |
| numeric | int4 | `numeric_ge_int4` | `int4 <= numeric` | `numeric < int4` |
| numeric | int8 | `numeric_ge_int8` | `int8 <= numeric` | `numeric < int8` |
| float4 | int2 | `float4_ge_int2` | `int2 <= float4` | `float4 < int2` |
| float4 | int4 | `float4_ge_int4` | `int4 <= float4` | `float4 < int4` |
| float4 | int8 | `float4_ge_int8` | `int8 <= float4` | `float4 < int8` |
| float8 | int2 | `float8_ge_int2` | `int2 <= float8` | `float8 < int2` |
| float8 | int4 | `float8_ge_int4` | `int4 <= float8` | `float8 < int4` |
| float8 | int8 | `float8_ge_int8` | `int8 <= float8` | `float8 < int8` |

**Properties**: COMMUTATOR, NEGATOR

**Index Support**: Yes (via btree operator family membership)

## Operator Properties

### HASHES

Applied to: `=` operators only

Enables: Hash joins for equality-based JOIN operations

Example:

```sql
SELECT * FROM a JOIN b ON a.int_col = b.numeric_col;
-- Can use Hash Join strategy
```

### COMMUTATOR

All operators have commutators (reversed argument order):

- `numeric = int4` commutes with `int4 = numeric`
- `float4 < int2` commutes with `int2 > float4`

### NEGATOR

All operators have negators (logical opposites):

- `=` negates with `<>`
- `<` negates with `>=`
- `>` negates with `<=`

### MERGES

Applied to: `=` operators only

Enables: Merge joins for equality-based JOIN operations on pre-sorted data.

### Operator Family Membership

**Btree Families** (enables index scans and merge joins):

- Cross-type operators added to `integer_ops`, `numeric_ops`, and `float_ops` btree families
- Each operator registered with standard btree strategy (1=<, 2=<=, 3==, 4=>=, 5=>)
- Each type pair has a dedicated comparison support function (FUNCTION 1)
- Builtin same-type operators also added to enable sorting within the family context (e.g., numeric×numeric operators in `integer_ops` for merge join sorting)
- Enables queries like `WHERE int_col = numeric_val` to use btree indexes with Index Cond
- Enables merge joins between integer and numeric/float columns

**Hash Families** (enables hash joins):

- Cross-type operators added to `integer_ops`, `numeric_ops`, and `float_ops` hash families
- Builtin same-type hash functions also added for consistent hashing within each family
- Integer hash wrapper functions cast to higher-precision type before hashing
- Ensures equal values hash identically: `hash(10::int4) = hash(10.0::numeric)`
- All equality operators have HASHES property

**DROP EXTENSION Handling**:

- Operator family entries (`pg_amop`) added to builtin families are not automatically tracked as extension-owned by PostgreSQL
- An event trigger runs on DROP EXTENSION to remove these entries, enabling clean reinstallation
- The support function's internal operator OID cache is invalidated via an OPEROID syscache callback when operators are dropped

## Support Function

### num2int_support

**Purpose**: Optimizes cross-type comparisons during query planning

**Mechanism**: Implements `SupportRequestSimplify` and `SupportRequestIndexCondition` to transform predicates at plan time

**Behavior**:

- Detects when comparison operand is a constant
- Transforms fractional constants to impossible predicates or adjusted integer bounds
- Converts exact integer constants to native integer comparisons
- Enables index scans via predicate transformation

**Example**:

```sql
CREATE INDEX ON table(intkey1, intkey2);
-- Query with constants
SELECT * FROM table WHERE intkey1 = 100.0::numeric AND intkey2 > 10.5::numeric;
-- num2int_support transforms to: intkey1 = 100 AND intkey2 >= 11
-- Result: Index Scan using table_intkey1_intkey2_idx
```

## Type Aliases

### Serial Types

| Alias | Base Type | Notes |
|-------|-----------|-------|
| `smallserial` | `int2` | Auto-incrementing 16-bit |
| `serial` | `int4` | Auto-incrementing 32-bit |
| `bigserial` | `int8` | Auto-incrementing 64-bit |

All serial types work identically to their base integer types in comparisons.

### Decimal Type

| Alias | Base Type | Notes |
|-------|-----------|-------|
| `decimal` | `numeric` | Arbitrary precision |

Decimal is a SQL standard alias for numeric.

## Precision Boundaries

### float4 (real)

- **Mantissa**: 24 bits
- **Exact range**: ±2^24 (±16,777,216)
- **Beyond range**: Precision loss in comparisons

```sql
SELECT 16777216::int4 = 16777216::float4;  -- true (within range)
SELECT 16777217::int4 = 16777217::float4;  -- false (precision loss)
```

### float8 (double precision)

- **Mantissa**: 53 bits
- **Exact range**: ±2^53 (±9,007,199,254,740,992)
- **Beyond range**: Precision loss in comparisons

```sql
SELECT 9007199254740992::int8 = 9007199254740992::float8;  -- true
SELECT 9007199254740993::int8 = 9007199254740993::float8;  -- false
```

### numeric/decimal

- **Precision**: Arbitrary (limited by available memory)
- **Exact range**: No inherent limits
- **Integer types**:
  - int2: -32,768 to 32,767
  - int4: -2,147,483,648 to 2,147,483,647
  - int8: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807

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
-- NaN comparisons with integers always return false for equality
SELECT 'NaN'::float4 = 10::int4;      -- false
SELECT 'NaN'::float8 = 10::int8;      -- false

-- NaN sorts higher than everything (including max integers)
SELECT 2147483647::int4 < 'NaN'::float4;  -- true

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

The extension's support functions optimize constant predicates during query planning via `SupportRequestSimplify`.

### Impossible Predicate Detection

When an integer column is compared to a fractional constant, the predicate is recognized as always-false:

```sql
-- int_col can never equal 10.5 (fractional value)
EXPLAIN (COSTS OFF) SELECT * FROM table WHERE int_col = 10.5::numeric;
-- Result:
--  Result
--    One-Time Filter: false
```

Benefits: Planner estimates rows=0, may skip table scan entirely.

### Exact Match Transformation

When a numeric constant exactly equals an integer, it's transformed to use the native integer operator:

```sql
-- 100::numeric exactly equals 100::int4
EXPLAIN (COSTS OFF) SELECT * FROM table WHERE int_col = 100::numeric;
-- Result:
--  Index Scan using idx on table
--    Index Cond: (int_col = 100)    -- Native int4=int4 operator
```

Benefits: Perfect selectivity estimation, most efficient comparison operator.

### Range Boundary Transformation

Range predicates with fractional boundaries are transformed to equivalent integer predicates:

```sql
-- "int_col > 10.5" becomes "int_col >= 11" for integers
-- Transformations:
-- int_col > 10.5   →  int_col >= 11  (round up for >)
-- int_col >= 10.5  →  int_col >= 11  (round up for >=)
-- int_col < 10.5   →  int_col <= 10  (round down for <)
-- int_col <= 10.5  →  int_col <= 10  (round down for <=)
```

## Performance Characteristics

Cross-type comparisons with constant operands are transformed to native integer comparisons at plan time, achieving sub-millisecond execution with index scans on 1M+ row tables.

See [Benchmark Guide](benchmark.md) for benchmark details.

## Best Practices

### Use Explicit Type Casts

Always cast literals to avoid ambiguity:

```sql
-- Good: explicit type
WHERE value = 10.0::numeric

-- Avoid: ambiguous literal
WHERE value = 10.0  -- PostgreSQL picks type
```

### Leverage Indexes

Create indexes on integer columns for mixed-type queries:

```sql
CREATE INDEX ON table(int_column);
-- Enables fast lookups: WHERE int_column = numeric_value
```

### Validate Data Integrity

Use exact comparisons to detect precision issues:

```sql
-- Audit query
SELECT COUNT(*) 
FROM data_sync 
WHERE source_id::int4 <> replica_id::float4;
```

### Test Edge Cases

Test boundary values in application code:

```sql
-- Test precision boundaries
SELECT 16777216::int4 = 16777216::float4;   -- true
SELECT 16777217::int4 = 16777217::float4;   -- false (float4 limit)
```

## Troubleshooting

### Comparison Returns Unexpected False

Verify float precision limits:

```sql
-- float4 loses precision beyond 2^24
SELECT 16777217::float4::text;  -- Shows: 1.67772e+07
```

### Query Not Using Index

Ensure explicit cast and run ANALYZE:

```sql
ANALYZE table_name;
EXPLAIN SELECT * FROM table_name WHERE int_col = 100::numeric;
```

## See Also

- [Installation Guide](installation.md) - Setup and prerequisites
- [Development Guide](development.md) - Contributing and testing
