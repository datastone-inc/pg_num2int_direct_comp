# Research: Direct Exact Comparison for Numeric and Integer Types

**Feature**: 001-num-int-direct-comp  
**Created**: 2025-12-23  
**Purpose**: Technical research for implementing exact comparison operators between numeric and integer types in PostgreSQL

## Research Tasks

This document consolidates findings for technical decisions needed to implement the feature specification. Each section addresses unknowns from the Technical Context and evaluates implementation approaches.

## 1. Exact Comparison Implementation Strategy

**Decision**: Use C body functions that reuse PostgreSQL's existing truncation functions and min/max bounds checking

**Rationale**:
- PostgreSQL already provides robust numeric truncation via `numeric_trunc()` in `utils/numeric.h`
- Float truncation available via `trunc()` in `<math.h>` or PostgreSQL's float helpers
- Integer type bounds defined in PostgreSQL headers (`INT16_MIN`, `INT32_MIN`, `INT64_MIN`, etc.)
- Reusing existing PostgreSQL primitives ensures correctness and maintainability
- Approach: truncate inexact numeric type, check bounds, compare truncated value with integer

**Alternatives Considered**:
- **Custom precision analysis**: Would require reimplementing IEEE 754 mantissa inspection. Rejected because PostgreSQL already handles this correctly in numeric/float utilities.
- **String conversion comparison**: Would be slower and lose type safety. Rejected for performance reasons.
- **Casting both sides to highest precision type**: Would introduce the very precision loss we're trying to avoid. Rejected as it defeats the feature's purpose.

**Implementation Pattern**:
```c
// Core comparison function returning -1, 0, 1 (PostgreSQL standard pattern)
static int32
numeric_cmp_int4_internal(Numeric num, int32 i4) {
  // Truncate numeric to integer part (reuse PG's numeric_trunc)
  Numeric num_trunc = DatumGetNumeric(
    DirectFunctionCall2(numeric_trunc, NumericGetDatum(num), Int32GetDatum(0))
  );
  
  // Check bounds: if num_trunc outside int4 range → comparison based on sign
  if (numeric_is_greater_than_int4_max(num_trunc)) {
    return 1;  // numeric > int4
  }
  if (numeric_is_less_than_int4_min(num_trunc)) {
    return -1;  // numeric < int4
  }
  
  // Convert truncated numeric to int4 and compare
  int32 num_as_int = DatumGetInt32(
    DirectFunctionCall1(numeric_int4, NumericGetDatum(num_trunc))
  );
  
  if (num_as_int < i4) return -1;
  if (num_as_int > i4) return 1;
  return 0;
}

// Operator functions are thin wrappers calling the cmp function
Datum
numeric_eq_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 i4 = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) == 0);
}

Datum
numeric_lt_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 i4 = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) < 0);
}

Datum
numeric_gt_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 i4 = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) > 0);
}

// Similar pattern for <=, >=, <>
```

## 2. Index Access via SupportRequestIndexCondition

**Decision**: Implement index support using a single generic `num2int_support()` function via `SupportRequestIndexCondition`, similar to PostgreSQL's LIKE operator implementation

**Critical Discovery**: Shell operators (created by COMMUTATOR clause) cannot have support functions! Both directions must be implemented with actual function bodies.

**Rationale**:
- `SupportRequestIndexCondition` is PostgreSQL's standard mechanism for enabling index-optimized predicates
- LIKE operator (`like_support.c`) provides proven pattern for type-crossing index access
- Support function **MUST return a List of OpExpr nodes** (not a single OpExpr), following PostgreSQL's `match_pattern_prefix()` pattern
- Support function uses `list_make1()` to wrap the transformed OpExpr in a List
- A single generic support function handles all type combinations by inspecting operator OID and operand types
- Reduces code duplication - one support function instead of 54 separate functions
- Uses lazy OID cache to identify which operator is being invoked
- Allows query planner to recognize `intcol = 10.0::numeric` as indexable predicate
- Returns transformed clause that planner can use for index scan instead of seq scan

