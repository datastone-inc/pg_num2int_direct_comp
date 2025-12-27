-- Test hash joins with cross-type equality operators
-- This file tests that our operators support hash join optimization

\echo '=== Test 1: Hash Join with numeric = int4 ==='
CREATE TEMP TABLE hash_numeric (val numeric, id int);
CREATE TEMP TABLE hash_int4 (val int4, id int);
INSERT INTO hash_numeric SELECT i::numeric, i FROM generate_series(1, 100) i;
INSERT INTO hash_int4 SELECT i, i FROM generate_series(1, 100) i;

-- Force hash join
SET enable_nestloop = off;
SET enable_mergejoin = off;

EXPLAIN (COSTS OFF) 
SELECT COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val;

SELECT COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val;

\echo '=== Test 1b: Hash Aggregate with GROUP BY ==='
EXPLAIN (COSTS OFF)
SELECT i.val, COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val GROUP BY i.val;

SELECT i.val, COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val GROUP BY i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT i.val, n.val, COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val GROUP BY i.val, n.val;

SELECT i.val, n.val, COUNT(*) FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val GROUP BY i.val, n.val ORDER BY i.val LIMIT 10;

\echo '=== Test 1c: Hash Aggregate with DISTINCT ==='
EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val;

SELECT DISTINCT i.val FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val, n.val FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val;

SELECT DISTINCT i.val, n.val FROM hash_numeric n JOIN hash_int4 i ON n.val = i.val ORDER BY i.val LIMIT 10;

\echo '=== Test 2: Hash Join with float8 = int8 ==='
CREATE TEMP TABLE hash_float8 (val float8);
CREATE TEMP TABLE hash_int8 (val int8);
INSERT INTO hash_float8 SELECT i::float8 FROM generate_series(1, 50) i;
INSERT INTO hash_int8 SELECT i FROM generate_series(1, 50) i;

EXPLAIN (COSTS OFF)
SELECT COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val;

SELECT COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val;

\echo '=== Test 2b: Hash Aggregate with GROUP BY ==='
EXPLAIN (COSTS OFF)
SELECT i.val, COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val GROUP BY i.val;

SELECT i.val, COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val GROUP BY i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT i.val, f.val, COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val GROUP BY i.val, f.val;

SELECT i.val, f.val, COUNT(*) FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val GROUP BY i.val, f.val ORDER BY i.val LIMIT 10;

\echo '=== Test 2c: Hash Aggregate with DISTINCT ==='
EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val;

SELECT DISTINCT i.val FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val, f.val FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val;

SELECT DISTINCT i.val, f.val FROM hash_float8 f JOIN hash_int8 i ON f.val = i.val ORDER BY i.val LIMIT 10;

\echo '=== Test 3: Hash Join with float4 = int2 ==='
CREATE TEMP TABLE hash_float4 (val float4);
CREATE TEMP TABLE hash_int2 (val int2);
INSERT INTO hash_float4 SELECT i::float4 FROM generate_series(1, 20) i;
INSERT INTO hash_int2 SELECT i::int2 FROM generate_series(1, 20) i;

EXPLAIN (COSTS OFF)
SELECT COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val;

SELECT COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val;

\echo '=== Test 3c: Hash Aggregate with DISTINCT ==='
EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val;

SELECT DISTINCT i.val FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT DISTINCT i.val, f.val FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val;

SELECT DISTINCT i.val, f.val FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val ORDER BY i.val LIMIT 10;

\echo '=== Test 3b: Hash Aggregate with GROUP BY ==='
EXPLAIN (COSTS OFF)
SELECT i.val, COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val GROUP BY i.val;

SELECT i.val, COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val GROUP BY i.val ORDER BY i.val LIMIT 10;

EXPLAIN (COSTS OFF)
SELECT i.val, f.val, COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val GROUP BY i.val, f.val;

SELECT i.val, f.val, COUNT(*) FROM hash_float4 f JOIN hash_int2 i ON f.val = i.val GROUP BY i.val, f.val ORDER BY i.val LIMIT 10;

\echo '=== Test 4: Verify HASHES property ==='
-- Check that our equality operators have the HASHES property
SELECT oprname, oprleft::regtype, oprright::regtype, oprcanhash
FROM pg_operator
WHERE oprname = '='
  AND (oprleft::regtype::text = 'numeric' AND oprright::regtype::text IN ('smallint', 'integer', 'bigint')
       OR oprleft::regtype::text IN ('smallint', 'integer', 'bigint') AND oprright::regtype::text = 'numeric')
ORDER BY oprleft, oprright;

\echo '=== Test 5: Hash consistency check ==='
-- Verify that equal values hash consistently across types
SELECT hash_int4_as_numeric(10) = hash_int4_as_numeric(10) AS int4_consistent;
SELECT hash_int8_as_numeric(100) = hash_int8_as_numeric(100) AS int8_consistent;
SELECT hash_int4_as_float8(10) = hash_int4_as_float8(10) AS float8_consistent;
