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

**Rationale**:
- `SupportRequestIndexCondition` is PostgreSQL's standard mechanism for enabling index-optimized predicates
- LIKE operator (`pattern_ops`) provides proven pattern for type-crossing index access
- A single generic support function handles all type combinations by inspecting operator OID and operand types
- Reduces code duplication - one support function instead of 54 separate functions
- Uses lazy OID cache to identify which operator is being invoked
- Allows query planner to recognize `intcol = 10.0::numeric` as indexable predicate
- Returns transformed node that planner can use for index scan instead of seq scan

**Alternatives Considered**:
- **Separate support function per operator**: Would require 54 functions with nearly identical logic. Rejected due to excessive duplication.
- **Custom operator class**: Would require defining complete btree operator families. Rejected as unnecessarily complex since integer types already have btree support.
- **Planner hooks**: Would be fragile across PostgreSQL versions. Rejected for maintainability.
- **No index support**: Would make extension unusable for large tables. Rejected per User Story 2 requirements.

**Implementation Pattern**:
```c
// Generic support function for all exact comparison operators
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;
    
    // Initialize OID cache if needed
    init_oid_cache();
    
    // Extract the comparison: intcol = <numeric_constant>
    if (is_opclause(req->node)) {
      OpExpr *clause = (OpExpr *) req->node;
      Oid opno = clause->opno;
      
      // Identify which type combination based on operator OID
      // Check if numeric/float arg is const and within int range
      // If yes, transform to standard integer comparison
      // Return modified clause that planner can use with existing btree index
    }
  }
  
  PG_RETURN_POINTER(NULL);
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

## 4. Preventing Transitive Inference

**Decision**: Do not add these operators to btree operator families that would enable transitive inference across different type combinations

**Rationale**:
- PostgreSQL query optimizer applies transitive inference when operators are members of compatible btree operator families
- The optimizer checks operator family membership and strategy numbers to determine if transitivity is valid
- By keeping these operators independent (not grouped into cross-type operator families), the planner won't infer `floatcol = 10.0` from `floatcol = intcol AND intcol = 10.0`
- Each comparison stands alone without family relationships that would allow `A = B AND B = C` to imply `A = C` when A, B, C are different numeric types

**Implementation**:
```sql
-- Operator definition as standalone (not added to cross-type operator family)
CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_eq_int4,
  COMMUTATOR = =,        -- Safe: equality is commutative
  NEGATOR = <>,          -- Safe: logical negation
  RESTRICT = eqsel,      -- Selectivity estimator
  JOIN = eqjoinsel,      -- Join selectivity estimator
  HASHES                 -- Safe: enables hash joins without transitivity
  -- Note: SUPPORT property is set on the function, not the operator
  -- MERGES property would be used on ordering operators (<, >, <=, >=)
  -- Not added to btree operator family to prevent transitivity
);
```

**PostgreSQL References**:
- `src/backend/optimizer/util/clauses.c` - Transitive inference logic
- Operator catalog (`pg_operator`) - Properties that enable/disable inference

## 5. Test Strategy with pg_regress

**Decision**: Organize tests by functionality (operators, index usage, NULL handling, special values, edge cases) using pg_regress framework

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

## Summary of Technical Approach

The extension implementation strategy is:

1. **Comparison functions**: C functions using PostgreSQL's `numeric_trunc()` and bounds checking primitives
2. **Index integration**: `SupportRequestIndexCondition` support functions modeled after LIKE operator
3. **Performance optimization**: Lazy per-backend OID cache for operator lookups
4. **Transitivity prevention**: Omit `MERGES` property from operator definitions
5. **Testing**: pg_regress framework with categorized test suites
6. **Type coverage**: Full 9-combination matrix with type-specific precision handling

All decisions align with the constitution's requirements for code style (K&R C), testing (pg_regress), build system (PGXS), and documentation standards.
