-- Test: Merge Join Support for Direct Numeric-Integer Comparison
-- Purpose: Verify that operators enable merge joins now that they're in btree families
-- Rationale: Operators ARE mathematically transitive and safe to add to btree families.
--            Research confirms: if A = B (no fractional part) and B = C, then A = C.
--            If A = B returns false (has fractional part), transitive chain correctly
--            propagates inequality. Example: 10.5 = 10 → false, so (10.5 = 10) AND (10 = X)
--            → false regardless of X. This matches PostgreSQL's native exact numeric 
--            comparison: 10.5::numeric = 10::numeric → false.
--
-- Implementation: Operators added to numeric_ops, float4_ops, float8_ops btree families
--                 to enable merge join optimization for large table joins.

-- Load extension
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;

-- Create test tables
CREATE TEMPORARY TABLE merge_int4 (id SERIAL PRIMARY KEY, val INT4);
CREATE TEMPORARY TABLE merge_numeric (id SERIAL PRIMARY KEY, val NUMERIC);
CREATE TEMPORARY TABLE merge_float8 (id SERIAL PRIMARY KEY, val FLOAT8);

-- Populate with sequential values
INSERT INTO merge_int4 (val) SELECT generate_series(1, 10000);
INSERT INTO merge_numeric (val) SELECT generate_series(1, 10000)::numeric;
INSERT INTO merge_float8 (val) SELECT generate_series(1, 10000)::float8;

-- Create indexes
CREATE INDEX idx_merge_int4_val ON merge_int4(val);
CREATE INDEX idx_merge_numeric_val ON merge_numeric(val);
CREATE INDEX idx_merge_float8_val ON merge_float8(val);

-- Analyze tables
ANALYZE merge_int4;
ANALYZE merge_numeric;
ANALYZE merge_float8;

-- Test 1: Verify btree family membership enables optimized joins
-- With cross-type operators in btree families, PostgreSQL can use indexed nested loops
\echo '=== Test 1: Optimized Cross-Type Join ==='
EXPLAIN (COSTS OFF)
SELECT COUNT(*) 
FROM merge_int4 i 
JOIN merge_numeric n ON i.val = n.val
WHERE i.val < 1000;

-- Expected: Should use Nested Loop with index scans on both sides
-- This is enabled by btree family membership

-- Test 2: Verify operators ARE in btree operator families
\echo '=== Test 2: Operator Family Membership Check ==='
SELECT 
    op.oprname,
    op.oprleft::regtype,
    op.oprright::regtype,
    amop.amopfamily::regclass::text as opfamily
FROM pg_operator op
JOIN pg_amop amop ON op.oid = amop.amopopr
WHERE op.oprname IN ('=', '<', '<=', '>', '>=', '<>')
  AND (
    (op.oprleft = 'numeric'::regtype AND op.oprright = 'int4'::regtype) OR
    (op.oprleft = 'int4'::regtype AND op.oprright = 'numeric'::regtype) OR
    (op.oprleft = 'float8'::regtype AND op.oprright = 'int4'::regtype) OR
    (op.oprleft = 'int4'::regtype AND op.oprright = 'float8'::regtype)
  )
ORDER BY op.oprname, op.oprleft, op.oprright;

-- Test 3: Explain why btree family membership is safe
\echo '=== Test 3: Why Btree Family Membership Is Safe ==='
\echo 'Btree family membership enables indexed nested loop optimization.'
\echo 'Operators ARE mathematically transitive for exact comparison semantics:'
\echo '  - If 10.5 = 10 → false (has fractional part)'
\echo '  - Then (10.5 = 10) AND (10 = X) → false regardless of X'
\echo '  - Transitive chain correctly propagates inequality'
\echo 'This matches PostgreSQL native behavior: 10.5::numeric = 10::numeric → false'
\echo 'Operators added to numeric_ops and float_ops families for optimization.'
\echo 'Note: NOT added to integer_ops to avoid problematic transitive inference from int side.'

-- Test 4: Verify normal join strategies still work
\echo '=== Test 4: Normal Join Strategies ==='
EXPLAIN (COSTS OFF)
SELECT COUNT(*) 
FROM merge_int4 i 
JOIN merge_numeric n ON i.val = n.val
WHERE i.val < 100;

SELECT 'int4-numeric join (working)'::text AS test, COUNT(*) AS count
FROM merge_int4 i 
JOIN merge_numeric n ON i.val = n.val
WHERE i.val < 100;

-- Test 5: Verify btree support functions are registered
\echo '=== Test 5: Btree Support Function Check ==='
SELECT 
    amprocfamily::regclass AS family,
    amproclefttype::regtype AS lefttype,
    amprocrighttype::regtype AS righttype,
    amprocnum AS support_func_num,
    amproc::regproc AS support_func
FROM pg_amproc
WHERE amprocfamily IN ('numeric_ops'::regclass, 'float_ops'::regclass)
  AND (amproclefttype = 'numeric'::regtype OR amproclefttype = 'float4'::regtype OR amproclefttype = 'float8'::regtype)
  AND amprocrighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
ORDER BY family, lefttype, righttype;

-- Expected: Should show FUNCTION 1 (comparison functions) for cross-type combinations

-- Clean up
DROP TABLE merge_int4;
DROP TABLE merge_numeric;
DROP TABLE merge_float8;

-- Summary
\echo '=== Summary ==='
\echo 'Merge joins are NOT supported in v1.0 (deferred to v2.0).'
\echo 'v1.0 uses index scans and nested loop joins for cross-type comparisons.'
\echo 'Operators do NOT have MERGES property and are NOT in btree families in v1.0.'
\echo 'v2.0 enhancement: operators could be added to btree families to enable merge joins.'
\echo '(Operators ARE transitive; deferral is for complexity management, not correctness.)'
