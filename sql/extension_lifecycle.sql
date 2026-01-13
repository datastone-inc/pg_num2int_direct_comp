-- Extension Lifecycle Tests
-- Verifies the extension can be installed correctly and that DROP/CREATE
-- cycles work properly. This test requires superuser privileges.
--
-- Tests:
-- 1. OID stability: hardcoded PostgreSQL OIDs match the system catalog
-- 2. Extension cleanup: event trigger removes operator family entries on DROP
-- 3. Syscache invalidation: OID cache is refreshed after DROP/CREATE

-- ============================================================================
-- OID Stability Tests
-- ============================================================================
-- Verifies that hardcoded PostgreSQL built-in OIDs in pg_num2int_direct_comp.h
-- match the actual system catalog values. These OIDs should be stable across
-- all PostgreSQL versions, but this test provides early warning if they change.

-- Cast function OIDs (NUM2INT_CAST_NUMERIC_INT* in header)
SELECT 
    CASE WHEN oid = 1783 THEN 'PASS' ELSE 'FAIL: expected 1783, got ' || oid END AS int2_numeric_cast
FROM pg_proc WHERE proname = 'int2' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype;

SELECT 
    CASE WHEN oid = 1744 THEN 'PASS' ELSE 'FAIL: expected 1744, got ' || oid END AS int4_numeric_cast
FROM pg_proc WHERE proname = 'int4' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype;

SELECT 
    CASE WHEN oid = 1779 THEN 'PASS' ELSE 'FAIL: expected 1779, got ' || oid END AS int8_numeric_cast
FROM pg_proc WHERE proname = 'int8' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype;

-- Integer comparison operator OIDs (NUM2INT_INT*_OID in header)
-- Numeric function OIDs (NUM2INT_NUMERIC_* in header)
WITH oid_checks AS (
    SELECT 1783 AS expected, oid AS actual, 'int2(numeric)' AS name FROM pg_proc WHERE proname = 'int2' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype
    UNION ALL SELECT 1744, oid, 'int4(numeric)' FROM pg_proc WHERE proname = 'int4' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype
    UNION ALL SELECT 1779, oid, 'int8(numeric)' FROM pg_proc WHERE proname = 'int8' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype
    UNION ALL SELECT 1710, oid, 'trunc(numeric)' FROM pg_proc WHERE proname = 'trunc' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype
    UNION ALL SELECT 1712, oid, 'floor(numeric)' FROM pg_proc WHERE proname = 'floor' AND pronargs = 1 AND proargtypes[0] = 'numeric'::regtype
    UNION ALL SELECT 1718, oid, 'numeric_eq(numeric,numeric)' FROM pg_proc WHERE proname = 'numeric_eq' AND pronargs = 2 AND proargtypes[0] = 'numeric'::regtype AND proargtypes[1] = 'numeric'::regtype
    UNION ALL SELECT 94, oid, 'int2 = int2' FROM pg_operator WHERE oprname = '=' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 95, oid, 'int2 < int2' FROM pg_operator WHERE oprname = '<' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 520, oid, 'int2 > int2' FROM pg_operator WHERE oprname = '>' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 522, oid, 'int2 <= int2' FROM pg_operator WHERE oprname = '<=' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 524, oid, 'int2 >= int2' FROM pg_operator WHERE oprname = '>=' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 519, oid, 'int2 <> int2' FROM pg_operator WHERE oprname = '<>' AND oprleft = 'int2'::regtype AND oprright = 'int2'::regtype
    UNION ALL SELECT 96, oid, 'int4 = int4' FROM pg_operator WHERE oprname = '=' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 97, oid, 'int4 < int4' FROM pg_operator WHERE oprname = '<' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 521, oid, 'int4 > int4' FROM pg_operator WHERE oprname = '>' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 523, oid, 'int4 <= int4' FROM pg_operator WHERE oprname = '<=' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 525, oid, 'int4 >= int4' FROM pg_operator WHERE oprname = '>=' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 518, oid, 'int4 <> int4' FROM pg_operator WHERE oprname = '<>' AND oprleft = 'int4'::regtype AND oprright = 'int4'::regtype
    UNION ALL SELECT 410, oid, 'int8 = int8' FROM pg_operator WHERE oprname = '=' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
    UNION ALL SELECT 412, oid, 'int8 < int8' FROM pg_operator WHERE oprname = '<' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
    UNION ALL SELECT 413, oid, 'int8 > int8' FROM pg_operator WHERE oprname = '>' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
    UNION ALL SELECT 414, oid, 'int8 <= int8' FROM pg_operator WHERE oprname = '<=' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
    UNION ALL SELECT 415, oid, 'int8 >= int8' FROM pg_operator WHERE oprname = '>=' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
    UNION ALL SELECT 411, oid, 'int8 <> int8' FROM pg_operator WHERE oprname = '<>' AND oprleft = 'int8'::regtype AND oprright = 'int8'::regtype
)
SELECT 
    COUNT(*) FILTER (WHERE expected != actual) AS oid_mismatches,
    CASE WHEN COUNT(*) FILTER (WHERE expected != actual) = 0 
         THEN 'All hardcoded OIDs match system catalog' 
         ELSE 'WARNING: OID mismatches detected!' 
    END AS status
