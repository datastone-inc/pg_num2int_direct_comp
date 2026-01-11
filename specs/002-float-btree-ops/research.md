# Research: Float-Integer Btree Operator Family Integration

**Feature**: 002-float-btree-ops
**Date**: 2026-01-11

## Research Questions

### RQ-1: Btree Family Registration for Float Types

**Question**: How should int x float operators be registered in btree families to enable merge joins?

**Research**:
- PostgreSQL requires operators in the same btree family to satisfy transitivity
- Existing v1.0.0 pattern for int x numeric:
  - Add to BOTH `integer_ops` AND `numeric_ops` families
  - Include same-type operators for sort consistency within family context
  - Register comparison support function (FUNCTION 1) for each type pair

**Decision**: Follow identical pattern for float types:
- Add int x float4 operators to `integer_ops` AND `float4_ops`
- Add int x float8 operators to `integer_ops` AND `float8_ops`
- Add same-type float4/float8 operators to `integer_ops`
- Register existing comparison functions as FUNCTION 1

**Rationale**: Proven pattern from int x numeric. Enables merge joins from either side.

---

### RQ-2: Transitivity Proof for Exact Float Comparison

**Question**: Do our exact int x float operators satisfy transitivity required for btree families?

**Research**:
PostgreSQL's btree operator families require:
- If `a = b` returns TRUE and `b = c` returns TRUE, then `a = c` must be TRUE

Our exact comparison operators satisfy this because:
1. For any stored float value, exactly ONE integer value can equal it
2. If `int_a = float_b` returns TRUE, then `int_a` equals the exact integer representation of `float_b`
3. If `float_b = int_c` returns TRUE, then `int_c` also equals that same integer representation
4. Therefore `int_a = int_c` (both equal the same integer value)

**Example**:
```
float4 value 16777216.0 (stored)
  - Only 16777216::int4 equals this float
  - 16777217::int4 returns FALSE (our exact comparison detects difference)
  - No transitivity violation possible
```

**Decision**: Safe to add to btree families. Transitivity is preserved for stored values.

**Caveat**: User literals may round before storage (e.g., `16777217.0::float4` becomes `16777216.0`). This is float representation behavior, not a transitivity violation. Document this clearly.

---

### RQ-3: Existing Comparison Functions

**Question**: Can we reuse existing C comparison functions?

**Research**:
Checked pg_num2int_direct_comp--1.0.0.sql for existing functions:

| Function | Exists | Returns |
|----------|--------|---------|
| float4_cmp_int2 | ✅ Yes | int4 (-1/0/+1) |
| float4_cmp_int4 | ✅ Yes | int4 (-1/0/+1) |
| float4_cmp_int8 | ✅ Yes | int4 (-1/0/+1) |
| float8_cmp_int2 | ✅ Yes | int4 (-1/0/+1) |
| float8_cmp_int4 | ✅ Yes | int4 (-1/0/+1) |
| float8_cmp_int8 | ✅ Yes | int4 (-1/0/+1) |
| int2_cmp_float4 | ✅ Yes | int4 (-1/0/+1) |
| int4_cmp_float4 | ✅ Yes | int4 (-1/0/+1) |
| int8_cmp_float4 | ✅ Yes | int4 (-1/0/+1) |
| int2_cmp_float8 | ✅ Yes | int4 (-1/0/+1) |
| int4_cmp_float8 | ✅ Yes | int4 (-1/0/+1) |
| int8_cmp_float8 | ✅ Yes | int4 (-1/0/+1) |

**Decision**: Reuse all 12 existing comparison functions. No new C code needed.

---

### RQ-4: Event Trigger Cleanup Extension

**Question**: How should DROP EXTENSION cleanup be extended?

**Research**:
Existing cleanup function `pg_num2int_direct_comp_cleanup()`:
- Uses hardcoded array of DROP statements
- Executes each, ignoring errors (operator may not exist)
- Validated by extension_lifecycle.sql regression test

**Decision**: Extend the `ops` array with new entries:
- float4_ops btree: 6 type pairs x 6 entries each = 36 DROP statements
- float8_ops btree: 6 type pairs x 6 entries each = 36 DROP statements
- integer_ops btree: float4 and float8 same-type operators = 12 DROP statements

**Rationale**: Consistent with existing pattern. Lifecycle test will catch any omissions.

---

### RQ-5: Documentation Reduction Strategy

