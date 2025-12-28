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

## 4. Btree Operator Family Support (Implemented in v1.0)

**Decision**: Add operators to btree operator families (`numeric_ops` and `float_ops`) but do NOT add MERGES property and do NOT add to `integer_ops` family.

**Rationale**:
- Operators are mathematically transitive and safe for btree families
- Adding to higher-precision families (`numeric_ops`, `float_ops`) enables critical optimization: **indexed nested loop joins**
- However, MERGES property and `integer_ops` membership would cause problematic transitive inference from integer side
- This approach balances optimization benefits with correctness constraints

### 4.1 Why Operators Are Transitive and Safe

**Mathematical Proof**:
- If `A = B` (no fractional part) and `B = C`, then `A = C` ✓
- If `A = B` returns false (has fractional part), transitive chain correctly propagates inequality
- Example: `10.5 = 10` → false, so `(10.5 = 10) AND (10 = X)` → false regardless of X
- Native PostgreSQL's exact numeric comparison is also transitive: `10.5::numeric = 10::numeric` → false

**Safe when added to higher-precision families**:
- Adding `numeric × int` operators to `numeric_ops` family is safe
- Adding `float × int` operators to `float_ops` family is safe
- Transitivity works correctly because all comparisons happen at the higher precision

### 4.2 Why NOT to Add to integer_ops Family

**Critical Problem**: Adding cross-type operators to `integer_ops` would enable invalid transitive inference:

**Example of problematic inference**:
```sql
-- If operators were in integer_ops family:
SELECT * FROM t 
WHERE int_col = 10 
  AND int_col = numeric_col 
  AND numeric_col = 10.5;

-- Planner could infer: int_col = 10.5 (WRONG!)
-- This is false because 10 ≠ 10.5
```

**Why this happens**:
- With operators in `integer_ops`, planner sees: `int_col = 10` and `int_col = numeric_col` both in same family
- Planner assumes transitivity within the family: if `A = B` and `B = C` then `A = C`
- But this breaks when C (numeric_col) has fractional values that don't match the integer

**Solution**: Keep operators ONLY in `numeric_ops` and `float_ops`, NOT in `integer_ops`


### 4.3 Merge Join Implementation Path (Planned for v1.0.0)

**Decision**: Merge join support is planned for v1.0.0. This will require adding cross-type operators to the `integer_ops` family, with careful safeguards to prevent invalid transitive inference by the planner.

**Implementation Plan**:
- Add cross-type operators to `integer_ops` with custom support functions and planner hooks to ensure only valid inferences are made.
- Thoroughly test for edge cases where the planner could incorrectly infer `int_col = 10.5` from `int_col = 10 AND int_col = numeric_col AND numeric_col = 10.5`.
- Document all constraints and provide regression tests for transitivity and merge join correctness.

**Why this is challenging**:
- Merge join requires both input relations to be sorted by the same operator family.
- Adding to `integer_ops` risks planner making invalid inferences unless carefully controlled.
- The implementation will include additional planner logic and documentation to ensure correctness.

### 4.4 Benefit of Btree Family Membership: Indexed Nested Loop Joins

**Key Optimization**: Adding operators to `numeric_ops` and `float_ops` enables **indexed nested loop joins** with cross-type predicates

**Without btree family membership**:
```sql
EXPLAIN SELECT COUNT(*) 
FROM test_int i JOIN test_numeric n ON i.val = n.val 
WHERE i.val < 100;

                    QUERY PLAN                    
--------------------------------------------------
 Aggregate
   ->  Hash Join
         Hash Cond: ((i.val)::numeric = n.val)
         ->  Index Scan on test_int
         ->  Hash
               ->  Seq Scan on test_numeric
```
- Uses Hash Join
- Cannot use index on numeric column for join condition
- Seq scan on numeric table

