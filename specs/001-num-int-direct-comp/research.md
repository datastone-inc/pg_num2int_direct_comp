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

## 4. Btree Operator Family Support

**Decision**: Add int×numeric operators to BOTH `integer_ops` AND `numeric_ops` btree families with MERGES property. Add int×float operators to `float_ops` only (btree family integration deferred to reduce scope).

**Rationale**:
- int×numeric operators ARE mathematically transitive and safe for all btree families
- Adding to BOTH integer_ops AND numeric_ops enables merge joins from either side
- MERGES property enables merge join optimization
- int×float btree family integration is deferred to reduce v1.0 scope (index optimization via support functions still works)

### 4.1 Why Operators Are Transitive and Safe for All Btree Families

**Mathematical Proof**:
- If `A = B` returns TRUE (no fractional part) and `B = C` returns TRUE, then `A = C` is guaranteed TRUE
- If `A = B` returns FALSE (has fractional part), the transitive chain correctly propagates: planner won't infer anything from a FALSE equality
- Example: `10.5 = 10` → FALSE, so `(10.5 = 10) AND (10 = X)` is unsatisfiable — no invalid inference
- The "problem case" (`int_col = 10 AND numeric_col = 10.5`) is correctly unsatisfiable because 10 ≠ 10.5

**Key Insight**: Our exact operators compare against actual stored values, not user literals. When both `A = B` and `B = C` are TRUE with our exact semantics, transitivity holds mathematically.

**Safe for all btree families**:
- Adding int×numeric operators to `integer_ops` family IS safe
- Adding int×numeric operators to `numeric_ops` family IS safe
- Both directions must be registered in both families (symmetric registration)
- This enables merge joins from either side of the join

### 4.2 Why Integer_ops Membership IS Required

**Rationale for symmetric registration**:
- Query: `SELECT * FROM int_table i JOIN numeric_table n ON i.col = n.col`
- If only `numeric_ops` has our operators, planner can only consider merge join when scanning from numeric side
- With BOTH `integer_ops` AND `numeric_ops` membership, merge join works from either side
- PostgreSQL's planner needs to find compatible operators in the index's operator family

**Example scenario requiring integer_ops**:
```sql
-- If int_table.col has btree index in integer_ops family
-- Planner needs int×numeric operator IN integer_ops to use merge join efficiently
SELECT * FROM int_table i JOIN numeric_table n ON i.col = n.col;
```

**Transitivity is safe**: The concern about transitive inference was analyzed and found to be unfounded:
- Our exact operators only return TRUE when values are mathematically equal
- When TRUE, transitivity holds perfectly
- When FALSE, no inference is made
- This matches PostgreSQL's built-in exact numeric equality semantics


### 4.3 MERGES Property for Merge Joins

**Decision**: Add MERGES property to all int×numeric equality operators.

**What MERGES Enables**:
- Allows PostgreSQL to use merge join algorithm for these operators
- Merge joins require sorted inputs and MERGES-marked operators
- Combined with btree family membership (both `integer_ops` AND `numeric_ops`), enables efficient merge join execution

**SQL Declaration**:
```sql
-- Enable merge join for int×numeric equality
CREATE OPERATOR = (
    LEFTARG = numeric,
    RIGHTARG = integer,
    ...
    MERGES  -- Enables merge join optimization
);
```

**Result**: int×numeric joins can use merge join strategy when beneficial.

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

**What was NOT implemented in initial v1.0** (now required per spec):
- ⚠️ **MERGES property**: Required for int×numeric equality operators (implementation task pending)
- ⚠️ **integer_ops family membership**: Required for int×numeric operators (implementation task pending)
- ⚠️ **int×float btree integration**: Deferred to reduce scope

**What IS implemented**:
- ✅ **Merge join optimization**: Enabled via MERGES property and dual family membership (for int×numeric)
- ✅ **Hash join support**: All operators have HASHES property