**Question**: How to reduce documentation by 20% while improving clarity?

**Research**:
Current word counts:
- README.md: ~4,200 words
- doc/api-reference.md: ~2,100 words
- doc/benchmark.md: ~800 words
- doc/development.md: ~1,500 words
- doc/installation.md: ~400 words
- doc/operator-reference.md: ~2,200 words
- **Total: 11,234 words**
- **Target: ≤8,987 words**

**Decision**:
1. **README.md** (target: ~2,800 words, -33%):
   - Remove duplicate explanations of precision loss
   - Add concise "Why Custom Operators?" section upfront
   - Convert verbose examples to focused demonstrations
   - Move deep technical background to doc/development.md

2. **doc/api-reference.md** (target: ~1,600 words, -24%):
   - Remove redundant operator descriptions (tables are self-documenting)
   - Focus on behavior, not implementation

3. **doc/operator-reference.md** (target: ~1,800 words, -18%):
   - Condense repetitive table explanations
   - Add float btree family information concisely

4. **Persuasive framing**:
   - Lead with the problem (wrong results, slow queries)
   - Acknowledge "non-standard" concern directly
   - Explain why custom operators are the only solution
   - Demonstrate with clear before/after examples

---

## Summary

| Question | Decision | Risk |
|----------|----------|------|
| RQ-1: Btree registration | Follow int x numeric pattern | Low |
| RQ-2: Transitivity | Safe for stored values; document literal rounding | Low |
| RQ-3: Comparison functions | Reuse all 12 existing | None |
| RQ-4: Event trigger | Extend hardcoded array | Low (lifecycle test validates) |
| RQ-5: Documentation | Reduce by restructuring and condensing | Medium (subjective quality) |
| RQ-6: Float precision limits | Document that precision loss is permanent; use int/numeric params | Low |

---

## RQ-6: Cross-Type Comparison Equivalence Expressions

**Question**: How can users switch between extension and stock PostgreSQL comparison semantics?

### Background: int8 vs float precision

int8 range: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807 (≈ ±9.2 × 10¹⁸)

Float precision boundaries:
- **float4**: Exact integers up to 2²⁴ = 16,777,216
- **float8**: Exact integers up to 2⁵³ = 9,007,199,254,740,992

This means:
- **int8 vs float4**: Most int8 values exceed float4 precision (gap starts at 2²⁴)
- **int8 vs float8**: Large int8 values (> 2⁵³) exceed float8 precision

### The Two Semantics

| Semantics | Description | Example: `16777217::int8` vs `16777216.0::float4` |
|-----------|-------------|---------------------------------------------------|
| **Stock PostgreSQL** | Cast int to float, then compare | TRUE (16777217→16777216.0, equals stored) |
| **Extension** | Exact cross-type comparison | FALSE (16777217 ≠ 16777216.0 mathematically) |

---

### Case 1: With Extension, Emulate Stock PostgreSQL

**Goal**: Force the "cast then compare" behavior even when extension is installed.

**Solution**: Explicit cast to float8 (matching stock PostgreSQL behavior):

```sql
-- int8 column, float4 column/literal/parameter
-- Stock behavior: both sides cast to float8
WHERE int8_col::float8 = float4_col::float8   -- column
WHERE int8_col::float8 = 16777216.0::float4::float8   -- literal  
WHERE int8_col::float8 = $1::float8           -- float4 parameter

-- int8 column, float8 column/literal/parameter
WHERE int8_col::float8 = float8_col           -- column
WHERE int8_col::float8 = 1.0::float8          -- literal
WHERE int8_col::float8 = $1                   -- float8 parameter
```

**Explanation**: In stock PostgreSQL, int vs float4 comparisons cast both sides to float8 (the common supertype). By explicitly casting both to float8, you invoke PostgreSQL's built-in `float8 = float8` operator, bypassing the extension's cross-type operator.

**When to use**: Legacy compatibility, or when you specifically want "same float representation" semantics.

---

### Case 2: Without Extension, Emulate Extension (Exact Comparison)

**Goal**: Get mathematically correct cross-type comparison without installing the extension.

**Solution**: Cast both sides to numeric:

```sql
-- int8 column vs float4 column
WHERE int8_col::numeric = float4_col::numeric

-- int8 column vs float8 column  
WHERE int8_col::numeric = float8_col::numeric

-- With literals (works correctly)
WHERE int8_col::numeric = 16777216.0::float4::numeric
```