**With btree family membership** (operators in numeric_ops):
```sql
                    QUERY PLAN                            
----------------------------------------------------------
 Aggregate
   ->  Nested Loop
         ->  Index Only Scan on test_int i
               Index Cond: (val < 100)
         ->  Index Only Scan on test_numeric n
               Index Cond: (val = i.val)  ← KEY BENEFIT!
```
- Uses indexed Nested Loop
- **Can use index on numeric column** with cross-type predicate `val = i.val`
- Both sides use index scans
- Much more efficient for selective queries

**How it works**:
1. Scan int table with index condition `val < 100`
2. For each int row, use btree index on numeric table to find matching rows
3. Index condition `numeric.val = int.val` is recognized because operators are in `numeric_ops` family
4. PostgreSQL knows how to use numeric btree index with the cross-type equality

**Performance impact**:
- Enables index-only scans on both sides of join
- Avoids hash table construction
- Dramatically faster for selective queries (small result sets)
- Similar to how native PostgreSQL handles `int4 = int8` joins

### 4.5 Implementation in v1.0

**What was implemented**:

### 4.5 Implementation in v1.0

**What was implemented**:

1. **Created SQL-callable btree support functions**:
```c
// Comparison functions returning int (-1, 0, 1) for btree
PG_FUNCTION_INFO_V1(numeric_cmp_int2);
PG_FUNCTION_INFO_V1(numeric_cmp_int4);
PG_FUNCTION_INFO_V1(numeric_cmp_int8);
PG_FUNCTION_INFO_V1(float4_cmp_int2);
PG_FUNCTION_INFO_V1(float4_cmp_int4);
PG_FUNCTION_INFO_V1(float4_cmp_int8);
PG_FUNCTION_INFO_V1(float8_cmp_int2);
PG_FUNCTION_INFO_V1(float8_cmp_int4);
PG_FUNCTION_INFO_V1(float8_cmp_int8);
```

2. **Registered operators with btree families**:
```sql
-- Add numeric × int operators to numeric_ops btree family
ALTER OPERATOR FAMILY numeric_ops USING btree ADD
  -- numeric <op> int2 with support function
  FUNCTION 1 (numeric, int2) numeric_cmp_int2(numeric, int2),
  OPERATOR 1 < (numeric, int2),
  OPERATOR 2 <= (numeric, int2),
  OPERATOR 3 = (numeric, int2),
  OPERATOR 4 >= (numeric, int2),
  OPERATOR 5 > (numeric, int2),
  
  -- numeric <op> int4 with support function
  FUNCTION 1 (numeric, int4) numeric_cmp_int4(numeric, int4),
  OPERATOR 1 < (numeric, int4),
  -- ... (repeat for all 6 type combinations: numeric×int2/4/8 both directions)
  
-- Add float4/8 × int operators to float_ops btree family
ALTER OPERATOR FAMILY float_ops USING btree ADD
  FUNCTION 1 (float4, int2) float4_cmp_int2(float4, int2),
  OPERATOR 1 < (float4, int2),
  -- ... (repeat for float4×int2/4/8 and float8×int2/4/8, both directions)
```

**What was NOT implemented** (and why):
- ❌ **MERGES property**: Cannot work without operators in `integer_ops` family
- ❌ **integer_ops family membership**: Would cause invalid transitive inference
- ❌ **Merge join optimization**: Requires same-family membership on both sides

**Result**:
- ✅ Operators enable indexed nested loop joins (major performance win)
- ✅ Operators are safe from transitive inference problems
- ✅ All 10 regression tests pass with improved query plans
- ✅ Hash joins still work when needed (planner chooses based on statistics)

### 4.6 Summary: Why This Approach is Optimal

**Comparison of approaches**:

| Approach | Indexed NL Join | Merge Join | Transitive Safety | Implementation |
|----------|-----------------|------------|-------------------|----------------|
| No btree families | ❌ No | ❌ No | ✅ Safe | Simple |
| In numeric_ops + float_ops only | ✅ Yes | ❌ No | ✅ Safe | **CHOSEN** |
| In all families (including integer_ops) | ✅ Yes | ✅ Yes | ❌ **UNSAFE** | Complex |