FROM oid_checks;

-- ============================================================================
-- Internal Numeric Format Validation
-- ============================================================================
-- These tests verify that our internal Numeric format macros (NUM2INT_NUMERIC_*)
-- correctly interpret PostgreSQL's Numeric storage format. If PostgreSQL ever
-- changes the internal Numeric format, these tests should catch it.
--
-- Tests cover: NaN/Inf detection, sign extraction, weight calculation,
-- integral vs fractional detection, and digit array interpretation.

-- Special Values: NaN and Infinity (tests NUM2INT_NUMERIC_IS_NAN/PINF/NINF)
SELECT 'NaN = 0'::text AS test, 'NaN'::numeric = 0::int4 AS result;
SELECT 'NaN <> 0'::text AS test, 'NaN'::numeric <> 0::int4 AS result;
SELECT '+Inf > max_int8'::text AS test, 'Infinity'::numeric > 9223372036854775807::int8 AS result;
SELECT '-Inf < min_int8'::text AS test, '-Infinity'::numeric < (-9223372036854775808)::int8 AS result;

-- Zero handling (ndigits=0 in Numeric format)
SELECT '0 = 0'::text AS test, 0::numeric = 0::int4 AS result;
SELECT '0.0 = 0'::text AS test, 0.0::numeric = 0::int4 AS result;

-- Sign detection (NUM2INT_NUMERIC_SIGN)
SELECT '100 > 0'::text AS test, 100::numeric > 0::int4 AS result;
SELECT '-100 < 0'::text AS test, (-100)::numeric < 0::int4 AS result;

-- Weight and digit array (NUM2INT_NUMERIC_WEIGHT, NUM2INT_NUMERIC_NDIGITS)
SELECT '9999 = 9999'::text AS test, 9999::numeric = 9999::int4 AS result;  -- single base-10000 digit
SELECT '10000 = 10000'::text AS test, 10000::numeric = 10000::int4 AS result;  -- weight=1
SELECT 'max_int8 exact'::text AS test, 9223372036854775807::numeric = 9223372036854775807::int8 AS result;
SELECT 'min_int8 exact'::text AS test, (-9223372036854775808)::numeric = (-9223372036854775808)::int8 AS result;

-- Fractional detection (ndigits > weight + 1)
SELECT '0.5 = 0'::text AS test, 0.5::numeric = 0::int4 AS result;  -- false
SELECT '100.5 > 100'::text AS test, 100.5::numeric > 100::int4 AS result;  -- true
SELECT '-100.5 > -101'::text AS test, (-100.5)::numeric > (-101)::int4 AS result;  -- true (floor test)
SELECT '-0.5 > -1'::text AS test, (-0.5)::numeric > (-1)::int4 AS result;  -- true

-- Huge values beyond int64 range (sign-based comparison)
SELECT 'huge_pos > max_int8'::text AS test, 
    99999999999999999999::numeric > 9223372036854775807::int8 AS result;
SELECT 'huge_neg < min_int8'::text AS test, 
    (-99999999999999999999)::numeric < (-9223372036854775808)::int8 AS result;

-- Trailing zeros (storage normalization)
SELECT '100.00 = 100'::text AS test, 100.00::numeric = 100::int4 AS result;

-- ============================================================================
-- Extension Cleanup Tests
-- ============================================================================
-- The extension cleanup event trigger fires on DROP EXTENSION and removes
-- operators/functions added to built-in operator families. Without this
-- cleanup, CREATE EXTENSION would fail with "operator already exists".

-- Start with extension loaded (from setup)
SELECT extname FROM pg_extension WHERE extname = 'pg_num2int_direct_comp';