**Implementation Requirements**:
1. **Both operator directions need real functions**: For `int4 = numeric`, must implement BOTH:
   - `numeric = int4` with function `numeric_eq_int4()` + SUPPORT clause
   - `int4 = numeric` with function `int4_eq_numeric()` + SUPPORT clause (commutator)
2. **COMMUTATOR alone creates shell operators**: Shell operators have no function, therefore no support function attachment point
3. **Support function return type**: Must return `List *` containing transformed OpExpr nodes
4. **Use PostgreSQL's casting functions**: Use built-in `int2(numeric)` (OID 1783), `int4(numeric)` (OID 1744), `int8(numeric)` (OID 1779) for safe type conversion with rounding
5. **Set lossy flag**: `req->lossy = false` for exact transformations

**Alternatives Considered**:
- **Separate support function per operator**: Would require 54 functions with nearly identical logic. Rejected due to excessive duplication.
- **Custom operator class**: Would require defining complete btree operator families. Rejected as unnecessarily complex since integer types already have btree support.
- **Planner hooks**: Would be fragile across PostgreSQL versions. Rejected for maintainability.
- **No index support**: Would make extension unusable for large tables. Rejected per User Story 2 requirements.
- **Shell operators with support**: Attempted but failed - shell operators have no function implementation, so SUPPORT clause cannot be attached.

**Implementation Pattern**:
```c
// Generic support function for all exact comparison operators
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  Node *ret = NULL;
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;
    
    // Set lossy flag - our transformation is exact
    req->lossy = false;
    
    // Initialize OID cache if needed
    init_oid_cache(&oid_cache);
    
    // Extract the comparison: intcol = <numeric_constant>
    if (is_opclause(req->node)) {
      OpExpr *clause = (OpExpr *) req->node;
      Oid opno = clause->opno;
      
      // Extract Var and Const nodes
      Var *var_node;
      Const *const_node;
      // ... identify which is which
      
      // Check if this is one of our operators
      if (opno == oid_cache.int4_eq_numeric_oid || /* other operators... */) {
        // Convert numeric constant to integer using PostgreSQL's casting functions
        // For int4: OidFunctionCall1(1744, NumericGetDatum(num))  // int4(numeric)
        // For int2: OidFunctionCall1(1783, NumericGetDatum(num))  // int2(numeric)
        // For int8: OidFunctionCall1(1779, NumericGetDatum(num))  // int8(numeric)
        
        // Create new Const with converted integer value
        Const *new_const = makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                                     Int32GetDatum(int_val), false, true);
        
        // Build new OpExpr using standard int = int operator
        Var *new_var = (Var *) copyObject(var_node);
        OpExpr *new_clause = (OpExpr *) make_opclause(
            oid_cache.int4_eq_int4_oid,  // Standard int4 = int4 operator (OID 96)
            BOOLOID, false,
            (Expr *) new_var,
            (Expr *) new_const,
            InvalidOid, InvalidOid);
        
        // CRITICAL: Return a List containing the transformed OpExpr
        ret = (Node *) list_make1(new_clause);
      }
    }
  }
  
  PG_RETURN_POINTER(ret);
}
```

**SQL Function Declarations** (both directions required):
```sql
-- Forward direction with support
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

-- Commutator direction also needs real function + support
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

**C Commutator Functions** (swap arguments and reuse core logic):
```c
// Commutator function: int4 = numeric → calls numeric = int4 logic with swapped args
Datum
int4_eq_numeric(PG_FUNCTION_ARGS) {
  int32 i4 = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  // Reuse existing comparison function with swapped args
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) == 0);
}
```

**Key PostgreSQL References**:
- `src/backend/utils/adt/like_support.c` - LIKE operator index support pattern
- `src/include/nodes/supportnodes.h` - `SupportRequestIndexCondition` definition
- `src/backend/optimizer/path/indxpath.c` - How planner processes support functions

## 3. Lazy OID Cache for Operator Lookups

**Decision**: Implement per-backend lazy cache of operator OIDs, initialized on first access with fast lookup thereafter

**Rationale**:
- Operator OID lookups via catalog queries are expensive (involves syscache lookups)
- Each comparison could trigger OID resolution if not cached
- Lazy initialization avoids startup overhead for backends not using these operators
- Per-backend cache is safe (OIDs don't change within a backend session)

**Alternatives Considered**:
- **Lookup on every call**: Too slow, would defeat performance goals. Rejected.
- **Shared memory cache**: Unnecessary complexity, OIDs are session-stable. Rejected.
- **Preprocessor OID hardcoding**: Fragile across PostgreSQL installations. Rejected.

**Implementation Pattern**:
```c
// Static cache structure (per-backend)
typedef struct {
  bool initialized;
  Oid numeric_eq_int2_oid;
  Oid numeric_eq_int4_oid;
  Oid numeric_eq_int8_oid;
  // ... other operators
} OperatorOidCache;