**Why the chosen approach is best**:
1. Enables the most critical optimization: indexed nested loop joins
2. Maintains correctness (no transitive inference bugs)
3. Works for the most common query patterns (selective joins with indexes)
4. Simpler than full merge join support
5. Merge joins aren't needed for most real-world queries with our optimization

**When each join strategy is used**:
- **Indexed Nested Loop**: Small selective result sets (most queries) - NOW OPTIMIZED ✅
- **Hash Join**: Large non-selective joins without indexes - still works ✅
- **Merge Join**: Large joins with sorted inputs - not available, but rarely needed ⚠️

## 5. Hash Function Support for Hash Joins (Implemented in v1.0)

**Decision**: Implement hash functions by casting integers to the higher-precision type and using existing PostgreSQL hash functions

**Rationale**:
- Hash joins require "if a = b then hash(a) = hash(b)" across types
- Initially considered complex logic to detect fractional parts, but discovered a simpler approach
- PostgreSQL's `hash_numeric()` accepts integers via implicit cast: `hash_numeric(10::int4)` works
- PostgreSQL's float hash functions are consistent: `hashfloat4(10.0) = hashfloat8(10.0)`
- This eliminates need for custom hash logic - just cast and use existing functions

**Key Insight**:
Testing revealed that PostgreSQL's native cross-type operators (e.g., `int2 = int8` in `integer_ops`) only register same-type hash functions, yet hash joins still work. This validates our approach:
```sql
-- PostgreSQL only has hashint2(int2), hashint4(int4), hashint8(int8)
-- Yet int2 = int8 hash joins work via implicit casting
SELECT hashint4(10), hash_numeric(10::int4), hash_numeric(10.0::numeric);
  hashint4   | hash_numeric | hash_numeric 
-------------+--------------+--------------
 -1905060026 |   1324868424 |   1324868424  -- int4→numeric cast works!
```

**Implementation Approach**:
Create wrapper functions that cast integers to the family's base type:

```c
/* Hash int4 as numeric - for numeric_ops hash family */
PG_FUNCTION_INFO_V1(hash_int4_as_numeric);
Datum
hash_int4_as_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
}

/* Hash int4 as float8 - for float_ops hash family */
PG_FUNCTION_INFO_V1(hash_int4_as_float8);
Datum
hash_int4_as_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = (float8) ival;
  return DirectFunctionCall1(hashfloat8, Float8GetDatum(fval));
}
```

**Hash Family Registration**:
Add operators to `numeric_ops` and `float_ops` hash families with wrapper functions:

```sql
-- Add to numeric_ops hash family
ALTER OPERATOR FAMILY numeric_ops USING hash ADD
  -- Hash functions for integer types (cast to numeric)
  FUNCTION 1 (int2, int2) hash_int2_as_numeric(int2),
  FUNCTION 2 (int2, int2) hash_int2_as_numeric_extended(int2, int8),
  FUNCTION 1 (int4, int4) hash_int4_as_numeric(int4),
  FUNCTION 2 (int4, int4) hash_int4_as_numeric_extended(int4, int8),
  FUNCTION 1 (int8, int8) hash_int8_as_numeric(int8),
  FUNCTION 2 (int8, int8) hash_int8_as_numeric_extended(int8, int8),
  
  -- Operators
  OPERATOR 1 = (numeric, int2),
  OPERATOR 1 = (int2, numeric),
  OPERATOR 1 = (numeric, int4),
  OPERATOR 1 = (int4, numeric),
  OPERATOR 1 = (numeric, int8),
  OPERATOR 1 = (int8, numeric);

-- Note: Use float8 wrappers for float_ops (not float4) because
-- PostgreSQL's float_ops already ensures hashfloat4(x) = hashfloat8(x)
```

**HASHES Property**:
All 18 equality operators now have the HASHES property, enabling hash joins:

```sql
CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES  -- Implemented in v1.0
);
```

**Benefits**:
- Hash joins work for large table joins where nested loop would be inefficient
- Planner can choose between nested loop (indexed), hash join, or sequential based on statistics
- No custom hash logic needed - reuses PostgreSQL's battle-tested hash functions
- Consistent with PostgreSQL's approach for native cross-type operators

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

