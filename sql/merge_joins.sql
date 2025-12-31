-- Test: Merge Join NOT Supported for Cross-Type Comparisons
-- Purpose: Demonstrate that merge joins are NOT currently supported
--
-- What happens: When merge join is forced, PostgreSQL falls back to hash or nested loop
--
-- Note: This is v1.0 behavior. Future versions may enable merge joins by adding
--       operators to integer_ops family after further analysis.

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

-- Test 1: Force merge join and observe fallback
\echo '=== Test 1: Merge Join NOT Used (Falls Back to Hash Join) ==='
SET enable_nestloop = off;
SET enable_hashjoin = on;
SET enable_mergejoin = on;

EXPLAIN (COSTS OFF)
SELECT COUNT(*) 
FROM merge_int4 i 
JOIN merge_numeric n ON i.val = n.val;

-- Expected: Hash Join (NOT Merge Join)
-- Reason: Cannot use merge join because operators not in integer_ops family

-- Test 2: Try to force merge join by disabling hash
\echo '=== Test 2: Disable Hash, Still No Merge Join ==='
SET enable_hashjoin = off;

EXPLAIN (COSTS OFF)
SELECT COUNT(*) 
FROM merge_int4 i 
JOIN merge_numeric n ON i.val = n.val;

-- Expected: Nested Loop (still NOT Merge Join)
-- PostgreSQL cannot use merge join for cross-type comparison

RESET enable_nestloop;
RESET enable_hashjoin;
RESET enable_mergejoin;

-- Test 3: Verify operators ARE in btree operator families (but not integer_ops)
\echo '=== Test 3: Operator Family Membership Check ==='
SELECT 
    op.oprname,
    op.oprleft::regtype,
    op.oprright::regtype,
    opf.opfname as opfamily
FROM pg_operator op
JOIN pg_amop amop ON op.oid = amop.amopopr
JOIN pg_opfamily opf ON amop.amopfamily = opf.oid
WHERE op.oprname IN ('=', '<', '<=', '>', '>=', '<>')
  AND (
    (op.oprleft = 'numeric'::regtype AND op.oprright = 'int4'::regtype) OR
    (op.oprleft = 'int4'::regtype AND op.oprright = 'numeric'::regtype) OR
    (op.oprleft = 'float8'::regtype AND op.oprright = 'int4'::regtype) OR
    (op.oprleft = 'int4'::regtype AND op.oprright = 'float8'::regtype)
  )
ORDER BY op.oprname, op.oprleft, op.oprright, opf.opfname;

-- Expected: Operators in numeric_ops and float_ops, NOT in integer_ops

-- Test 4: Verify operators NOT in integer_ops family
\echo '=== Test 4: Operators NOT in integer_ops Family ==='
SELECT 
    op.oprname,
    op.oprleft::regtype,
    op.oprright::regtype,
    opf.opfname as opfamily
FROM pg_operator op
JOIN pg_amop amop ON op.oid = amop.amopopr
JOIN pg_opfamily opf ON amop.amopfamily = opf.oid
WHERE op.oprname = '='
  AND opf.opfname = 'integer_ops'
  AND (
    (op.oprleft = 'numeric'::regtype AND op.oprright IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)) OR
    (op.oprright = 'numeric'::regtype AND op.oprleft IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype))
  );

-- Expected: No rows (operators not in integer_ops)
-- This is why merge joins don't work

-- Test 5: Verify btree support functions are registered
\echo '=== Test 5: Btree Support Function Check ==='
SELECT 
    opf.opfname::text AS family,
    amproclefttype::regtype AS lefttype,
    amprocrighttype::regtype AS righttype,
    amprocnum AS support_func_num,
    amproc::regproc AS support_func
FROM pg_amproc
JOIN pg_opfamily opf ON amprocfamily = opf.oid
WHERE opf.opfname IN ('numeric_ops', 'float_ops')
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
\echo 'Merge joins are NOT supported in v1.0.'
\echo 'Reason: Cross-type operators only in numeric_ops/float_ops, not in integer_ops.'
\echo 'PostgreSQL requires operators in integer_ops to merge join from the integer side.'
\echo 'v1.0 provides: Hash joins and indexed nested loop joins (see index_nested_loop.sql).'
\echo 'Future: May enable merge joins by carefully adding to integer_ops family.'