static OperatorOidCache oid_cache = {.initialized = false};

static void
init_oid_cache(void) {
  if (oid_cache.initialized) {
    return;
  }
  
  // Lookup OIDs via catalog (one-time per backend)
  oid_cache.numeric_eq_int2_oid = 
    get_opfamily_member(...);
  // ... initialize all operator OIDs
  
  oid_cache.initialized = true;
}

// Use in support functions
Datum
some_support_func(PG_FUNCTION_ARGS) {
  init_oid_cache();  // Fast no-op if already initialized
  
  if (clause->opno == oid_cache.numeric_eq_int4_oid) {
    // Handle numeric = int4 case
  }
  // ...
}
```

## 4. Merge Join Support (Deferred to v2.0)

**Decision**: Do not add operators to btree operator families in v1.0. Therefore, operators do not have MERGES property.

**Rationale for Deferral**:
- Operators are mathematically transitive for the higher precision type and could be safely added to the higher precision btree families.
  - TBD add "proof" of this. 
- This would enable planner inferences, e.g., transitivity, and merge join optimization for large table joins
- However, it adds implementation complexity, need to add operators to appropriate btree operator families for each type:
  - numeric × int operators → `numeric_ops` family
  - float4 × int operators → `float4_ops` family  
  - float8 × int operators → `float8_ops` family
  - Additional testing required to validate planner behavior
  - Need to verify interaction with native PostgreSQL operators in same family
- Index optimization via `SupportRequestIndexCondition` already provides excellent performance for most use cases
- Merge joins primarily benefit queries joining very large tables where indexes aren't selective

**Why Operators Are Transitive**:
- If `A = B` (no fractional part) and `B = C`, then `A = C`
- If `A = B` returns false (has fractional part), transitive chain correctly propagates inequality
- Example: `10.5 = 10` → false, so `(10.5 = 10) AND (10 = X)` → false regardless of X
- Native PostgreSQL's exact numeric comparison is also transitive: `10.5::numeric = 10::numeric` → false

**Merge Join Requirements**:
- For `intcol = float4col`, both columns would use float4_ops family operators for sorting
- For `intcol = float8col`, both columns would use float8_ops family operators for sorting
- Each type combination needs operators added to the appropriate family (numeric_ops, float4_ops, float8_ops)
- This is exactly how native PostgreSQL handles cross-type comparisons
- For `intcol = numcol`, both columns would use numeric_ops family operators for sorting
- The integer column would be implicitly cast to numeric for comparison
- This is exactly how native PostgreSQL handles `int = numeric` with implicit casting

**v1.0 Implementation**: Operators have only basic properties (COMMUTATOR, NEGATOR, RESTRICT, JOIN). No MERGES property, not in any operator families.

**v2.0 numeric × int operators to numeric_ops btree family
ALTER OPERATOR FAMILY numeric_ops USING btree ADD
  OPERATOR 1 < (int4, numeric),
  OPERATOR 2 <= (int4, numeric),
  OPERATOR 3 = (int4, numeric),
  OPERATOR 4 >= (int4, numeric),
  OPERATOR 5 > (int4, numeric);
-- Repeat for int2, int8, and reverse directions (numeric, int)

-- Add float4 × int operators to float4_ops btree family
ALTER OPERATOR FAMILY float4_ops USING btree ADD
  OPERATOR 1 < (int4, float4),
  OPERATOR 2 <= (int4, float4),
  OPERATOR 3 = (int4, float4),
  OPERATOR 4 >= (int4, float4),
  OPERATOR 5 > (int4, float4);
-- Repeat for int2, int8, and reverse directions (float4, int)

-- Add float8 × int operators to float8_ops btree family
ALTER OPERATOR FAMILY float8_ops USING btree ADD
  OPERATOR 1 < (int4, float8),
  OPERATOR 2 <= (int4, float8),
  OPERATOR 3 = (int4, float8),
  OPERATOR 4 >= (int4, float8),
  OPERATOR 5 > (int4, float8);
-- Repeat for int2, int8, and reverse directions (float8, int)

-- Add MERGES property to operators
ALTER OPERATOR = (int4, numeric) SET (MERGES);
ALTER OPERATOR = (int4, float4) SET (MERGES);
ALTER OPERATOR = (int4, float8) SET (MERGES);
-- Repeat for all type combinations and comparison operators
-- Add MERGES property to operators
ALTER OPERATOR = (int4, numeric) SET (MERGES);
```

