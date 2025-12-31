-- Comprehensive Performance Benchmark for pg_num2int_direct_comp
-- Compares extension performance vs. stock PostgreSQL behavior
--
-- Tests:
-- 1. Constant transformation (fractional → integer bounds)
-- 2. Cross-type SARG key lookups (index scan with cross-type predicates)
-- 3. Cross-type SARG range scans
-- 4. Join strategies: Hash, Merge, Indexed Nested Loop
-- 5. Stock PostgreSQL comparison (using explicit casts)
--
-- Setup (run once):
--   createdb pg_num2int_test
--   psql -d pg_num2int_test -c "CREATE EXTENSION pg_num2int_direct_comp;"
--
-- Usage:
--   psql -d pg_num2int_test -f benchmark_performance.sql

\timing on
\pset pager off

--------------------------------------------------------------------------------
-- Setup: Create test tables
--------------------------------------------------------------------------------

\echo '================================================================================'
\echo '=== SETUP: Creating test tables with 1M rows each ==='
\echo '================================================================================'

DROP TABLE IF EXISTS int_table CASCADE;
DROP TABLE IF EXISTS numeric_table CASCADE;
DROP TABLE IF EXISTS float_table CASCADE;

-- Table with integer primary key (1M rows)
CREATE TABLE int_table (
    id int4 PRIMARY KEY,
    val int4,
    data text
);
INSERT INTO int_table 
SELECT i, i % 10000, 'data_' || i 
FROM generate_series(1, 1000000) i;
CREATE INDEX idx_int_table_val ON int_table(val);

-- Table with numeric column for joins (1M rows)
CREATE TABLE numeric_table (
    id serial PRIMARY KEY,
    int_ref numeric,  -- references int_table.id as numeric
    amount numeric(10,2)
);
INSERT INTO numeric_table (int_ref, amount)
SELECT (random() * 999999 + 1)::int, random() * 1000
FROM generate_series(1, 1000000);
CREATE INDEX idx_numeric_table_int_ref ON numeric_table(int_ref);

-- Table with float8 column for joins (1M rows)
CREATE TABLE float_table (
    id serial PRIMARY KEY,
    int_ref float8,  -- references int_table.id as float8
    score float8
);
INSERT INTO float_table (int_ref, score)
SELECT (random() * 999999 + 1)::int, random() * 100
FROM generate_series(1, 1000000);
CREATE INDEX idx_float_table_int_ref ON float_table(int_ref);

ANALYZE int_table;
ANALYZE numeric_table;
ANALYZE float_table;

\echo 'Setup complete: 3 tables with 1M rows each'
\echo ''

--------------------------------------------------------------------------------
-- Test 1: Constant Transformation (fractional → integer bounds)
--------------------------------------------------------------------------------

\echo '================================================================================'
\echo '=== TEST 1: Constant Range Transformation ==='
\echo '=== Verifies: int_col > 10.5::numeric → int_col >= 11 ==='
\echo '================================================================================'

-- With extension: should transform and use index
\echo ''
\echo '--- 1a. Greater than fractional (val > 10.5::numeric) - should become val >= 11 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val > 10.5::numeric;

\echo ''
\echo '--- 1b. Less than fractional (val < 10.5::numeric) - should become val <= 10 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val < 10.5::numeric;

\echo ''
\echo '--- 1c. Greater than or equal fractional (val >= 10.5::numeric) - should become val >= 11 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val >= 10.5::numeric;

\echo ''
\echo '--- 1d. Equal to fractional (val = 10.5::numeric) - impossible, should be false ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val = 10.5::numeric;

\echo ''
\echo '--- 1e. Equal to whole number (val = 100.0::numeric) - should become val = 100 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val = 100.0::numeric;

--------------------------------------------------------------------------------
-- Test 2: Cross-Type SARG Key Lookups
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 2: Cross-Type SARG Key Lookups (Extension vs Stock) ==='
\echo '================================================================================'

\echo ''
\echo '--- 2a. WITH EXTENSION: id = 500000::numeric (should use Index Scan) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT * FROM int_table WHERE id = 500000::numeric;

\echo ''
\echo '--- 2b. STOCK EMULATION: id::numeric = 500000::numeric (forces Seq Scan) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT * FROM int_table WHERE id::numeric = 500000::numeric;

\echo ''
\echo '--- 2c. WITH EXTENSION: id = 500000::float8 (should use Index Scan) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT * FROM int_table WHERE id = 500000::float8;

\echo ''
\echo '--- 2d. STOCK EMULATION: id::float8 = 500000::float8 (forces Seq Scan) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT * FROM int_table WHERE id::float8 = 500000::float8;