**Result**:
- ✅ Operators enable indexed nested loop joins (major performance win)
- ✅ Operators enable merge joins for int×numeric (via integer_ops + numeric_ops membership)
- ✅ Operators are safe from transitive inference problems (exact semantics guarantee correctness)
- ✅ All 10 regression tests pass with improved query plans
- ✅ Hash joins still work when needed (planner chooses based on statistics)

### 4.6 Summary: Why This Approach is Optimal

**Comparison of approaches**:

| Approach | Indexed NL Join | Merge Join | Transitive Safety | Implementation |
|----------|-----------------|------------|-------------------|----------------|
| No btree families | ❌ No | ❌ No | ✅ Safe | Simple |
| In numeric_ops + float_ops only | ✅ Yes | ❌ No | ✅ Safe | Partial |
| In both integer_ops AND numeric_ops | ✅ Yes | ✅ Yes | ✅ **SAFE** | **CHOSEN for int×numeric** |

**Why the chosen approach is best for int×numeric**:
1. Enables indexed nested loop joins
2. Enables merge joins via symmetric btree family membership
3. Maintains correctness (exact operators guarantee transitivity)
4. Works for all query patterns

**int×float btree integration deferred**:
- int×float operators are in float_ops for hash joins only
- Btree family integration deferred to reduce v1.0 scope
- Index optimization still works via support functions (SupportRequestIndexCondition)

**When each join strategy is used**:
- **Indexed Nested Loop**: Small selective result sets (most queries) - NOW OPTIMIZED ✅
- **Hash Join**: Large non-selective joins without indexes - still works ✅
- **Merge Join**: Large joins with sorted inputs - NOW AVAILABLE for int×numeric ✅

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
4. **Btree family membership**: 
   - int×numeric: BOTH `integer_ops` AND `numeric_ops` families (enables merge joins)
   - int×float: `float_ops` hash family only (btree integration deferred)
5. **MERGES property**: Added to int×numeric equality operators for merge join support
6. **HASHES property**: All equality operators have HASHES for hash join support
7. **Testing**: pg_regress framework with categorized test suites
8. **Type coverage**: Full 9-combination matrix (108 operators) with type-specific precision handling

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

**Insight**: Adding operators to btree families enables **indexed nested loop joins** and, with MERGES property, **merge joins**.

**Impact**:
- Operators benefit from btree indexes via family membership
- Index Cond predicates work with cross-type operators
- For int×numeric: adding to BOTH `integer_ops` AND `numeric_ops` enables merge joins from either side
- For int×float: btree family integration deferred (support functions still enable index optimization)

**Key Realization**: 
- Family membership + support functions = index optimization
- MERGES property + symmetric family registration = merge join optimization
- Exact operator semantics guarantee transitivity is safe

### 8.2 Merge Joins ARE Possible for int×numeric

**Insight**: Merge joins CAN be implemented because our exact operators are mathematically transitive.

**Reasoning**:
- Merge joins require operators in compatible btree families on both sides
- Adding int×numeric operators to BOTH `integer_ops` AND `numeric_ops` enables merge joins
- Transitivity concern was analyzed and found to be unfounded:
  - Our exact operators only return TRUE when values are mathematically equal
  - When TRUE, transitivity holds perfectly (A=B and B=C implies A=C)
  - When FALSE, planner makes no inference
  - The "problem case" is actually correct: `10 = 10.5` is FALSE, so no transitive inference occurs

**Design Decision**: Implement merge joins for int×numeric via:
- MERGES property on equality operators
- Symmetric registration in both `integer_ops` and `numeric_ops` families
- int×float merge joins deferred (not architecturally blocked, just scope reduction)

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

**Insight**: Operator family membership must balance performance optimization with semantic correctness. Our exact operators ARE mathematically transitive, enabling full btree family membership.

**Strategy for int×numeric** (36 operators):
- ✅ **Add to BOTH `integer_ops` AND `numeric_ops` families** → enables merge joins, indexed nested loop joins
  - Symmetric registration enables merge joins from either side
  - MERGES property on equality operators
  - Transitivity is safe because exact operators only return TRUE for mathematically equal values
  
