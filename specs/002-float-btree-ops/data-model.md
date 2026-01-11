# Data Model: Float-Integer Btree Operator Families

**Feature**: 002-float-btree-ops
**Date**: 2026-01-11

## Overview

This feature adds int x float comparison operators to PostgreSQL's builtin btree operator families, enabling merge joins between integer and floating-point columns.

## Operator Family Schema

### Btree Operator Families Modified

| Family | Access Method | Added Types |
|--------|---------------|-------------|
| `integer_ops` | btree | float4, float8 (same-type + cross-type) |
| `float4_ops` | btree | int2, int4, int8 (cross-type only) |
| `float8_ops` | btree | int2, int4, int8 (cross-type only) |

### pg_amop Entries (Operators)

#### integer_ops btree additions

| Left Type | Right Type | Strategy | Operator | Count |
|-----------|------------|----------|----------|-------|
| float4 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| float4 | int2 | 1-5 | <, <=, =, >=, > | 5 |
| float4 | int4 | 1-5 | <, <=, =, >=, > | 5 |
| float4 | int8 | 1-5 | <, <=, =, >=, > | 5 |
| int2 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| int4 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| int8 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | int2 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | int4 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | int8 | 1-5 | <, <=, =, >=, > | 5 |
| int2 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| int4 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| int8 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| **Total** | | | | **70** |

#### float4_ops btree additions

| Left Type | Right Type | Strategy | Operator | Count |
|-----------|------------|----------|----------|-------|
| float4 | int2 | 1-5 | <, <=, =, >=, > | 5 |
| float4 | int4 | 1-5 | <, <=, =, >=, > | 5 |
| float4 | int8 | 1-5 | <, <=, =, >=, > | 5 |
| int2 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| int4 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| int8 | float4 | 1-5 | <, <=, =, >=, > | 5 |
| **Total** | | | | **30** |

#### float8_ops btree additions

| Left Type | Right Type | Strategy | Operator | Count |
|-----------|------------|----------|----------|-------|
| float8 | int2 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | int4 | 1-5 | <, <=, =, >=, > | 5 |
| float8 | int8 | 1-5 | <, <=, =, >=, > | 5 |
| int2 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| int4 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| int8 | float8 | 1-5 | <, <=, =, >=, > | 5 |
| **Total** | | | | **30** |

### pg_amproc Entries (Support Functions)

#### integer_ops btree additions

| Left Type | Right Type | Function | Proc Num |
|-----------|------------|----------|----------|
| float4 | float4 | btfloat4cmp | 1 |
| float8 | float8 | btfloat8cmp | 1 |
| float4 | int2 | float4_cmp_int2 | 1 |
| float4 | int4 | float4_cmp_int4 | 1 |
| float4 | int8 | float4_cmp_int8 | 1 |
| int2 | float4 | int2_cmp_float4 | 1 |
| int4 | float4 | int4_cmp_float4 | 1 |
| int8 | float4 | int8_cmp_float4 | 1 |
| float8 | int2 | float8_cmp_int2 | 1 |
| float8 | int4 | float8_cmp_int4 | 1 |
| float8 | int8 | float8_cmp_int8 | 1 |
| int2 | float8 | int2_cmp_float8 | 1 |
| int4 | float8 | int4_cmp_float8 | 1 |
| int8 | float8 | int8_cmp_float8 | 1 |

#### float4_ops btree additions

| Left Type | Right Type | Function | Proc Num |
|-----------|------------|----------|----------|
| float4 | int2 | float4_cmp_int2 | 1 |
| float4 | int4 | float4_cmp_int4 | 1 |
| float4 | int8 | float4_cmp_int8 | 1 |
| int2 | float4 | int2_cmp_float4 | 1 |
| int4 | float4 | int4_cmp_float4 | 1 |
| int8 | float4 | int8_cmp_float4 | 1 |

#### float8_ops btree additions

| Left Type | Right Type | Function | Proc Num |
|-----------|------------|----------|----------|
| float8 | int2 | float8_cmp_int2 | 1 |
| float8 | int4 | float8_cmp_int4 | 1 |
| float8 | int8 | float8_cmp_int8 | 1 |
| int2 | float8 | int2_cmp_float8 | 1 |
| int4 | float8 | int4_cmp_float8 | 1 |
| int8 | float8 | int8_cmp_float8 | 1 |

## Event Trigger Cleanup Entries

The `pg_num2int_direct_comp_cleanup()` function must be extended with DROP statements for:

### integer_ops btree (new entries)

```sql
-- Same-type float operators
'ALTER OPERATOR FAMILY integer_ops USING btree DROP OPERATOR 1 (float4, float4)',
'ALTER OPERATOR FAMILY integer_ops USING btree DROP OPERATOR 2 (float4, float4)',
-- ... (5 per type x 2 types = 10)
'ALTER OPERATOR FAMILY integer_ops USING btree DROP FUNCTION 1 (float4, float4)',
'ALTER OPERATOR FAMILY integer_ops USING btree DROP FUNCTION 1 (float8, float8)',

-- Cross-type float x int operators (12 type pairs x 6 entries = 72)
'ALTER OPERATOR FAMILY integer_ops USING btree DROP OPERATOR 1 (float4, int2)',
-- ... etc
```

### float4_ops btree (new entries)

```sql
-- Cross-type operators only (6 type pairs x 6 entries = 36)
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 1 (float4, int2)',
-- ... etc
```

### float8_ops btree (new entries)

```sql
-- Cross-type operators only (6 type pairs x 6 entries = 36)
'ALTER OPERATOR FAMILY float8_ops USING btree DROP OPERATOR 1 (float8, int2)',
-- ... etc
```

## Validation Queries

### Verify operator family membership

```sql
-- Count int x float operators in integer_ops btree
SELECT COUNT(*) 
FROM pg_amop 
WHERE amopfamily = (SELECT oid FROM pg_opfamily WHERE opfname = 'integer_ops' AND opfmethod = (SELECT oid FROM pg_am WHERE amname = 'btree'))
  AND ((amoplefttype = 'float4'::regtype OR amoprighttype = 'float4'::regtype)
    OR (amoplefttype = 'float8'::regtype OR amoprighttype = 'float8'::regtype));
-- Expected: 70

-- Count int x float operators in float4_ops btree
SELECT COUNT(*) 
FROM pg_amop 
WHERE amopfamily = (SELECT oid FROM pg_opfamily WHERE opfname = 'float4_ops' AND opfmethod = (SELECT oid FROM pg_am WHERE amname = 'btree'))
  AND (amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
    OR amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype));
-- Expected: 30
```

### Verify merge join availability

```sql
SET enable_hashjoin = off;
SET enable_nestloop = off;

EXPLAIN (COSTS OFF)
SELECT * FROM int_table i JOIN float4_table f ON i.val = f.val;
-- Should show: Merge Join
```