-- Verify event trigger exists and is owned by extension
SELECT evtname, evtenabled 
FROM pg_event_trigger 
WHERE evtname = 'pg_num2int_direct_comp_drop_trigger';

-- Verify the trigger is part of the extension
SELECT pg_describe_object(d.classid, d.objid, d.objsubid) AS object_desc
FROM pg_depend d
JOIN pg_extension e ON e.oid = d.refobjid
WHERE d.refclassid = 'pg_extension'::regclass
AND e.extname = 'pg_num2int_direct_comp'
AND d.classid = 'pg_event_trigger'::regclass;

-- Test basic functionality before DROP
SELECT 10::numeric = 10::int4 AS before_drop_test;

-- DROP and recreate extension
DROP EXTENSION pg_num2int_direct_comp;

-- Verify no operators remain in numeric_ops for int types
-- (These would have been added by the extension and should be cleaned up)
SELECT COUNT(*) AS leftover_operators
FROM pg_amop ao
JOIN pg_opfamily of ON ao.amopfamily = of.oid
JOIN pg_am am ON of.opfmethod = am.oid
WHERE of.opfname = 'numeric_ops' 
AND am.amname = 'btree'
AND ao.amoplefttype = 'int4'::regtype
AND ao.amoprighttype = 'int4'::regtype;

-- Verify no int×float operators remain in float_ops (new test for btree family integration)
SELECT COUNT(*) AS leftover_float_ops
FROM pg_amop ao
JOIN pg_opfamily of ON ao.amopfamily = of.oid
JOIN pg_am am ON of.opfmethod = am.oid
WHERE of.opfname = 'float_ops' 
AND am.amname = 'btree'
AND ((ao.amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND ao.amoprighttype IN ('float4'::regtype, 'float8'::regtype))
     OR (ao.amoplefttype IN ('float4'::regtype, 'float8'::regtype) AND ao.amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)));

-- Verify no float×int operators remain in integer_ops (new test for btree family integration)  
SELECT COUNT(*) AS leftover_integer_ops
FROM pg_amop ao
JOIN pg_opfamily of ON ao.amopfamily = of.oid
JOIN pg_am am ON of.opfmethod = am.oid
WHERE of.opfname = 'integer_ops'
AND am.amname = 'btree'
AND ((ao.amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND ao.amoprighttype IN ('float4'::regtype, 'float8'::regtype))
     OR (ao.amoplefttype IN ('float4'::regtype, 'float8'::regtype) AND ao.amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)));

-- Recreate extension
CREATE EXTENSION pg_num2int_direct_comp;

-- Test functionality after reinstall
SELECT 10::numeric = 10::int4 AS after_reinstall_test;

-- Test int×float operators after reinstall (new test for btree family integration)
SELECT 10::int4 = 10.0::float4 AS int4_float4_reinstall_test;
SELECT 10::int4 = 10.0::float8 AS int4_float8_reinstall_test;

-- Verify btree family registrations were recreated correctly
SELECT COUNT(*) AS recreated_float_ops
FROM pg_amop ao
JOIN pg_opfamily of ON ao.amopfamily = of.oid
JOIN pg_am am ON of.opfmethod = am.oid
WHERE of.opfname = 'float_ops' 
AND am.amname = 'btree'
AND ((ao.amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND ao.amoprighttype IN ('float4'::regtype, 'float8'::regtype))
     OR (ao.amoplefttype IN ('float4'::regtype, 'float8'::regtype) AND ao.amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)));

SELECT COUNT(*) AS recreated_integer_ops
FROM pg_amop ao
JOIN pg_opfamily of ON ao.amopfamily = of.oid
JOIN pg_am am ON of.opfmethod = am.oid
WHERE of.opfname = 'integer_ops'
AND am.amname = 'btree'
AND ((ao.amoplefttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND ao.amoprighttype IN ('float4'::regtype, 'float8'::regtype))
     OR (ao.amoplefttype IN ('float4'::regtype, 'float8'::regtype) AND ao.amoprighttype IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)));

-- Verify event trigger was recreated
SELECT evtname FROM pg_event_trigger 
WHERE evtname = 'pg_num2int_direct_comp_drop_trigger';

-- Second DROP/CREATE cycle
DROP EXTENSION pg_num2int_direct_comp;
CREATE EXTENSION pg_num2int_direct_comp;
SELECT 10::numeric = 10::int4 AS second_cycle_test;