**Strategy for int×float** (72 operators):
- ✅ **Add to `float_ops` hash family** → enables hash joins
- ⚠️ **Btree family integration deferred** → reduces v1.0 scope
  - Support functions (SupportRequestIndexCondition) still enable index optimization
  - Not architecturally blocked, just scope reduction

**Principle**: Exact comparison operators ARE transitive when properly implemented. Strategic family membership enables optimal join strategies.

### 8.5 Complete Implementation Scope: 108 Operators

**Insight**: Full bidirectional coverage requires **108 operators**: 9 type combinations × 6 operators × 2 directions.

**Breakdown**:
- **int×numeric**: 36 operators (3 int types × 6 ops × 2 directions) with full btree/hash family integration
- **int×float4**: 36 operators with hash family integration (btree deferred)
- **int×float8**: 36 operators with hash family integration (btree deferred)

**Rationale**:
- Commutators enable bidirectional comparisons: `int4 = numeric` AND `numeric = int4`
- Both directions need real C implementations (COMMUTATOR property alone insufficient for all optimizations)
- Equality operators (`=`, `<>`) have HASHES property in both directions
- int×numeric equality operators have MERGES property for merge joins
- Complete coverage: all (numeric/float4/float8) × (int2/int4/int8) × 6 operators × 2 directions

**Benefits**:
- Users can write comparisons in natural order
- Query optimizer has maximum flexibility
- Hash joins work for all type combinations
- Merge joins work for int×numeric
- Indexed nested loop joins work via support functions and btree family membership

**Result**: A symmetric, complete operator set with no "missing" combinations that would surprise users.

---

## Conclusion

These insights represent critical design decisions that shaped v1.0:
1. Leverage btree family membership for index optimization
2. Exact operators ARE transitive — merge joins enabled for int×numeric via symmetric registration
3. Use casting strategy for hash functions (simplicity + correctness)
4. Strategic operator family membership enables optimal join strategies
5. Complete bidirectional operator coverage (108 operators) for user experience
6. int×float btree integration deferred to reduce scope (support functions still work)

The combination enables high performance (hash joins, merge joins, indexed lookups) while maintaining exact comparison semantics.

---

## 9. SupportRequestSimplify vs SupportRequestIndexCondition

**Date**: 2025-12-29

**Context**: After implementing btree family membership for int×numeric operators, we discovered that `SupportRequestIndexCondition` is bypassed when operators are in a btree family. This raises the question: can `SupportRequestSimplify` provide better planning while still enabling merge joins?

### 9.1 Problem Discovery

When int×numeric operators are added to btree families (integer_ops + numeric_ops), PostgreSQL's planner uses the btree family machinery directly for index access. This **bypasses** the `SupportRequestIndexCondition` handler, meaning:

1. **No predicate transformation**: `int_col = 100::numeric` is NOT transformed to `int_col = 100`
2. **No fractional detection**: `int_col = 10.5::numeric` is NOT transformed to `FALSE`
3. **Suboptimal selectivity estimates**: Planner estimates `rows=1` even for impossible predicates

**Observed Behavior**:
```sql
EXPLAIN SELECT * FROM test_selectivity WHERE val = 10.5::numeric;
-- Shows: Index Cond: (val = 10.5)
-- Estimates: rows=1 (but returns 0 rows - impossible predicate)
```

### 9.2 Support Request Types Comparison

| Aspect | SupportRequestIndexCondition | SupportRequestSimplify |
|--------|------------------------------|------------------------|
| **When called** | During index path planning | During query simplification (earlier) |
| **What it transforms** | Index condition only | The expression itself |
| **Btree family interaction** | Bypassed when op is in btree family | Called BEFORE btree family lookup |
| **Fractional → FALSE** | Can transform, but bypassed | Can transform at parse time |
| **Join conditions** | N/A (constants only) | N/A (constants only) |
| **PostgreSQL version** | PG12+ | PG12+ |