## 5. Hash Function Support for Hash Joins (Future Enhancement)

**Decision**: Defer hash function implementation to v2.0; operators will not have HASHES property in v1.0

**Rationale**:
- Hash joins require hash functions where "if a = b then hash(a) = hash(b)" across types
- This is implementable but adds complexity (~100 lines per type combination)
- Index scan optimization via support functions already provides excellent performance
- Hash joins are beneficial for large table joins, but most queries will use indexes
- Can be added in a minor version without breaking compatibility

**Implementation Approach (Future)**:
```c
// Hash function for numeric values in cross-type equality
Datum numeric_hash_for_int_eq(PG_FUNCTION_ARGS) {
    Numeric num = PG_GETARG_NUMERIC(0);
    
    // Check if value has fractional part
    Numeric trunc_num = DatumGetNumeric(
        DirectFunctionCall2(numeric_trunc, NumericGetDatum(num), Int32GetDatum(0))
    );
    
    bool has_fraction = !DatumGetBool(
        DirectFunctionCall2(numeric_eq, NumericGetDatum(num), NumericGetDatum(trunc_num))
    );
    
    if (has_fraction) {
        // Has fractional part - use standard numeric hash
        return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
    }
    
    // No fractional part - convert to int64 and hash as integer
    // This ensures 10::int4 and 10.0::numeric hash to the same value
    int64 as_int = DatumGetInt64(
        DirectFunctionCall1(numeric_int8, NumericGetDatum(trunc_num))
    );
    return DirectFunctionCall1(hashint8, Int64GetDatum(as_int));
}
```

**SQL Registration (Future)**:
```sql
-- Register hash function with operator
CREATE FUNCTION numeric_hash_for_int_eq(numeric) 
RETURNS int4 AS '$libdir/pg_num2int_direct_comp' LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES  -- Can be added in v2.0 once hash functions implemented
  -- Note: not added to btree opfamily to prevent transitivity
);
```

## 6. Preventing Transitive Inference

**Decision**: Do not add these operators to btree operator families that would enable transitive inference across different type combinations

**Rationale**:
- pg_regress is PostgreSQL's standard testing framework (constitutional requirement)
- Separate test files improve maintainability and debugging
- EXPLAIN output in tests verifies index usage (critical for User Story 2)
- Expected output files ensure regression detection

**Test Categories**:
1. **Operator correctness**: Basic equality/inequality/ordering tests for all type combinations
2. **Index usage**: EXPLAIN tests verifying Index Scan vs Seq Scan
3. **NULL handling**: NULL propagation per SQL standard
4. **Special values**: NaN, Infinity, -Infinity per IEEE 754
5. **Edge cases**: Precision boundaries, overflow, type coercion ambiguity

**Example Test**:
```sql
-- sql/index_usage.sql
CREATE TABLE test_int4 (id SERIAL, value INT4);
CREATE INDEX idx_value ON test_int4(value);
INSERT INTO test_int4 (value) SELECT generate_series(1, 1000);

-- Should use index scan
EXPLAIN (COSTS OFF) 
SELECT * FROM test_int4 WHERE value = 500.0::numeric;

-- Expected output should show "Index Scan using idx_value"
```