**Explanation**: 
- `float4_col::numeric` converts the stored float to its exact decimal representation
- `int8_col::numeric` converts the integer exactly
- Numeric comparison is exact

**Example**:
```sql
-- Stored: int8_col = 16777217, float4_col = 16777216.0
SELECT 16777217::numeric = 16777216.0::float4::numeric;  -- FALSE (correct!)

-- Stock PostgreSQL without cast:
SELECT 16777217::int8 = 16777216.0::float4;  -- TRUE (wrong!)
```

**Limitation**: Cannot use btree index on `int8_col` (expression disables index scan).

---

### Case 3: Parameters

Float parameters lose precision at bind time—this is unavoidable regardless of comparison method.

| Parameter Type | Passed Value | Received Value | Notes |
|---------------|--------------|----------------|-------|
| float4 | 16777217 | 16777216.0 | Precision lost at bind |
| float8 | 16777217 | 16777217.0 | Exact (within float8 precision) |
| int8 | 16777217 | 16777217 | Exact |
| numeric | 16777217 | 16777217 | Exact |

**Recommendations**:

```sql
-- Searching int8 column: use int8 or numeric parameter
PREPARE find_int8(int8) AS 
  SELECT * FROM t WHERE int8_col = $1;            -- with extension: exact
  
PREPARE find_int8(numeric) AS 
  SELECT * FROM t WHERE int8_col = $1;            -- with extension: exact

-- Searching float4 column with known integer: use int4/int8 parameter
PREPARE find_float4(int8) AS 
  SELECT * FROM t WHERE float4_col = $1;          -- with extension: exact

-- Without extension, emulate exact comparison:
PREPARE find_float4_exact(int8) AS 
  SELECT * FROM t WHERE float4_col::numeric = $1::numeric;
```

---

### Summary Table: Equivalence Expressions

| Scenario | With Extension | Without Extension (exact) | Stock PG (both → float8) |
|----------|----------------|---------------------------|---------------------------|
| int8_col = float4_col | `int8_col = float4_col` | `int8_col::numeric = float4_col::numeric` | `int8_col::float8 = float4_col::float8` |
| int8_col = float8_col | `int8_col = float8_col` | `int8_col::numeric = float8_col::numeric` | `int8_col::float8 = float8_col` |
| int8_col = float4_lit | `int8_col = 1.5::float4` | `int8_col::numeric = 1.5::float4::numeric` | `int8_col::float8 = 1.5::float4::float8` |
| int8_col = float4_param | `int8_col = $1` | `int8_col::numeric = $1::numeric` | `int8_col::float8 = $1::float8` |

**Index usage**:
- Extension operators: Yes (via support functions)
- `::numeric` cast: No (expression on indexed column)
- `::float` cast on int column: No (expression on indexed column)
- `::float` cast on parameter only: Possible with extension

---

### Finding Precision Boundaries (Helper Queries)

```sql
-- Check if an int8 value is exactly representable as float4
SELECT i, i::float4::int8 = i AS exact_float4,
          i::float8::int8 = i AS exact_float8
FROM (VALUES 
    (16777215::int8), (16777216), (16777217), (16777218),
    (9007199254740992::int8), (9007199254740993)
) AS v(i);
--          i          | exact_float4 | exact_float8
-- --------------------+--------------+--------------
--            16777215 | t            | t
--            16777216 | t            | t
--            16777217 | f            | t   ← float4 loses, float8 keeps
--            16777218 | t            | t
--   9007199254740992  | f            | t   ← float8 boundary
--   9007199254740993  | f            | f   ← both lose

-- Demonstrate the two semantics
SELECT 
    16777217::int8 = 16777216.0::float4 AS stock_pg,           -- true (wrong)
    16777217::numeric = 16777216.0::float4::numeric AS exact;  -- false (correct)
```

### Decision

Document equivalence expressions for users who need to:
1. **With extension → stock behavior**: Use explicit `int_col::float = float_col`
2. **Without extension → exact behavior**: Use `int_col::numeric = float_col::numeric`

Note that the numeric cast approach cannot use indexes on the integer column, making it suitable only for small result sets or when correctness outweighs performance.

**Rationale**: Users migrating to/from the extension need clear guidance on equivalent expressions. This also helps testing and validation.
