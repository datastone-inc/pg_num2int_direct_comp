-- Test constant predicate optimization and selectivity estimation
-- Verify SupportRequestSimplify detects impossible predicates and transforms expressions
-- Tests FR-015: Impossible equality detection (int_col = 10.5::numeric → FALSE)
-- Tests FR-016: Exact match transformation (int_col = 100::numeric → int_col = 100)
-- Tests FR-017: Range boundary transformation (int_col > 10.5::numeric → int_col >= 11)

-- Load extension
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;

-- ============================================================================
-- Setup: Create test table
-- ============================================================================
DROP TABLE IF EXISTS selectivity_test;
CREATE TABLE selectivity_test (
  id serial PRIMARY KEY,
  int2_col int2,
  int4_col int4,
  int8_col int8
);
INSERT INTO selectivity_test (int2_col, int4_col, int8_col)
SELECT i::int2, i, i::int8
FROM generate_series(1, 1000) i;
ANALYZE selectivity_test;

-- ============================================================================
-- Test Group 1: FR-015 - Impossible Equality Detection
-- Predicates like "int_col = 10.5::numeric" should be recognized as always-false
-- Expected: rows=0 estimate OR constant FALSE in plan
-- ============================================================================

-- Test 1a: int4 = fractional numeric → impossible
-- The predicate should simplify to FALSE since 10.5 can never equal an integer
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 10.5::numeric;

-- Test 1b: int2 = fractional numeric → impossible
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col = 10.5::numeric;

-- Test 1c: int8 = fractional numeric → impossible
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int8_col = 10.5::numeric;

-- Test 1d: int4 = fractional float4 → impossible
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 10.5::float4;

-- Test 1e: int4 = fractional float8 → impossible
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 10.5::float8;

-- Test 1f: Verify impossible predicate returns no rows at execution
SELECT count(*) FROM selectivity_test WHERE int4_col = 10.5::numeric;

-- ============================================================================
-- Test Group 2: FR-016 - Exact Integer Match Transformation
-- Predicates like "int_col = 100::numeric" should transform to "int_col = 100"
-- Expected: Uses native int=int operator for optimal selectivity estimation
-- ============================================================================

-- Test 2a: int4 = whole numeric (100.0) → transforms to int4 = 100
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 100::numeric;

-- Test 2b: int2 = whole numeric → transforms correctly
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col = 100::numeric;

-- Test 2c: int8 = whole numeric → transforms correctly
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int8_col = 100::numeric;

-- Test 2d: Verify exact match returns correct row
SELECT int4_col FROM selectivity_test WHERE int4_col = 100::numeric;

-- Test 2e: Verify commutator direction works (numeric = int4)
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE 100::numeric = int4_col;

-- ============================================================================
-- Test Group 3: FR-017 - Range Boundary Transformation
-- Predicates with fractional boundaries should transform to correct integer predicates
-- int_col > 10.5 → int_col >= 11 (round up for >)
-- int_col < 10.5 → int_col <= 10 (truncate for <)
-- int_col >= 10.5 → int_col >= 11 (round up for >=)
-- int_col <= 10.5 → int_col <= 10 (truncate for <=)
-- ============================================================================

-- Test 3a: int4 > 10.5::numeric → should be equivalent to int4 >= 11
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col > 10.5::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col > 10.5::numeric;
-- Verify: should match count of int4_col >= 11
SELECT count(*) FROM selectivity_test WHERE int4_col >= 11;

-- Test 3b: int4 < 10.5::numeric → should be equivalent to int4 <= 10
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col < 10.5::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col < 10.5::numeric;
-- Verify: should match count of int4_col <= 10
SELECT count(*) FROM selectivity_test WHERE int4_col <= 10;

-- Test 3c: int4 >= 10.5::numeric → should be equivalent to int4 >= 11
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col >= 10.5::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col >= 10.5::numeric;

-- Test 3d: int4 <= 10.5::numeric → should be equivalent to int4 <= 10
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col <= 10.5::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col <= 10.5::numeric;

-- Test 3e: Range with exact boundary (no fractional part) preserves operator
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col > 10::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col > 10::numeric;

-- ============================================================================
-- Test Group 4: Inequality with fractional values
-- int_col <> 10.5::numeric should always be TRUE (every integer differs from 10.5)
-- ============================================================================

-- Test 4a: int4 <> 10.5::numeric → should simplify to TRUE (all rows match)
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col <> 10.5::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col <> 10.5::numeric;

-- ============================================================================
-- Test Group 5: Edge cases
-- ============================================================================

-- Test 5a: Very small fractional part
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 100.0001::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col = 100.0001::numeric;

-- Test 5b: Negative fractional values
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = (-10.5)::numeric;
SELECT count(*) FROM selectivity_test WHERE int4_col = (-10.5)::numeric;

-- Test 5c: Zero with decimal (should match zero exactly)
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int4_col = 0.0::numeric;

-- ============================================================================
-- Test 6: Join optimization with constant propagation
-- ============================================================================
-- These tests verify that constant predicates optimize joins correctly

-- Test 6a: Join with exact numeric constant (10.0)
-- Should transform to native int comparison and propagate to both tables
EXPLAIN (COSTS OFF) 
SELECT COUNT(*) FROM selectivity_test a, selectivity_test b
WHERE a.int4_col = b.int4_col AND b.int4_col = 10.0;

SELECT COUNT(*) FROM selectivity_test a, selectivity_test b
WHERE a.int4_col = b.int4_col AND b.int4_col = 10.0;

-- Test 6b: Join with fractional constant (10.5)
-- Should recognize impossible predicate and short-circuit entire join
EXPLAIN (COSTS OFF) 
SELECT COUNT(*) FROM selectivity_test a, selectivity_test b
WHERE a.int4_col = b.int4_col AND b.int4_col = 10.5;

SELECT COUNT(*) FROM selectivity_test a, selectivity_test b
WHERE a.int4_col = b.int4_col AND b.int4_col = 10.5;

-- ============================================================================
-- Test Group 7: Out-of-range constant optimization
-- Constants outside the integer range should be optimized to TRUE/FALSE
-- ============================================================================

-- Test 7a: int2 = value exceeding int2 max → always FALSE
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col = 99999::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col = 99999::numeric;

-- Test 7b: int2 <> value exceeding int2 max → always TRUE (all rows)
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col <> 99999::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col <> 99999::numeric;

-- Test 7c: int2 < value exceeding int2 max → always TRUE
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col < 99999::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col < 99999::numeric;

-- Test 7d: int2 > value exceeding int2 max → always FALSE
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col > 99999::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col > 99999::numeric;

-- Test 7e: int2 < value below int2 min → always FALSE
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col < (-99999)::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col < (-99999)::numeric;

-- Test 7f: int2 > value below int2 min → always TRUE
EXPLAIN (COSTS OFF) SELECT * FROM selectivity_test WHERE int2_col > (-99999)::numeric;
SELECT count(*) FROM selectivity_test WHERE int2_col > (-99999)::numeric;

-- ============================================================================
-- Cleanup
-- ============================================================================
DROP TABLE selectivity_test;
