# API Reference

Complete reference for all 54 comparison operators provided by pg_num2int_direct_comp.

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

**Index Support**: Yes (via SupportRequestIndexCondition)

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

**Index Support**: Yes (via SupportRequestIndexCondition)

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

**Properties**: MERGES (enables merge joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via SupportRequestIndexCondition)

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

**Properties**: MERGES (enables merge joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via SupportRequestIndexCondition)

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

**Properties**: MERGES (enables merge joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via SupportRequestIndexCondition)

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

**Properties**: MERGES (enables merge joins), COMMUTATOR, NEGATOR

**Index Support**: Yes (via SupportRequestIndexCondition)

## Operator Properties

### HASHES

Applied to: `=`, `<>` operators

Enables: Hash joins for equality-based JOIN operations

Example:
```sql
SELECT * FROM a JOIN b ON a.int_col = b.numeric_col;
-- Can use Hash Join strategy
```

### MERGES

Applied to: `<`, `>`, `<=`, `>=` operators

Enables: Merge joins for ordered JOIN operations

Example:
```sql
SELECT * FROM a JOIN b ON a.int_col < b.float_col;
-- Can use Merge Join strategy
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

## Support Function

### num2int_support

**Purpose**: Provides index optimization for all 54 operators

**Mechanism**: Implements `SupportRequestIndexCondition` to enable btree index usage

**Behavior**:
- Detects when comparison operand is a constant
- Validates constant is within integer type range
- Transforms predicate for index scan

**Example**:
```sql
CREATE INDEX ON table(int_col);
-- Query with constant
SELECT * FROM table WHERE int_col = 100.0::numeric;
-- num2int_support enables: Index Scan using table_int_col_idx
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

## Performance Characteristics

| Operation | Overhead vs Native | Index Usage |
|-----------|-------------------|-------------|
| Equality (=, <>) | <5% | Yes |
| Ordering (<, >, <=, >=) | <10% | Yes |
| Hash Join | <5% | N/A |
| Merge Join | <10% | N/A |

**Tested**: 1M+ row tables, PostgreSQL 12-16

## Limitations

### No Transitivity

Operators are NOT added to btree operator families, preventing transitive inference:

```sql
-- Query optimizer does NOT infer floatcol = 10.0 from:
WHERE floatcol = intcol AND intcol = 10.0::numeric
```

This is intentional to maintain exact comparison semantics.

### No Cross-Type Operator Families

Operators do not participate in cross-type operator families, meaning:
- Cannot be used as index operators for mixed-type indexes
- Cannot be used in CREATE INDEX ON table USING btree (mixed_expression)

This is a safety feature to prevent incorrect query optimization.

## See Also

- [User Guide](user-guide.md) - Usage patterns and examples
- [Development Guide](development.md) - Contributing and testing
