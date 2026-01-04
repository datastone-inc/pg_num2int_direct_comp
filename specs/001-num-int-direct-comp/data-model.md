# Data Model: Direct Exact Comparison for Numeric and Integer Types

**Feature**: 001-num-int-direct-comp  
**Created**: 2025-12-23  
**Purpose**: Define the operator catalog structures and function signatures for exact comparison operators

## Overview

This extension defines comparison operators and support functions in the PostgreSQL catalog. There is no user data storage - the "data model" describes the operator metadata structures that enable exact comparisons between inexact numeric and integer types.

## Operator Catalog Entities

### 1. Comparison Operators (54 total)

Each operator is a catalog entry (`pg_operator`) with properties defining its behavior:

**Operator Types**: 6 comparison operators (=, <>, <, >, <=, >=)  
**Type Combinations**: 9 pairs: (numeric, float4, float8) × (int2, int4, int8)  
**Total**: 6 × 9 = 54 operators

**Operator Structure (example: numeric = int4)**:
```
Operator Name: =
Left Argument Type: numeric
Right Argument Type: int4
Function: numeric_eq_int4
Commutator: = (with reversed arg types)
Negator: <>
RESTRICT: eqsel (selectivity estimator)
JOIN: eqjoinsel (join selectivity)
HASHES: true (enables hash joins for equality)
```

**Key Properties**:
- **COMMUTATOR**: Each operator has a commutator with reversed argument types (e.g., `numeric = int4` commutes to `int4 = numeric`)
- **NEGATOR**: Logical negation operator (= negates to <>, < negates to >=, etc.)
- **RESTRICT/JOIN**: Standard PostgreSQL selectivity estimators (eqsel for equality, scalarltsel for <, etc.)
- **HASHES**: Equality operators support hash joins and hash aggregation
- **MERGES**: Ordering operators (<, >, <=, >=) support merge joins

### 2. Comparison Functions (27 total)

C functions implementing the actual comparison logic. Each function is registered in `pg_proc` catalog:

**Pattern**: `<type1>_<op>_<type2>` (e.g., `numeric_eq_int4`, `float8_lt_int2`)

**Function Categories**:
- **Equality (=)**: 9 functions (one per type combination)
- **Inequality (<>)**: 9 functions  
- **Less than (<)**: 9 functions
- **Greater than (>)**: 9 functions (may be implemented via commutator reuse)
- **Less than or equal (<=)**: 9 functions (may be implemented via negator reuse)
- **Greater than or equal (>=)**: 9 functions (may be implemented via negator reuse)

**Note**: Some functions may be consolidated via commutator/negator relationships to reduce code duplication (e.g., `numeric > int4` can reuse `int4 < numeric` logic).

**Function Signature**:
```c
Datum function_name(PG_FUNCTION_ARGS);
```

**Arguments** (via `PG_FUNCTION_ARGS` macro):
- Argument 0: Left operand (numeric, float4, or float8)
- Argument 1: Right operand (int2, int4, or int8)

**Return**: Boolean (true/false) via `PG_RETURN_BOOL()`

### 2. Comparison Functions (9 core + 54 wrappers)

C functions implementing exact comparison logic following PostgreSQL's standard pattern:

**Core Comparison Functions (9 total)**: Internal `_cmp_` functions returning -1, 0, 1

**Pattern**: `<type1>_cmp_<type2>_internal`

**Function Signature**:
```c
static int32 function_name_internal(Type1 arg1, Type2 arg2);
```