## 6. Type Combinations Matrix

**Decision**: Implement full matrix of 9 type combinations: (numeric, float4, float8) × (int2, int4, int8)

**Rationale**:
- User Story 5 requires complete coverage to avoid "why doesn't X work?" questions
- Each type pairing has unique conversion logic (different precision boundaries)
- Completeness improves extension usability

**Type Pair Details**:
- **numeric × int2/int4/int8**: Check fractional part via truncation, range check, exact comparison
- **float4 × int2/int4/int8**: Check if float value equals `floor(float)`, range check based on mantissa precision
- **float8 × int2/int4/int8**: Similar to float4 but with double precision mantissa

**Float Precision Boundaries**:
- float4 (32-bit IEEE 754): 24-bit mantissa → exactly represents integers up to 2^24 (16,777,216)
- float8 (64-bit IEEE 754): 53-bit mantissa → exactly represents integers up to 2^53 (9,007,199,254,740,992)

**Critical Implementation Detail**: Float comparison functions must detect when a float value cannot exactly represent an integer, even when the integer-to-float conversion produces the same rounded value as the original float. For example:
- `9007199254740993::float8` rounds to `9007199254740992` (loses precision)
- `9007199254740993::int8` converts to `9007199254740992::float8` (same rounding)
- These should NOT compare as equal because the float cannot represent the exact integer
- Solution: Check both fractional parts AND verify integer-to-float conversion preserves exact value

## Summary of Technical Approach

The extension implementation strategy is:

1. **Comparison functions**: C functions using PostgreSQL's `numeric_trunc()` and bounds checking primitives
2. **Index integration**: `SupportRequestIndexCondition` support functions modeled after LIKE operator
3. **Performance optimization**: Lazy per-backend OID cache for operator lookups
4. **Transitivity prevention**: Operators are NOT in btree operator families, which also means MERGES property cannot be used (MERGES requires btree family membership)
5. **Testing**: pg_regress framework with categorized test suites
6. **Type coverage**: Full 9-combination matrix with type-specific precision handling

All decisions align with the constitution's requirements for code style (K&R C), testing (pg_regress), build system (PGXS), and documentation standards.
## 7. Implementation Discoveries and Critical Lessons

**Discoveries Made During Phase 4 (Index Optimization)**:

### 7.1 Shell Operators Cannot Have Support Functions

**Problem Encountered**: Initially attempted to use COMMUTATOR clause to automatically create reverse operators:
```sql
CREATE OPERATOR = (
  LEFTARG = numeric, RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,  -- Automatically creates int4 = numeric
  SUPPORT num2int_support
);
```

**Root Cause**: The COMMUTATOR clause creates a "shell operator" - an operator entry in `pg_operator` with no associated function implementation. Shell operators exist purely as metadata placeholders for the optimizer.

**Impact**: Shell operators have no `oprcode` (function OID), therefore the SUPPORT clause cannot be attached. When PostgreSQL plans a query like `WHERE intcol = 100::numeric`, it looks up the `int4 = numeric` operator, finds it's a shell, and has no support function to call.

**Solution**: Both operator directions must be implemented with actual C functions:
```sql
-- Forward direction
CREATE FUNCTION numeric_eq_int4(numeric, int4) ...
SUPPORT num2int_support;

CREATE OPERATOR = (
  LEFTARG = numeric, RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =
);

-- Reverse direction (commutator) - MUST have real function
CREATE FUNCTION int4_eq_numeric(int4, numeric) ...
SUPPORT num2int_support;

CREATE OPERATOR = (
  LEFTARG = int4, RIGHTARG = numeric,
  FUNCTION = int4_eq_numeric,
  COMMUTATOR = =
);
```

**C Implementation**: Commutator functions simply swap arguments and reuse core comparison logic:
```c
Datum int4_eq_numeric(PG_FUNCTION_ARGS) {
  int32 i4 = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(numeric_cmp_int4_internal(num, i4) == 0);
}
```

**Lesson**: For index-optimized operators, every direction needs a real function implementation. COMMUTATOR alone is insufficient.

### 7.2 Support Functions Return Lists, Not Single Nodes

