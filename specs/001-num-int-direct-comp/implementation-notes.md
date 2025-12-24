# Implementation Notes: pg_num2int_direct_comp

**Last Updated**: 2025-12-24  
**Status**: Phase 4 Index Optimization - Infrastructure Complete, Testing In Progress

## Implementation Summary

**Completed**:
- ✅ All 54 comparison operators (equality, inequality, range) implemented
- ✅ Support function with SupportRequestIndexCondition for index optimization  
- ✅ OID cache for operator lookups
- ✅ NUM2INT_ prefixed symbolic constants for all OIDs
- ✅ Bidirectional function registration with SUPPORT clauses
- ✅ Regression tests passing (numeric_int_ops, float_int_ops, index_usage)

**Remaining**:
- Performance benchmarking on large datasets (1M+ rows)
- Comprehensive index usage testing for all 9 type combinations
- Range operator support in SupportRequestIndexCondition (currently equality only)

## Critical Implementation Patterns

### 1. Bidirectional Operator Implementation

**Problem**: Shell operators (created by COMMUTATOR) cannot have support functions.

**Solution**: Implement BOTH directions with real C functions:

```sql
-- Forward direction: numeric = int4
CREATE FUNCTION numeric_eq_int4(numeric, int4)
RETURNS boolean LANGUAGE C STRICT IMMUTABLE
AS '$libdir/pg_num2int_direct_comp', 'numeric_eq_int4'
SUPPORT num2int_support;

CREATE OPERATOR = (
  LEFTARG = numeric, RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>
);

-- Reverse direction: int4 = numeric (MUST have real function, not shell)
CREATE FUNCTION int4_eq_numeric(int4, numeric)
RETURNS boolean LANGUAGE C STRICT IMMUTABLE
AS '$libdir/pg_num2int_direct_comp', 'int4_eq_numeric'
SUPPORT num2int_support;

CREATE OPERATOR = (
  LEFTARG = int4, RIGHTARG = numeric,
  FUNCTION = int4_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>
);
```

**C Implementation** (commutator swaps args):
```c
// Forward: numeric = int4
Datum
numeric_eq_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 i4 = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) == 0);
}

// Reverse: int4 = numeric (reuses same comparison logic)
Datum
int4_eq_numeric(PG_FUNCTION_ARGS) {
  int32 i4 = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) == 0);
}
```

**Key Insight**: PostgreSQL's query planner uses operator direction based on operand order. For `WHERE intcol = 100::numeric`, it looks for `int4 = numeric`, not `numeric = int4`. Both directions must exist as real functions for index optimization to work.

### 2. Support Function Return Type

**Problem**: Returning bare `OpExpr *` causes server crash.

**Solution**: Support functions MUST return `List *` containing the transformed OpExpr:

```c
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  Node *ret = NULL;
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;
    
    // Set lossy flag for exact transformations
    req->lossy = false;
    
    // ... transformation logic ...
    
    // Build transformed OpExpr
    OpExpr *new_clause = (OpExpr *) make_opclause(
        standard_eq_oid,  // int4 = int4 operator
        BOOLOID, false,
        (Expr *) new_var,
        (Expr *) new_const,
        InvalidOid, InvalidOid);
    
    // CRITICAL: Wrap in List before returning
    ret = (Node *) list_make1(new_clause);
  }
  
  PG_RETURN_POINTER(ret);
}
```

**Rationale**: Some transformations generate multiple conditions (e.g., LIKE 'abc%' → `col >= 'abc' AND col < 'abd'`). The API expects a List even for single-condition cases.

**Reference**: `src/backend/utils/adt/like_support.c:375-380`

### 3. Type Conversion Using PostgreSQL's Cast Functions

**Problem**: `numeric_int4_opt_error()` returns `is_valid=false` even for exact integers like `100.0`.

**Solution**: Use PostgreSQL's standard cast functions via `OidFunctionCall1`:

```c
if (const_node->consttype == oid_cache.numeric_oid) {
  Numeric num = DatumGetNumeric(const_node->constvalue);
  
  // Skip NaN/Infinity
  if (numeric_is_nan(num) || numeric_is_inf(num)) {
    PG_RETURN_POINTER(NULL);
  }
  
  PG_TRY();
  {
    Datum result;
    if (int_type == INT2OID) {
      // Use int2(numeric) cast function - OID 1783
      result = OidFunctionCall1(1783, NumericGetDatum(num));
      int_val = DatumGetInt16(result);
    } else if (int_type == INT4OID) {
      // Use int4(numeric) cast function - OID 1744
      result = OidFunctionCall1(1744, NumericGetDatum(num));
      int_val = DatumGetInt32(result);
    } else if (int_type == INT8OID) {
      // Use int8(numeric) cast function - OID 1779
      result = OidFunctionCall1(1779, NumericGetDatum(num));
      int_val = DatumGetInt64(result);
    }
    is_valid = true;
  }
  PG_CATCH();
  {
    // Out of range - conversion failed
    FlushErrorState();
    is_valid = false;
  }
  PG_END_TRY();
}
```

**Cast Behavior** (banker's rounding):
- `100.0::numeric::int4` → `100`
- `100.5::numeric::int4` → `101` (rounds to nearest even)
- `100.4::numeric::int4` → `100`

**Key Function OIDs**:
- `int2(numeric)`: OID 1783 → `#define F_NUMERIC_INT2 1783`
- `int4(numeric)`: OID 1744 → `#define F_NUMERIC_INT4 1744`
- `int8(numeric)`: OID 1779 → `#define F_NUMERIC_INT8 1779`

**Usage in code**:
```c
result = OidFunctionCall1(F_NUMERIC_INT4, NumericGetDatum(num));
```

**Handling All Comparison Operators**:

The support function must handle ALL six comparison operators. Here's the complete logic:

```c
// Determine operator type from OpExpr OID
Oid operator_oid = opexpr->opno;
bool is_eq = (operator_oid == oid_cache.numeric_eq_int4_oid || 
              operator_oid == oid_cache.int4_eq_numeric_oid);
bool is_ne = (operator_oid == oid_cache.numeric_ne_int4_oid || 
              operator_oid == oid_cache.int4_ne_numeric_oid);
bool is_lt = (operator_oid == oid_cache.numeric_lt_int4_oid || 
              operator_oid == oid_cache.int4_gt_numeric_oid);  // note swap
bool is_le = (operator_oid == oid_cache.numeric_le_int4_oid || 
              operator_oid == oid_cache.int4_ge_numeric_oid);  // note swap
bool is_gt = (operator_oid == oid_cache.numeric_gt_int4_oid || 
              operator_oid == oid_cache.int4_lt_numeric_oid);  // note swap
bool is_ge = (operator_oid == oid_cache.numeric_ge_int4_oid || 
              operator_oid == oid_cache.int4_le_numeric_oid);  // note swap

// Convert constant with PostgreSQL's standard cast
bool has_fractional_part = false;
int64 int_val;
if (!convert_to_int_with_cast(const_node, int_type, &int_val, &has_fractional_part)) {
  // Out of range - handle by operator type
  if (is_eq || is_lt || is_le || is_gt || is_ge) {
    return create_always_false_condition(var_node, int_type);
  } else {  // is_ne
    return NULL;  // All rows match, use original condition
  }
}

// Handle fractional values based on operator
if (has_fractional_part) {
  if (is_eq) {
    // int4col = 10.5 → int4col > MAX_INT4 (always false)
    return create_always_false_condition(var_node, int_type);
  }
  else if (is_ne) {
    // int4col <> 10.5 → NULL (all rows match)
    return NULL;
  }
  else if (is_lt) {
    // int4col < 10.5 → int4col <= 10 (round down)
    return create_transformed_condition(var_node, int_type, LE_OP, 
                                       floor_value(int_val));
  }
  else if (is_le) {
    // int4col <= 10.5 → int4col <= 10 (round down)
    return create_transformed_condition(var_node, int_type, LE_OP, 
                                       floor_value(int_val));
  }
  else if (is_gt) {
    // int4col > 10.5 → int4col >= 11 (round up)
    return create_transformed_condition(var_node, int_type, GE_OP, 
                                       ceil_value(int_val));
  }
  else if (is_ge) {
    // int4col >= 10.5 → int4col >= 11 (round up)
    return create_transformed_condition(var_node, int_type, GE_OP, 
                                       ceil_value(int_val));
  }
}

// No fractional part - direct transformation
return create_transformed_condition(var_node, int_type, 
                                   map_to_standard_op(operator_oid), 
                                   int_val);
```

**Helper Function for Always-False Condition**:
```c
static List *
create_always_false_condition(Var *var_node, Oid int_type) {
  Const *impossible_const;
  Oid gt_oid;
  
  if (int_type == INT2OID) {
    impossible_const = makeConst(INT2OID, -1, InvalidOid, sizeof(int16),
                                Int16GetDatum(PG_INT16_MAX), false, true);
    gt_oid = oid_cache.int2_gt_int2_oid;  // > operator for int2
  } else if (int_type == INT4OID) {
    impossible_const = makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                                Int32GetDatum(PG_INT32_MAX), false, true);
    gt_oid = oid_cache.int4_gt_int4_oid;  // > operator for int4
  } else {  // INT8OID
    impossible_const = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
                                Int64GetDatum(PG_INT64_MAX), false, true);
    gt_oid = oid_cache.int8_gt_int8_oid;  // > operator for int8
  }
  
  OpExpr *always_false = (OpExpr *) make_opclause(
      gt_oid, BOOLOID, false,
      (Expr *) copyObject(var_node),
      (Expr *) impossible_const,
      InvalidOid, InvalidOid);
  
  return list_make1(always_false);
}
```

**Why This Works**:
- **Equality with fraction**: `int4col > 2147483647` always false → index quickly rejects
- **Inequality with fraction**: NULL means use original, but all int values ≠ fractional → optimizer handles
- **Range with fraction**: Intelligent rounding preserves semantics while enabling index usage
- **Applies to ALL type combinations**: numeric, float4, float8 with int2, int4, int8

### 4. OID Cache Structure

**Important**: Only cache operator OIDs. Type OIDs are available as PostgreSQL constants.

**Use PostgreSQL constants for types** (from `postgres.h`):
- `INT2OID` (21), `INT4OID` (23), `INT8OID` (20)
- `FLOAT4OID` (700), `FLOAT8OID` (701), `NUMERICOID` (1700)

**Simplified OID Cache Structure**:

```c
typedef struct {
  bool initialized;
  
  // Cross-type equality operators (18 total: 9 = + 9 <>)
  // These vary by extension installation, must be cached
  Oid numeric_eq_int2_oid;
  Oid numeric_eq_int4_oid;
  Oid numeric_eq_int8_oid;
  Oid numeric_ne_int2_oid;
  // ... etc for all combinations
  
  // Commutator direction operators (18 total)
  Oid int2_eq_numeric_oid;
  Oid int4_eq_numeric_oid;
  Oid int8_eq_numeric_oid;
  // ... etc
  
  // Standard integer equality operators (needed for transformation)
  // Note: Could use constants INT2EQ_OID, INT4EQ_OID, INT8EQ_OID
  // but caching allows flexibility for future PostgreSQL versions
  Oid int2_eq_int2_oid;  // typically OID 94 (INT2EQ_OID)
  Oid int4_eq_int4_oid;  // typically OID 96 (INT4EQ_OID)
  Oid int8_eq_int8_oid;  // typically OID 410 (INT8EQ_OID)
  
} OperatorOidCache;
```

**Cast function OIDs are constants** (define as symbols):
```c
#define F_NUMERIC_INT2 1783  /* int2(numeric) cast */
#define F_NUMERIC_INT4 1744  /* int4(numeric) cast */
#define F_NUMERIC_INT8 1779  /* int8(numeric) cast */
```

**Usage**: `OidFunctionCall1(F_NUMERIC_INT4, NumericGetDatum(num));`

**Purpose**: Support function needs standard int = int operators to build transformed clauses that use existing btree indexes.

### 5. Node Copying Pattern

**Always copy Var nodes** when building transformed OpExpr:

```c
// Copy the Var node to avoid sharing pointers
Var *new_var = (Var *) copyObject(var_node);

// Create new Const with converted value
Const *new_const = makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                             Int32GetDatum(int_val), false, true);

// Build new OpExpr with copied nodes
OpExpr *new_clause = (OpExpr *) make_opclause(
    standard_eq_oid,
    BOOLOID, false,
    (Expr *) new_var,
    (Expr *) new_const,
    InvalidOid, InvalidOid);
```

**Rationale**: Original nodes may be used elsewhere in the plan tree. Always use `copyObject()` for clean isolation.

## Implementation Checklist

For each operator type combination (e.g., numeric × int4):

- [ ] Implement forward direction function (e.g., `numeric_eq_int4`)
- [ ] Implement reverse direction function (e.g., `int4_eq_numeric`)
- [ ] Add SUPPORT clause to both SQL function declarations
- [ ] Register both operators in SQL with COMMUTATOR, NEGATOR, HASHES
- [ ] Add forward OID to cache (e.g., `numeric_eq_int4_oid`)
- [ ] Add reverse OID to cache (e.g., `int4_eq_numeric_oid`)
- [ ] Update support function to recognize both OIDs
- [ ] Add type conversion logic for that combination
- [ ] Add transformation logic using appropriate standard int operator
- [ ] Test both directions: `intcol = 100::numeric` AND `100::numeric = intcol`

## Testing Patterns

### Index Scan Verification

```sql
CREATE TABLE test_idx (id SERIAL, value INT4);
CREATE INDEX idx_val ON test_idx(value);
INSERT INTO test_idx (value) SELECT generate_series(1, 10000);

SET enable_seqscan = off;  -- Force index usage for testing

-- Should show "Index Scan using idx_val"
EXPLAIN (COSTS OFF) SELECT * FROM test_idx WHERE value = 100::numeric;

-- Should also work with reversed operand order
EXPLAIN (COSTS OFF) SELECT * FROM test_idx WHERE 100::numeric = value;
```

### Expected Output

```
              QUERY PLAN              
--------------------------------------
 Index Scan using idx_val on test_idx
   Index Cond: (value = 100)
```

## Known Limitations (Phase 4)

1. **Only int4 = numeric implemented**: Other type combinations (int2, int8, float4, float8) not yet complete
2. **Only equality operators**: Inequality (<>) and ordering (<, >, <=, >=) not yet supported
3. **Performance testing pending**: Need to verify sub-millisecond execution on 1M rows

## Next Steps

1. Extend to all 9 type combinations:
   - numeric × int2
   - numeric × int8
   - float4 × int2/int4/int8
   - float8 × int2/int4/int8

2. Add inequality (<>) support function logic

3. Add ordering operator (< , >, <=, >=) support (Phase 5)

4. Performance benchmarking with 1M+ row tables

5. Complete test coverage with expected output files

## References

- **PostgreSQL like_support.c**: `src/backend/utils/adt/like_support.c` - Complete reference implementation
- **Support Nodes API**: `src/include/nodes/supportnodes.h` - SupportRequestIndexCondition definition
- **Index Path Logic**: `src/backend/optimizer/path/indxpath.c` - How planner processes support functions
- **Operator Catalog**: pg_operator system table - Operator metadata and properties
- **Type Catalog**: pg_type system table - Type OIDs and properties

## Build and Install

```bash
# Build extension
make clean && make

# Install (requires sudo)
sudo make install

# Test
psql -U postgres -c "DROP EXTENSION IF EXISTS pg_num2int_direct_comp CASCADE; CREATE EXTENSION pg_num2int_direct_comp;"

# Verify index optimization
psql -U postgres -c "CREATE TABLE test (val INT4); CREATE INDEX ON test(val); EXPLAIN SELECT * FROM test WHERE val = 100::numeric;"
```
