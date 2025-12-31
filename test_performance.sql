-- Performance benchmark for index-optimized queries
-- Verify sub-millisecond execution on 1M+ row table
--
-- Setup (run once):
--   createdb pg_num2int_test
--   psql -d pg_num2int_test -c "CREATE EXTENSION pg_num2int_direct_comp;"
--
-- Usage:
--   psql -d pg_num2int_test -f test_performance.sql

\timing on

-- Create extension (idempotent)
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;

-- Create large test table with 1M rows
DROP TABLE IF EXISTS perf_test;
CREATE TABLE perf_test (
  id serial PRIMARY KEY,
  val int4
);

-- Populate with 1M rows
INSERT INTO perf_test (val) SELECT i FROM generate_series(1, 1000000) i;

-- Create index
CREATE INDEX idx_perf_test_val ON perf_test(val);

-- Analyze for statistics
ANALYZE perf_test;

-- Disable sequential scans to force index usage if possible
SET enable_seqscan = off;

-- Warm up the cache
SELECT COUNT(*) FROM perf_test WHERE val = 500000;

\echo '\n=== Test 1: numeric constant (should use Index Scan) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE val = 500000::numeric;

\echo '\n=== Test 2: float8 constant (should use Index Scan) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE val = 500000::float8;

\echo '\n=== Test 3: float4 constant (should use Index Scan) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE val = 500000::float4;

\echo '\n=== Test 4: Commutator direction - numeric (should use Index Scan) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE 500000::numeric = val;

\echo '\n=== Test 5: Multiple rows (should still be fast) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE val = 1000::numeric;

\echo '\n=== Test 6: Non-existent value (should be instant) ==='
EXPLAIN ANALYZE SELECT * FROM perf_test WHERE val = 1500000::numeric;

-- Cleanup
DROP TABLE perf_test;
