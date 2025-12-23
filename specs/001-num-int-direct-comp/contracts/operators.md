# Operator Definitions: pg_num2int_direct_comp

## Operator Matrix

Complete specification of all 54 comparison operators provided by the extension.

### Operator Naming Convention

Pattern: `<left_type> <operator> <right_type>`

Where:
- `left_type`: numeric, float4, or float8
- `operator`: =, <>, <, >, <=, >=
- `right_type`: int2 (smallint), int4 (integer), int8 (bigint)

### Equality Operators (=) - 9 operators

| SQL Signature | C Function | Commutator | Negator | Support Function |
|--------------|------------|------------|---------|------------------|
| `numeric = int2` | `numeric_eq_int2` | `int2 = numeric` | `numeric <> int2` | `num2int_support` |
| `numeric = int4` | `numeric_eq_int4` | `int4 = numeric` | `numeric <> int4` | `num2int_support` |
| `numeric = int8` | `numeric_eq_int8` | `int8 = numeric` | `numeric <> int8` | `num2int_support` |
| `float4 = int2` | `float4_eq_int2` | `int2 = float4` | `float4 <> int2` | `num2int_support` |
| `float4 = int4` | `float4_eq_int4` | `int4 = float4` | `float4 <> int4` | `num2int_support` |
| `float4 = int8` | `float4_eq_int8` | `int8 = float4` | `float4 <> int8` | `num2int_support` |
| `float8 = int2` | `float8_eq_int2` | `int2 = float8` | `float8 <> int2` | `num2int_support` |
| `float8 = int4` | `float8_eq_int4` | `int4 = float8` | `float8 <> int4` | `num2int_support` |
| `float8 = int8` | `float8_eq_int8` | `int8 = float8` | `float8 <> int8` | `num2int_support` |

### Inequality Operators (<>) - 9 operators

| SQL Signature | C Function | Commutator | Negator |
|--------------|------------|------------|---------|
| `numeric <> int2` | `numeric_ne_int2` | `int2 <> numeric` | `numeric = int2` |
| `numeric <> int4` | `numeric_ne_int4` | `int4 <> numeric` | `numeric = int4` |
| `numeric <> int8` | `numeric_ne_int8` | `int8 <> numeric` | `numeric = int8` |
| `float4 <> int2` | `float4_ne_int2` | `int2 <> float4` | `float4 = int2` |
| `float4 <> int4` | `float4_ne_int4` | `int4 <> float4` | `float4 = int4` |
| `float4 <> int8` | `float4_ne_int8` | `int8 <> float4` | `float4 = int8` |
| `float8 <> int2` | `float8_ne_int2` | `int2 <> float8` | `float8 = int2` |
| `float8 <> int4` | `float8_ne_int4` | `int4 <> float8` | `float8 = int4` |
| `float8 <> int8` | `float8_ne_int8` | `int8 <> float8` | `float8 = int8` |

### Less Than Operators (<) - 9 operators

| SQL Signature | C Function | Commutator | Negator |
|--------------|------------|------------|---------|
| `numeric < int2` | `numeric_lt_int2` | `int2 > numeric` | `numeric >= int2` |
| `numeric < int4` | `numeric_lt_int4` | `int4 > numeric` | `numeric >= int4` |
| `numeric < int8` | `numeric_lt_int8` | `int8 > numeric` | `numeric >= int8` |
| `float4 < int2` | `float4_lt_int2` | `int2 > float4` | `float4 >= int2` |
| `float4 < int4` | `float4_lt_int4` | `int4 > float4` | `float4 >= int4` |
| `float4 < int8` | `float4_lt_int8` | `int8 > float4` | `float4 >= int8` |
| `float8 < int2` | `float8_lt_int2` | `int2 > float8` | `float8 >= int2` |
| `float8 < int4` | `float8_lt_int4` | `int4 > float8` | `float8 >= int4` |
| `float8 < int8` | `float8_lt_int8` | `int8 > float8` | `float8 >= int8` |

### Greater Than Operators (>) - 9 operators

| SQL Signature | C Function | Commutator | Negator |
|--------------|------------|------------|---------|
| `numeric > int2` | `numeric_gt_int2` | `int2 < numeric` | `numeric <= int2` |
| `numeric > int4` | `numeric_gt_int4` | `int4 < numeric` | `numeric <= int4` |
| `numeric > int8` | `numeric_gt_int8` | `int8 < numeric` | `numeric <= int8` |
| `float4 > int2` | `float4_gt_int2` | `int2 < float4` | `float4 <= int2` |
| `float4 > int4` | `float4_gt_int4` | `int4 < float4` | `float4 <= int4` |
| `float4 > int8` | `float4_gt_int8` | `int8 < float4` | `float4 <= int8` |
| `float8 > int2` | `float8_gt_int2` | `int2 < float8` | `float8 <= int2` |
| `float8 > int4` | `float8_gt_int4` | `int4 < float8` | `float8 <= int4` |
| `float8 > int8` | `float8_gt_int8` | `int8 < float8` | `float8 <= int8` |

### Less Than or Equal Operators (<=) - 9 operators

| SQL Signature | C Function | Commutator | Negator |
|--------------|------------|------------|---------|
| `numeric <= int2` | `numeric_le_int2` | `int2 >= numeric` | `numeric > int2` |
| `numeric <= int4` | `numeric_le_int4` | `int4 >= numeric` | `numeric > int4` |
| `numeric <= int8` | `numeric_le_int8` | `int8 >= numeric` | `numeric > int8` |
| `float4 <= int2` | `float4_le_int2` | `int2 >= float4` | `float4 > int2` |
| `float4 <= int4` | `float4_le_int4` | `int4 >= float4` | `float4 > int4` |
| `float4 <= int8` | `float4_le_int8` | `int8 >= float4` | `float4 > int8` |
| `float8 <= int2` | `float8_le_int2` | `int2 >= float8` | `float8 > int2` |
| `float8 <= int4` | `float8_le_int4` | `int4 >= float8` | `float8 > int4` |
| `float8 <= int8` | `float8_le_int8` | `int8 >= float8` | `float8 > int8` |