--------------------------------------------------------------------------------
-- Test 3: Cross-Type SARG Range Scans
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 3: Cross-Type SARG Range Scans (Extension vs Stock) ==='
\echo '================================================================================'

\echo ''
\echo '--- 3a. WITH EXTENSION: val BETWEEN 100::numeric AND 200::numeric ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table WHERE val >= 100::numeric AND val <= 200::numeric;

\echo ''
\echo '--- 3b. STOCK EMULATION: val::numeric BETWEEN 100 AND 200 ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table WHERE val::numeric >= 100 AND val::numeric <= 200;

--------------------------------------------------------------------------------
-- Test 4: Hash Joins (Extension vs Stock)
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 4: Hash Joins (int4 = numeric) ==='
\echo '================================================================================'

-- Force hash join
SET enable_mergejoin = off;
SET enable_nestloop = off;
SET enable_hashjoin = on;

\echo ''
\echo '--- 4a. WITH EXTENSION: int_table.id = numeric_table.int_ref (Hash Join) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 4b. STOCK EMULATION: int_table.id::numeric = numeric_table.int_ref ---'
\echo '--- (May not get Hash Join without cross-type hash support) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

RESET enable_mergejoin;
RESET enable_nestloop;
RESET enable_hashjoin;

--------------------------------------------------------------------------------
-- Test 5: Merge Joins (Extension vs Stock) - int × numeric only
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 5: Merge Joins (int4 = numeric) ==='
\echo '================================================================================'

-- Force merge join
SET enable_hashjoin = off;
SET enable_nestloop = off;
SET enable_mergejoin = on;

\echo ''
\echo '--- 5a. WITH EXTENSION: int_table.id = numeric_table.int_ref (Merge Join) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 5b. STOCK EMULATION: int_table.id::numeric = numeric_table.int_ref ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

RESET enable_hashjoin;
RESET enable_nestloop;
RESET enable_mergejoin;

--------------------------------------------------------------------------------
-- Test 6: Indexed Nested Loop Joins (Extension vs Stock)
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 6: Indexed Nested Loop Joins ==='
\echo '================================================================================'

-- Force nested loop with index
SET enable_hashjoin = off;
SET enable_mergejoin = off;
SET enable_nestloop = on;

-- Small probe set for nested loop
\echo ''
\echo '--- 6a. WITH EXTENSION: Nested Loop with Index Scan on int_table ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) 
FROM (SELECT int_ref FROM numeric_table LIMIT 1000) n
JOIN int_table i ON i.id = n.int_ref;

\echo ''
\echo '--- 6b. STOCK EMULATION: Nested Loop WITHOUT Index (cast defeats index) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) 
FROM (SELECT int_ref FROM numeric_table LIMIT 1000) n
JOIN int_table i ON i.id::numeric = n.int_ref;

RESET enable_hashjoin;
RESET enable_mergejoin;
RESET enable_nestloop;

--------------------------------------------------------------------------------
-- Test 7: Float joins (Hash only, no Merge)
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 7: Float Joins (Hash and Nested Loop only, no Merge) ==='
\echo '================================================================================'

SET enable_mergejoin = off;

\echo ''
\echo '--- 7a. WITH EXTENSION: int_table.id = float_table.int_ref (Hash or NL) ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN float_table f ON i.id = f.int_ref;

\echo ''
\echo '--- 7b. STOCK EMULATION: int_table.id::float8 = float_table.int_ref ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN float_table f ON i.id::float8 = f.int_ref;

RESET enable_mergejoin;

--------------------------------------------------------------------------------
-- Test 8: Planner choice (let planner pick best strategy)
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 8: Planner Choice (default settings) ==='
\echo '================================================================================'

\echo ''
\echo '--- 8a. WITH EXTENSION: Planner picks best strategy for int=numeric join ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 8b. STOCK EMULATION: Planner constrained by cast ---'
EXPLAIN (ANALYZE, TIMING ON) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

--------------------------------------------------------------------------------
-- Summary
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== BENCHMARK COMPLETE ==='
\echo '================================================================================'
\echo ''
\echo 'Key observations to look for:'
\echo '1. Extension uses Index Scan; Stock uses Seq Scan (Test 2, 3)'
\echo '2. Extension enables Hash Join cross-type; Stock may not (Test 4)'
\echo '3. Extension enables Merge Join for int×numeric (Test 5)'
\echo '4. Extension enables Indexed Nested Loop; Stock does full scan (Test 6)'
\echo '5. Fractional constants are correctly transformed (Test 1)'
\echo ''

-- Cleanup (optional - comment out to inspect tables)
-- DROP TABLE IF EXISTS int_table CASCADE;
-- DROP TABLE IF EXISTS numeric_table CASCADE;
-- DROP TABLE IF EXISTS float_table CASCADE;
