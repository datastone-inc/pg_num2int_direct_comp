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
-- Stability techniques used:
-- - Disable parallel workers to eliminate coordination jitter
-- - High work_mem to force in-memory sorts (no disk spill variance)
-- - Warmup queries before timed runs
-- - Multiple runs per test for stability verification
--
-- NOTE: This is a long-running benchmark (~70 seconds) and is NOT included
-- in the default regression suite. To run manually:
--   make installcheck REGRESS=benchmark
--
-- For quick performance validation, use sql/performance.sql instead.

-- pg_regress loads the extension automatically via regression_setup
\pset pager off

--------------------------------------------------------------------------------
-- Stability Settings
--------------------------------------------------------------------------------

-- Disable parallel workers to eliminate worker startup/coordination variance
SET max_parallel_workers_per_gather = 0;

-- High work_mem forces in-memory quicksort instead of external merge
-- This eliminates disk I/O variance between runs
SET work_mem = '256MB';

-- Ensure stable query plans
SET random_page_cost = 1.1;  -- Encourage index usage on SSD

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

-- Seed random for reproducible test data
SELECT setseed(0.42);

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
-- Warmup: Load tables into buffer cache
--------------------------------------------------------------------------------

\echo '================================================================================'
\echo '=== WARMUP: Loading tables into buffer cache ==='
\echo '================================================================================'

-- Sequential scan to warm buffer cache
SELECT COUNT(*) FROM int_table;
SELECT COUNT(*) FROM numeric_table;
SELECT COUNT(*) FROM float_table;

-- Touch indexes
SELECT COUNT(*) FROM int_table WHERE id = 1;
SELECT COUNT(*) FROM int_table WHERE val = 1;
SELECT COUNT(*) FROM numeric_table WHERE int_ref = 1;
SELECT COUNT(*) FROM float_table WHERE int_ref = 1;

\echo 'Warmup complete: tables and indexes in buffer cache'
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
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT * FROM int_table WHERE id = 500000::numeric;

\echo ''
\echo '--- 2b. STOCK EMULATION: id::numeric = 500000::numeric (forces Seq Scan) ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT * FROM int_table WHERE id::numeric = 500000::numeric;

\echo ''
\echo '--- 2c. WITH EXTENSION: id = 500000::float8 (should use Index Scan) ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT * FROM int_table WHERE id = 500000::float8;

\echo ''
\echo '--- 2d. STOCK EMULATION: id::float8 = 500000::float8 (forces Seq Scan) ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
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
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val >= 100::numeric AND val <= 200::numeric;

\echo ''
\echo '--- 3b. STOCK EMULATION: val::numeric BETWEEN 100 AND 200 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table WHERE val::numeric >= 100 AND val::numeric <= 200;

--------------------------------------------------------------------------------
-- Test 4: Hash Joins (Extension vs Stock) - 3 runs each
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 4: Hash Joins (int4 = numeric) - 3 runs each ==='
\echo '================================================================================'

-- Force hash join
SET enable_mergejoin = off;
SET enable_nestloop = off;
SET enable_hashjoin = on;

-- Warmup
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

\echo ''
\echo '--- 4a. WITH EXTENSION: int_table.id = numeric_table.int_ref (Hash Join) ---'
\echo '--- Run 1 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
\echo '--- Run 2 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
\echo '--- Run 3 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 4b. STOCK EMULATION: int_table.id::numeric = numeric_table.int_ref ---'
\echo '--- Run 1 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;
\echo '--- Run 2 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;
\echo '--- Run 3 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

RESET enable_mergejoin;
RESET enable_nestloop;
RESET enable_hashjoin;

--------------------------------------------------------------------------------
-- Test 5: Merge Joins (Extension vs Stock) - int × numeric only
-- Run 3 times each for stability (take median)
--
-- NOTE: We use enable_seqscan = off to force both plans to use the numeric
-- index (idx_numeric_table_int_ref). Without this, PostgreSQL's cost model
-- picks different plan shapes:
--   - Extension: Index Scan int_table_pkey + Sort numeric_table
--   - Stock: Index Scan idx_numeric_table_int_ref + Sort int_table::numeric
--
-- The planner's cost model treats all comparison functions equally
-- (cpu_operator_cost = 0.0025), so it doesn't know that sorting int4 values
-- is ~4x faster than sorting numeric values. This causes it to prefer the
-- slower plan when using the extension.
--
-- By forcing both queries to use the numeric index, we get identical plan
-- shapes and a fair apples-to-apples comparison of the join execution itself.
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== TEST 5: Merge Joins (int4 = numeric) - 3 runs each ==='
\echo '=== (enable_seqscan=off forces optimal plan for fair comparison) ==='
\echo '================================================================================'

-- Force merge join with optimal plan
SET enable_hashjoin = off;
SET enable_nestloop = off;
SET enable_mergejoin = on;
SET enable_seqscan = off;  -- Force use of idx_numeric_table_int_ref

-- Warmup runs (not timed in output)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

\echo ''
\echo '--- 5a. WITH EXTENSION: int_table.id = numeric_table.int_ref (Merge Join) ---'
\echo '--- Run 1 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
\echo '--- Run 2 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;
\echo '--- Run 3 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 5b. STOCK EMULATION: int_table.id::numeric = numeric_table.int_ref ---'
\echo '--- Run 1 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;
\echo '--- Run 2 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF)
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;
\echo '--- Run 3 ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

RESET enable_hashjoin;
RESET enable_nestloop;
RESET enable_mergejoin;
RESET enable_seqscan;

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
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) 
FROM (SELECT int_ref FROM numeric_table LIMIT 1000) n
JOIN int_table i ON i.id = n.int_ref;

\echo ''
\echo '--- 6b. STOCK EMULATION: Nested Loop WITHOUT Index (cast defeats index) ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
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
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN float_table f ON i.id = f.int_ref;

\echo ''
\echo '--- 7b. STOCK EMULATION: int_table.id::float8 = float_table.int_ref ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
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
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id = n.int_ref;

\echo ''
\echo '--- 8b. STOCK EMULATION: Planner constrained by cast ---'
EXPLAIN (ANALYZE, COSTS OFF, TIMING OFF) 
SELECT COUNT(*) FROM int_table i JOIN numeric_table n ON i.id::numeric = n.int_ref;

--------------------------------------------------------------------------------
-- Summary
--------------------------------------------------------------------------------

\echo ''
\echo '================================================================================'
\echo '=== BENCHMARK COMPLETE ==='
\echo '================================================================================'
\echo ''
\echo 'Stability settings used:'
\echo '  - max_parallel_workers_per_gather = 0 (no parallel worker jitter)'
\echo '  - work_mem = 256MB (in-memory sorts, no disk spill variance)'
\echo '  - Warmup queries before timed runs (hot buffer cache)'
\echo '  - 3 runs per join test (take median for reporting)'
\echo ''
\echo 'Key observations to look for:'
\echo '1. Extension uses Index Scan; Stock uses Seq Scan (Test 2, 3)'
\echo '2. Extension enables Hash Join cross-type (Test 4)'
\echo '3. Extension enables Merge Join for int×numeric (Test 5)'
\echo '4. Extension enables Indexed Nested Loop; Stock does full scan (Test 6)'
\echo '5. Fractional constants are correctly transformed (Test 1)'
\echo ''

-- Reset all settings
RESET ALL;

-- Cleanup (optional - comment out to inspect tables)
-- DROP TABLE IF EXISTS int_table CASCADE;
-- DROP TABLE IF EXISTS numeric_table CASCADE;
-- DROP TABLE IF EXISTS float_table CASCADE;