### Greater Than or Equal Operators (>=) - 9 operators

| SQL Signature | C Function | Commutator | Negator |
|--------------|------------|------------|---------|
| `numeric >= int2` | `numeric_ge_int2` | `int2 <= numeric` | `numeric < int2` |
| `numeric >= int4` | `numeric_ge_int4` | `int4 <= numeric` | `numeric < int4` |
| `numeric >= int8` | `numeric_ge_int8` | `int8 <= numeric` | `numeric < int8` |
| `float4 >= int2` | `float4_ge_int2` | `int2 <= float4` | `float4 < int2` |
| `float4 >= int4` | `float4_ge_int4` | `int4 <= float4` | `float4 < int4` |
| `float4 >= int8` | `float4_ge_int8` | `int8 <= float4` | `float4 < int8` |
| `float8 >= int2` | `float8_ge_int2` | `int2 <= float8` | `float8 < int2` |
| `float8 >= int4` | `float8_ge_int4` | `int4 <= float8` | `float8 < int4` |
| `float8 >= int8` | `float8_ge_int8` | `int8 <= float8` | `float8 < int8` |

## Operator Properties

### All Operators Share These Properties

- **IMMUTABLE**: Yes (function output depends only on inputs, never changes)
- **STRICT**: Yes (returns NULL if any input is NULL)
- **PARALLEL SAFE**: Yes (safe for parallel query execution)
- **RESTRICT**: 
  - Equality/Inequality: `eqsel` / `neqsel`
  - Ordering: `scalarltsel` / `scalargtsel`
- **JOIN**:
  - Equality/Inequality: `eqjoinsel` / `neqjoinsel`
  - Ordering: `scalarltjoinsel` / `scalargtjoinsel`
- **HASHES**: Yes for `=` operator (enables hash joins and hash aggregation)
- **MERGES**: Yes for `<`, `>`, `<=`, `>=` operators (enables merge joins)

### Operator Relationships

**No Transitivity**: These operators are NOT added to btree operator families that would enable transitive inference. Each operator exists independently without family relationships linking different type combinations. The query optimizer will not infer transitive predicates across these operators.

**Example**: From `floatcol = intcol AND intcol = 10::numeric`, the optimizer cannot infer `floatcol = 10::numeric` because the operators are not members of a shared operator family that would indicate transitivity is valid. The exact comparison semantics differ between float↔int and numeric↔int comparisons.

## C Function Signatures

Functions follow PostgreSQL's standard comparison pattern with core `_cmp_internal` functions and operator wrappers:

```c
// Core comparison function (internal, not exposed to SQL)
static int32
type1_cmp_type2_internal(Type1 arg1, Type2 arg2) {
  // Truncate numeric/float to integer part
  // Check bounds (return -1 or 1 if out of range)
  // Compare integer representations
  // Return -1 (less), 0 (equal), or 1 (greater)
  
  return comparison_result;
}

// Operator wrapper functions (exposed to SQL)
PG_FUNCTION_INFO_V1(type1_eq_type2);
Datum
type1_eq_type2(PG_FUNCTION_ARGS) {
  Type1 arg1 = PG_GETARG_TYPE1(0);
  Type2 arg2 = PG_GETARG_TYPE2(1);
  PG_RETURN_BOOL(type1_cmp_type2_internal(arg1, arg2) == 0);
}

PG_FUNCTION_INFO_V1(type1_lt_type2);
Datum
type1_lt_type2(PG_FUNCTION_ARGS) {
  Type1 arg1 = PG_GETARG_TYPE1(0);
  Type2 arg2 = PG_GETARG_TYPE2(1);
  PG_RETURN_BOOL(type1_cmp_type2_internal(arg1, arg2) < 0);
}

// Similar pattern for >, <=, >=, <>
```
}
```

## Support Function Signature

A single generic support function handles all operators:

```c
PG_FUNCTION_INFO_V1(num2int_support);
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    // Initialize OID cache if needed
    init_oid_cache();
    
    // Inspect operator OID to determine type combination
    OpExpr *clause = ...;
    Oid opno = clause->opno;
    
    // Apply appropriate transformation based on operator
    // Transform predicate for index usage
    // Return modified node or NULL
  }
  
  PG_RETURN_POINTER(NULL);
}
```

## Installation SQL Pattern

Each operator and function is registered via SQL. All operators share the same support function:

```sql
-- Generic support function (defined once)
CREATE FUNCTION num2int_support(internal)
  RETURNS internal
  AS 'MODULE_PATHNAME', 'num2int_support'
  LANGUAGE C IMMUTABLE STRICT;

-- Function definition
CREATE FUNCTION numeric_eq_int4(numeric, int4)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'numeric_eq_int4'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
  SUPPORT num2int_support;

-- Operator definition
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

## Total Catalog Entries

- **Operators**: 54 (6 types × 9 combinations)
- **SQL-Callable Functions**: 54 (one per operator, registered in catalog)
- **Internal Functions**: 9 (`_cmp_internal` functions, not in catalog)
- **Support Function**: 1 (`num2int_support` - shared by all operators)

**Note**: Following PostgreSQL's standard pattern, each type combination has one internal `_cmp_` function returning -1/0/1. The 54 operator functions are thin wrappers that call the appropriate `_cmp_internal` function and compare the result. This reduces code duplication while maintaining full operator coverage. The single generic support function reduces maintenance burden and code size significantly.
