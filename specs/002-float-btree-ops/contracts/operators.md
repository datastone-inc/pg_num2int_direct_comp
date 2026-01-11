# Operator Contracts: Float-Integer Btree Family

**Feature**: 002-float-btree-ops
**Date**: 2026-01-11

## Btree Operator Family Contracts

### Strategy Numbers

| Strategy | Operator | Meaning |
|----------|----------|---------|
| 1 | `<` | Less than |
| 2 | `<=` | Less than or equal |
| 3 | `=` | Equal |
| 4 | `>=` | Greater than or equal |
| 5 | `>` | Greater than |

### Required Properties

All operators added to btree families MUST satisfy:

1. **Transitivity**: If `a op b` AND `b op c`, then `a op c` (for same operator)
2. **Trichotomy**: Exactly one of `a < b`, `a = b`, `a > b` is true
3. **Consistency**: Operators and comparison function agree on ordering

### Comparison Function Contract

```c
/**
 * @brief Compare float4 with int2 for btree ordering
 * @param arg0 float4 value
 * @param arg1 int2 value
 * @return int4: -1 if arg0 < arg1, 0 if equal, +1 if arg0 > arg1
 *
 * Special value handling:
 * - NaN > all integers (PostgreSQL convention)
 * - +Infinity > all integers
 * - -Infinity < all integers
 * - Exact comparison: no precision loss
 */
int4 float4_cmp_int2(float4 arg0, int2 arg1);
```

## Operator Definitions

### float4 x integer operators

| Operator | Left | Right | Function | Commutator | Negator |
|----------|------|-------|----------|------------|---------|
| `<` | float4 | int2 | float4_lt_int2 | `>` (int2, float4) | `>=` (float4, int2) |
| `<=` | float4 | int2 | float4_le_int2 | `>=` (int2, float4) | `>` (float4, int2) |
| `=` | float4 | int2 | float4_eq_int2 | `=` (int2, float4) | `<>` (float4, int2) |
| `>=` | float4 | int2 | float4_ge_int2 | `<=` (int2, float4) | `<` (float4, int2) |
| `>` | float4 | int2 | float4_gt_int2 | `<` (int2, float4) | `<=` (float4, int2) |

*(Same pattern for float4 x int4, float4 x int8, and reverse directions)*

### float8 x integer operators

| Operator | Left | Right | Function | Commutator | Negator |
|----------|------|-------|----------|------------|---------|
| `<` | float8 | int2 | float8_lt_int2 | `>` (int2, float8) | `>=` (float8, int2) |
| `<=` | float8 | int2 | float8_le_int2 | `>=` (int2, float8) | `>` (float8, int2) |
| `=` | float8 | int2 | float8_eq_int2 | `=` (int2, float8) | `<>` (float8, int2) |
| `>=` | float8 | int2 | float8_ge_int2 | `<=` (int2, float8) | `<` (float8, int2) |
| `>` | float8 | int2 | float8_gt_int2 | `<` (int2, float8) | `<=` (float8, int2) |

*(Same pattern for float8 x int4, float8 x int8, and reverse directions)*

## Btree Family Membership

### integer_ops (btree)

**New operators added**:
- float4 x float4 (same-type, for sort consistency)
- float8 x float8 (same-type, for sort consistency)
- float4 x int2, int4, int8 (cross-type, both directions)
- float8 x int2, int4, int8 (cross-type, both directions)

**Total**: 70 operators, 14 support functions

### float4_ops (btree)

**New operators added**:
- float4 x int2, int4, int8 (cross-type, both directions)

**Total**: 30 operators, 6 support functions

### float8_ops (btree)

**New operators added**:
- float8 x int2, int4, int8 (cross-type, both directions)

**Total**: 30 operators, 6 support functions

## Test Contracts

### Merge Join Verification

```sql
-- GIVEN: Tables with btree indexes on int and float columns
-- WHEN: Join with merge join enabled (hash/nestloop disabled)
-- THEN: Query plan shows "Merge Join"

SET enable_hashjoin = off;
SET enable_nestloop = off;

EXPLAIN (COSTS OFF)
SELECT * FROM int_table i JOIN float4_table f ON i.val = f.val;
-- Expected output contains: "Merge Join"
```

### Transitivity Verification

```sql
-- GIVEN: Chained equality conditions
-- WHEN: Comparing int x float x int
-- THEN: Results are mathematically consistent

SELECT 
  (16777216::int4 = 16777216.0::float4) AS a_eq_b,
  (16777216.0::float4 = 16777216::int8) AS b_eq_c,
  (16777216::int4 = 16777216::int8) AS a_eq_c;
-- Expected: all TRUE (transitivity holds)
```

### Catalog Verification

```sql
-- GIVEN: Extension installed
-- WHEN: Querying pg_amop for float4_ops btree
-- THEN: Cross-type operators are present

SELECT COUNT(*) FROM pg_amop 
WHERE amopfamily = (
  SELECT oid FROM pg_opfamily 
  WHERE opfname = 'float4_ops' 
    AND opfmethod = (SELECT oid FROM pg_am WHERE amname = 'btree')
)
AND (amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
  OR amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype));
-- Expected: 30
```