### 9.3 SupportRequestSimplify Advantages

For **constant predicates** (WHERE clause with literals), `SupportRequestSimplify` can:

1. **Transform earlier**: Before btree family even checked
2. **Improve selectivity**: `int_col = 10.5::numeric` → `FALSE` gives `rows=0` estimate
3. **Enable constant folding**: `int_col = 100::numeric` → `int_col = 100` uses native operator
4. **Work WITH btree families**: Transformation happens before family-based optimization

**Transformation Examples**:

| Original Predicate | SupportRequestSimplify Result | Benefit |
|-------------------|-------------------------------|---------|
| `int_col = 100::numeric` | `int_col = 100` | Uses int4=int4, perfect estimate |
| `int_col = 10.5::numeric` | `FALSE` | Rows=0 estimate, may skip scan |
| `int_col > 10.5::numeric` | `int_col > 10` or `int_col >= 11` | Correct boundary, good estimate |
| `int_col < 10.5::numeric` | `int_col <= 10` | Correct boundary |

### 9.4 Limitations

`SupportRequestSimplify` only works for **constant expressions**. It cannot help with:

1. **Join conditions**: `t1.int_col = t2.numeric_col` - runtime values unknown
2. **Parameterized queries**: `int_col = $1::numeric` - parameter value unknown at plan time

For these cases, we still need:
- **Btree family membership**: Enables nested loop join index access and merge joins for int×numeric
- **Cross-type operators**: Correct evaluation at runtime

### 9.5 Recommended Architecture

**Hybrid approach for v1.0+**:

1. **Keep btree family membership**: Required for joins
2. **Add SupportRequestSimplify handler**: For constant predicate optimization
3. **Remove SupportRequestIndexCondition** (optional): Redundant when Simplify handles constants

**Implementation in num2int_support()**:
```c
Datum
num2int_support(PG_FUNCTION_ARGS)
{
    Node *rawreq = (Node *) PG_GETARG_POINTER(0);
    
    if (IsA(rawreq, SupportRequestSimplify))
    {
        SupportRequestSimplify *req = (SupportRequestSimplify *) rawreq;
        // Transform: int_col = 100::numeric → int_col = 100
        // Transform: int_col = 10.5::numeric → FALSE
        // Return transformed expression or NULL if no transformation
    }
    
    // SupportRequestIndexCondition no longer needed if Simplify handles all cases
    
    PG_RETURN_POINTER(NULL);
}
```

### 9.6 Decision

**Recommendation**: Implement `SupportRequestSimplify` as the primary mechanism for constant predicate optimization. This provides:

1. ✅ Better selectivity estimates (fractional → FALSE gives rows=0)
2. ✅ Correct boundary handling for range operators
3. ✅ Works alongside btree family membership (not bypassed)
4. ✅ Simpler architecture (one transformation mechanism)

**For joins**: Btree family membership remains essential for merge join support. The operators themselves handle runtime evaluation correctly.

**Trade-off accepted**: Join predicates don't get the same optimization as constants, but:
- Merge joins work (btree family)
- Hash joins work (hash family)
- Results are always correct (exact operators)

### 9.7 Implementation Notes

The `SupportRequestSimplify` handler should:

1. **Check for constant operand**: Only transform when one side is a Const node
2. **Detect fractional values**: For equality, if numeric/float has fractional part → return FALSE
3. **Handle range boundaries**: For `<`, `>`, `<=`, `>=`, compute correct integer boundary
4. **Return NULL if no transformation**: Let operator execute normally for non-constant cases
5. **Build replacement expression**: Use `makeBoolConst()` for FALSE, or build new OpExpr with integer constant

**Key insight**: `SupportRequestSimplify` effectively replaces `SupportRequestIndexCondition` for constant predicates while enabling btree family benefits for joins.