**Behavior**: 
1. Truncate numeric/float to integer part (reuse PostgreSQL's `numeric_trunc`)
2. Check bounds (return -1 or 1 for out-of-range values)
3. Compare integer representations
4. Return -1 (less), 0 (equal), or 1 (greater)

**Operator Wrapper Functions (54 total)**: PostgreSQL-callable operators

**Pattern**: `<type1>_<op>_<type2>`

**Function Signature**:
```c
Datum function_name(PG_FUNCTION_ARGS);
```

**Input**: Two PostgreSQL Datum values (via `PG_GETARG_*` macros)  
**Output**: Boolean Datum (`PG_RETURN_BOOL`)

**Behavior**: Thin wrapper calling appropriate `_cmp_internal` function and comparing result:
- `=` returns `cmp == 0`
- `<>` returns `cmp != 0`
- `<` returns `cmp < 0`
- `>` returns `cmp > 0`
- `<=` returns `cmp <= 0`
- `>=` returns `cmp >= 0`

### 3. Support Function (1 total)

A single generic function providing index optimization support for all comparison operators via `SupportRequestIndexCondition`:

**Function Name**: `num2int_support`

**Function Signature**:
```c
Datum num2int_support(PG_FUNCTION_ARGS);
```

**Input**: `Node *` pointer to support request (cast to `SupportRequestIndexCondition`)  
**Output**: Modified query node for index optimization or NULL if not applicable

**Behavior**: 
- Inspects the operator OID using the lazy OID cache to identify which comparison is being invoked
- Extracts operand types from the clause
- Transforms predicates like `intcol = 10.0::numeric` into forms that can use existing btree indexes on integer columns
- Handles all 54 operators through a single generic implementation
- Returns appropriate transformation based on the specific type combination and operator

## Type Combination Matrix

Complete matrix of supported comparisons:

| Numeric Type | Integer Type | Precision Boundary | Implementation Notes |
|--------------|--------------|-------------------|---------------------|
| numeric | int2 | N/A (numeric is exact) | Check fractional part, range [-32768, 32767] |
| numeric | int4 | N/A | Check fractional part, range [-2^31, 2^31-1] |
| numeric | int8 | N/A | Check fractional part, range [-2^63, 2^63-1] |
| float4 | int2 | 2^24 (16,777,216) | Check mantissa precision, range [-32768, 32767] |
| float4 | int4 | 2^24 | Check mantissa precision, range [-2^31, 2^31-1] |
| float4 | int8 | 2^24 | Check mantissa precision, range [-2^63, 2^63-1] |
| float8 | int2 | 2^53 (9.0e15) | Check mantissa precision, range [-32768, 32767] |
| float8 | int4 | 2^53 | Check mantissa precision, range [-2^31, 2^31-1] |
| float8 | int8 | 2^53 | Check mantissa precision, range [-2^63, 2^63-1] |

## State Management

### Operator OID Cache

Per-backend static structure for caching operator OIDs (lazily initialized):

```c
typedef struct {
  bool initialized;
  Oid numeric_eq_int2;
  Oid numeric_eq_int4;
  Oid numeric_eq_int8;
  Oid numeric_ne_int2;
  // ... all 54 operator OIDs
} OperatorOidCache;
```

**Lifecycle**:
- Initialized on first access within a backend session
- Persists for lifetime of backend process
- No cleanup required (static storage)

**Purpose**: Avoid repeated catalog lookups in support functions

## Validation Rules

### Comparison Function Validation

1. **NULL Handling**: If either operand is NULL, return NULL (SQL standard)
2. **Fractional Check** (numeric types): If fractional part exists, equality returns false
3. **Range Check**: If numeric value exceeds integer type bounds, equality returns false
4. **Special Values** (float types):
   - NaN compared to anything (including NaN) returns false for equality
   - Infinity/-Infinity follow IEEE 754 ordering rules
   - Infinity never equals finite integer

### Index Support Validation

1. **Constant Detection**: Support function only optimizes when numeric operand is constant
2. **Range Verification**: Constant must be within integer type range to enable index scan
3. **Exactness Check**: For float constants, verify exact integer representation (no fractional bits)

## Catalog Integration

Extension defines catalog entries via SQL script:

```sql
-- Support function (shared by all operators)
CREATE FUNCTION num2int_support(internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'num2int_support'
  LANGUAGE C IMMUTABLE STRICT;

-- Function registration with SUPPORT property
CREATE FUNCTION numeric_eq_int4(numeric, int4) 
  RETURNS boolean 
  AS 'MODULE_PATHNAME', 'numeric_eq_int4'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
  SUPPORT num2int_support;  -- Support function is property of the function

-- Operator registration (no SUPPORT property here)
CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);
```

## Relationships

### Operator Properties

**Commutative Pairs**:
- `numeric = int4` ↔ `int4 = numeric`
- `float8 < int2` ↔ `int2 > float8`
- (All 54 operators have commutators)

**Negation Pairs**:
- `=` ↔ `<>`
- `<` ↔ `>=`
- `>` ↔ `<=`

**NO Transitivity**: Operators are not added to cross-type btree operator families. Each operator stands alone, preventing the optimizer from inferring `floatcol = 10.0` from `floatcol = intcol AND intcol = 10.0` since the different type comparisons use independent operator definitions.

## Performance Characteristics

### Comparison Function Performance

- **Numeric vs Integer**: O(1) - truncation check, bounds check, comparison
- **Float vs Integer**: O(1) - mantissa inspection, bounds check, comparison
- **Memory**: No dynamic allocation except for intermediate Numeric values (managed by PostgreSQL)

### Index Support Performance

- **OID Cache Hit**: O(1) - static array lookup
- **OID Cache Miss**: O(log n) - catalog syscache lookup (first access only)
- **Index Transformation**: O(1) - node modification

### Expected Query Performance

- **Indexed Queries**: Sub-millisecond for selective predicates (matches native integer index performance)
- **Sequential Scans**: <10% overhead vs native comparison (due to additional bounds/precision checks)

## Summary

The data model consists of 54 operators, 27 comparison functions, and 9 support functions registered in PostgreSQL's system catalogs. The extension introduces no user-visible data structures - all "data" is operator metadata enabling exact comparison semantics and index optimization for mixed numeric/integer type queries.