**Problem Encountered**: Initial implementation returned a single `OpExpr *`:
```c
OpExpr *new_clause = make_opclause(...);
ret = (Node *) new_clause;  // WRONG - causes segfault
PG_RETURN_POINTER(ret);
```

**Symptom**: Server crashed during EXPLAIN after support function returned successfully. Logs showed transformation completed but then PostgreSQL terminated abnormally.

**Root Cause**: PostgreSQL's index condition API expects support functions to return a `List *` of condition nodes, not a bare node pointer. This is documented in `src/backend/utils/adt/like_support.c` but easy to miss.

**Correct Implementation**:
```c
OpExpr *new_clause = make_opclause(...);
ret = (Node *) list_make1(new_clause);  // Wrap in List
PG_RETURN_POINTER(ret);
```

**PostgreSQL Pattern** (from `like_support.c:375-380`):
```c
if (pstatus == Pattern_Prefix_Exact) {
    expr = make_opclause(eqopr, BOOLOID, false,
                         (Expr *) leftop, (Expr *) prefix,
                         InvalidOid, indexcollation);
    result = list_make1(expr);  // ← Always wrap in list
    return result;
}
```

**Lesson**: `SupportRequestIndexCondition` returns a List because some transformations generate multiple conditions (e.g., LIKE 'abc%' → `col >= 'abc' AND col < 'abd'`). Even single-condition cases must use `list_make1()`.

### 7.3 Numeric Type Conversion Must Use PostgreSQL's Cast Functions

**Problem Encountered**: Attempted to use `numeric_int4_opt_error()` for conversion:
```c
int32 val = numeric_int4_opt_error(num, &is_valid);
// is_valid=0 even for exact integers like 100.0!
```

**Root Cause**: The `_opt_error` variants fail (return `is_valid=false`) if the numeric has ANY fractional digits, even `.0`. This is by design - they're for detecting precision loss.

**Symptom**: Transformation failed for all numeric constants, even exact integers. Logs showed `Converted to int4: 100, is_valid=0`.

**Solution**: Use PostgreSQL's standard casting functions via `OidFunctionCall1`:
```c
// For int2: OID 1783
Datum result = OidFunctionCall1(1783, NumericGetDatum(num));
int16 val = DatumGetInt16(result);

// For int4: OID 1744
Datum result = OidFunctionCall1(1744, NumericGetDatum(num));
int32 val = DatumGetInt32(result);

// For int8: OID 1779  
Datum result = OidFunctionCall1(1779, NumericGetDatum(num));
int64 val = DatumGetInt64(result);
```