-- ============================================================================
-- Test syscache callback: OID cache must be invalidated after DROP/CREATE
-- Without proper cache invalidation, support function won't recognize new
-- operator OIDs and index optimization will fail (shows '42'::numeric instead of 42)
-- ============================================================================

-- First, use the cross-type operator to populate the OID cache
CREATE TEMPORARY TABLE lifecycle_test (id INT4 PRIMARY KEY);
INSERT INTO lifecycle_test VALUES (42);
ANALYZE lifecycle_test;

-- Use a prepared statement to trigger the support function
PREPARE lifecycle_find(numeric) AS SELECT * FROM lifecycle_test WHERE id = $1;
EXPLAIN (COSTS OFF) EXECUTE lifecycle_find(42);
-- Should show: Filter: (id = 42)  -- NOT '42'::numeric

DEALLOCATE lifecycle_find;

-- DROP and CREATE to get new operator OIDs
DROP EXTENSION pg_num2int_direct_comp;
CREATE EXTENSION pg_num2int_direct_comp;

-- Test that support function works with NEW operator OIDs
-- (this fails if syscache callback doesn't invalidate the cache)
PREPARE lifecycle_find2(numeric) AS SELECT * FROM lifecycle_test WHERE id = $1;
EXPLAIN (COSTS OFF) EXECUTE lifecycle_find2(42);
-- Should show: Filter: (id = 42)  -- NOT '42'::numeric

DEALLOCATE lifecycle_find2;
DROP TABLE lifecycle_test;

-- ============================================================================
-- Hash Function Compatibility Tests
-- ============================================================================
-- Verifies that our optimized hash functions (which avoid palloc) produce
-- identical results to PostgreSQL's hash_numeric. This ensures hash joins
-- work correctly when comparing integers to numerics.

-- Test hash_int8_as_numeric matches hash_numeric for various values
SELECT 
    bool_and(hash_int8_as_numeric(val) = hash_numeric(val::numeric)) AS hash_match
FROM (VALUES 
    (0::int8), (1), (-1), (12345), (-12345),
    (10000), (100000000), (9999999900000001),
    (9223372036854775807), (-9223372036854775808)
) AS t(val);

-- Test hash_int8_as_numeric_extended matches hash_numeric_extended
SELECT 
    bool_and(hash_int8_as_numeric_extended(val, 12345::int8) = 
             hash_numeric_extended(val::numeric, 12345::int8)) AS extended_hash_match
FROM (VALUES 
    (0::int8), (1), (-1), (12345), (-12345),
    (10000), (100000000), (9999999900000001),
    (9223372036854775807), (-9223372036854775808)
) AS t(val);

-- Final verification
SELECT 'Extension lifecycle test passed' AS result;

-- ============================================================================
-- GUC Configuration Tests  
-- ============================================================================
-- Test that the GUC parameter works correctly for enabling/disabling optimizations

-- Check GUC exists and has correct default value
SHOW pg_num2int_direct_comp.enable_support_functions;

-- Test setting the GUC to different values
SET pg_num2int_direct_comp.enable_support_functions = off;
SHOW pg_num2int_direct_comp.enable_support_functions;

SET pg_num2int_direct_comp.enable_support_functions = on;  
SHOW pg_num2int_direct_comp.enable_support_functions;

-- Create test table for optimization verification
CREATE TEMPORARY TABLE guc_test (val int8);
INSERT INTO guc_test VALUES (9007199254740992);

-- Test with optimizations disabled (should show cross-type comparison)
SET pg_num2int_direct_comp.enable_support_functions = off;
EXPLAIN (VERBOSE, COSTS off) 
SELECT * FROM guc_test WHERE 9007199254740993.0::float8 = val;

-- Test with optimizations enabled (should show same-type comparison with transformed constant)
SET pg_num2int_direct_comp.enable_support_functions = on;
EXPLAIN (VERBOSE, COSTS off)
SELECT * FROM guc_test WHERE 9007199254740993.0::float8 = val;

-- Verify actual query results are correct in both modes
SET pg_num2int_direct_comp.enable_support_functions = off;
SELECT count(*) AS disabled_count FROM guc_test WHERE 9007199254740993.0::float8 = val;

SET pg_num2int_direct_comp.enable_support_functions = on;
SELECT count(*) AS enabled_count FROM guc_test WHERE 9007199254740993.0::float8 = val;

-- Cleanup
DROP TABLE guc_test;

SELECT 'GUC configuration test passed' AS result;