---

## 8. Critical User Insights: Design Breakthroughs

This section documents the key insights that transformed the project from a basic operator set into a fully-optimized PostgreSQL extension with proper join strategies and semantic guarantees.

### 8.1 Btree Operator Family Membership Discovery

**Insight**: Adding operators to `numeric_ops` and `float_ops` btree families enables **indexed nested loop joins** without needing the MERGES property.

**Impact**:
- Operators benefit from btree indexes via family membership alone
- Index Cond predicates work even without MERGES property
- No need to add operators to `integer_ops` family (which would break correctness)
- Enables efficient indexed lookups: `WHERE int_col = numeric_constant` uses btree index

**Key Realization**: Family membership + support functions = index optimization. MERGES property is only needed for merge join strategy, which we cannot use (see 8.2).

### 8.2 Merge Joins Are Architecturally Impossible

**Insight**: Merge joins **cannot be implemented** due to fundamental semantic constraints.

**Reasoning**:
- Merge joins require operators in `integer_ops` btree family
- Adding to `integer_ops` enables transitive inference
- Invalid inference example: `int_col = 10` AND `10 = 10.5::numeric` → planner infers `int_col = 10.5` ❌
- This violates exact comparison semantics (integers can never equal fractional values)

**Design Decision**: Document as "Not Possible" rather than "Future Enhancement"
- Architectural constraint, not implementation limitation
- Trade-off: Preserve semantic correctness over merge join optimization
- Alternative: Hash joins and indexed nested loop joins provide sufficient performance

### 8.3 Hash Function Casting Strategy

**Insight**: PostgreSQL's `hash_numeric(10::int4) = hash_numeric(10.0::numeric)` works automatically!

**Discovery**:
- PostgreSQL's hash functions naturally handle casted integers
- Casting integers to numeric/float before hashing ensures consistency
- No need for complex fractional detection logic in hash functions

**Implementation**:
```c
// Simple wrapper: cast int → numeric → hash
Datum hash_int4_as_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
}
```

**Result**: 18 clean wrapper functions leveraging PostgreSQL's existing hash infrastructure, enabling hash joins for all type combinations.

### 8.4 Operator Family Strategy for Correctness

**Insight**: Operator family membership must balance performance optimization with semantic correctness.

**Strategy**:
- ✅ **Add to `numeric_ops` and `float_ops` families** → enables joins, maintains correctness
  - These are higher-precision type families
  - Safe for operators comparing numeric/float with integers
  - Enables Index Cond predicates and hash/nested loop joins
  
- ❌ **Don't add to `integer_ops` family** → prevents invalid transitive inferences
  - Would allow planner to infer integer column equals fractional constant
  - Violates exact comparison semantics
  - Would break core functionality

**Principle**: Choose operator family membership based on semantic safety, not just performance.

### 8.5 Complete Implementation Scope: 72 Operators

**Insight**: Full bidirectional coverage requires **72 operators** (54 forward + 18 commutator), not just 54.

**Rationale**:
- Commutators enable bidirectional comparisons: `int4 = numeric` AND `numeric = int4`
- Both directions need real C implementations (COMMUTATOR property alone insufficient for all optimizations)
- Equality operators (`=`, `<>`) need HASHES property in both directions
- Complete coverage: all (numeric/float4/float8) × (int2/int4/int8) × 6 operators

**Benefits**:
- Users can write comparisons in natural order
- Query optimizer has maximum flexibility
- Both hash joins and indexed nested loop joins work bidirectionally

**Result**: A symmetric, complete operator set with no "missing" combinations that would surprise users.

---

## Conclusion

These insights represent critical design decisions that shaped v1.0:
1. Leverage btree family membership for index optimization
2. Accept merge join impossibility to preserve semantic correctness
3. Use casting strategy for hash functions (simplicity + correctness)
4. Strategic operator family membership prevents dangerous inferences
5. Complete bidirectional operator coverage for user experience

The combination enables high performance (hash joins, indexed lookups) while maintaining exact comparison semantics.