**Behavior**: These functions perform rounding (banker's rounding):
- `100.0::numeric::int4` → `100`
- `100.5::numeric::int4` → `101` (rounds to even)
- `100.4::numeric::int4` → `100`
- `-100.5::numeric::int4` → `-101`

**Lesson**: For index optimization, use the same casting behavior as regular PostgreSQL casts. The `_opt_error` functions are too strict for this use case.

**Handling All Comparison Operators**: The support function must handle ALL comparison operators (=, <>, <, <=, >, >=) for complete index optimization:

1. **Equality with fractional values** (e.g., `int4col = 10.5`): Return always-false condition `int4col > PG_INT32_MAX` to use index
2. **Inequality with fractional values** (e.g., `int4col <> 10.5`): Return NULL (use original condition, all rows match)
3. **Range operators with fractional values**: Transform with intelligent rounding:
   - `int4col < 10.5` → `int4col <= 10` (round down for <)
   - `int4col <= 10.5` → `int4col <= 10` (round down)
   - `int4col > 10.5` → `int4col >= 11` (round up for >)
   - `int4col >= 10.5` → `int4col >= 11` (round up)
4. **Out of range values**: Return always-false (for = and range) or NULL (for <>)

This pattern applies to ALL type combinations: numeric/float4/float8 with int2/int4/int8.

### 7.4 OID Cache Should Only Store Operator OIDs

**Important Clarification**: Type OIDs for built-in types are available as **PostgreSQL constants** and don't need caching:
- Use `INT2OID` (21), `INT4OID` (23), `INT8OID` (20)
- Use `FLOAT4OID` (700), `FLOAT8OID` (701), `NUMERICOID` (1700)
- These constants are defined in `postgres.h` and are stable

**The OID cache should only contain**:
1. **Cross-type operator OIDs**: All 54 comparison operators (9 type pairs × 6 operators) - these vary by extension installation
2. **Standard int equality OIDs**: `int2_eq_int2_oid` (94), `int4_eq_int4_oid` (96), `int8_eq_int8_oid` (410) - needed for building transformed clauses

**Cast function OIDs are also constants** (can be hardcoded):
- `int2(numeric)` = 1783, `int4(numeric)` = 1744, `int8(numeric)` = 1779

**Implementation Note**: Define symbolic constants to avoid magic numbers throughout the code. PostgreSQL does not define these in its headers (pg_proc_d.h, pg_operator_d.h), so we use the NUM2INT_ prefix to avoid potential future conflicts:
```c
/* PostgreSQL built-in cast function OIDs (stable across versions) */
#define NUM2INT_CAST_NUMERIC_INT2 1783  /* int2(numeric) */
#define NUM2INT_CAST_NUMERIC_INT4 1744  /* int4(numeric) */
#define NUM2INT_CAST_NUMERIC_INT8 1779  /* int8(numeric) */

/* Standard btree operator OIDs for transformed clauses */
#define NUM2INT_INT2EQ_OID   94   /* int2 = int2 */
#define NUM2INT_INT4EQ_OID   96   /* int4 = int4 */
#define NUM2INT_INT8EQ_OID  410   /* int8 = int8 */
#define NUM2INT_INT2LT_OID   95   /* int2 < int2 */
#define NUM2INT_INT4LT_OID   97   /* int4 < int4 */
#define NUM2INT_INT8LT_OID  412   /* int8 < int8 */
#define NUM2INT_INT2GT_OID  520   /* int2 > int2 */
#define NUM2INT_INT4GT_OID  521   /* int4 > int4 */
#define NUM2INT_INT8GT_OID  413   /* int8 > int8 */
#define NUM2INT_INT2LE_OID  522   /* int2 <= int2 */
#define NUM2INT_INT4LE_OID  523   /* int4 <= int4 */
#define NUM2INT_INT8LE_OID  414   /* int8 <= int8 */
#define NUM2INT_INT2GE_OID  524   /* int2 >= int2 */
#define NUM2INT_INT4GE_OID  525   /* int4 >= int4 */
#define NUM2INT_INT8GE_OID  415   /* int8 >= int8 */
#define NUM2INT_INT2NE_OID  519   /* int2 <> int2 */
#define NUM2INT_INT4NE_OID  518   /* int4 <> int4 */
#define NUM2INT_INT8NE_OID  411   /* int8 <> int8 */
```

The support function needs standard int equality operators to build the transformed clause using existing btree indexes.

### 7.5 Node Copying is Required

**Implementation Detail**: When building the transformed OpExpr, the Var node must be copied:
```c
Var *new_var = (Var *) copyObject(var_node);  // Don't reuse original pointer
```

**Rationale**: The original OpExpr node may be used elsewhere in the plan tree. Sharing node pointers can cause issues during plan optimization or execution. Always use `copyObject()` for nodes that will be modified or used in new contexts.

**Lesson**: PostgreSQL's planner expects independent node trees for different clauses. Use `copyObject()` from `nodes/nodeFuncs.h` to ensure clean copies.

## Summary of Implementation Requirements

Based on implementation experience:

1. **Bidirectional function implementations**: Every operator needs both directions with real C functions (COMMUTATOR alone insufficient)
2. **Support function return type**: Must return `List *` via `list_make1()`, not bare node pointers
3. **Type conversion**: Use `OidFunctionCall1()` with PostgreSQL's standard cast function OIDs (1783, 1744, 1779)
4. **OID cache coverage**: Include type OIDs, cross-type operator OIDs, and standard int equality OIDs
5. **Node copying**: Use `copyObject()` for Var nodes in transformed clauses
6. **Lossy flag**: Set `req->lossy = false` for exact transformations
7. **Reference implementation**: Study `src/backend/utils/adt/like_support.c` for complete pattern