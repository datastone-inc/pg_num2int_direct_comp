-- Test: Documentation Examples
-- Purpose: Verify all SQL examples in user documentation work correctly
-- This file ensures documentation examples remain accurate as the codebase evolves.
--
-- Constitution requirement: All SQL code blocks in documentation (README.md, doc/*.md)
-- MUST have corresponding regression tests in this file.

-- Load extension (suppress notice about already exists when run after other tests)
SET client_min_messages = warning;
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;
SET client_min_messages = notice;

-- ============================================================================
-- README.md: Wrong results, seq scans, and no transitivity from cross-type comparison (~line 152)
-- Without extension: returns wrong results (2 rows instead of 1), uses seq scans, no transitivity
-- ============================================================================

-- Without extension (verifies documented stock PostgreSQL behavior)
DROP EXTENSION pg_num2int_direct_comp;

CREATE TEMPORARY TABLE orders (orderid int8 PRIMARY KEY, customer text);
CREATE TEMPORARY TABLE order_items (id serial, orderid int8, product text);
CREATE INDEX ON order_items(orderid);

-- Bulk insert, plus two orders with adjacent IDs at the float8 precision boundary
INSERT INTO orders SELECT g, 'customer' || g FROM generate_series(1, 100000) g;
INSERT INTO orders VALUES (9007199254740992, 'Alice'), (9007199254740993, 'Bob');

INSERT INTO order_items (orderid, product) SELECT g, 'product' || g FROM generate_series(1, 100000) g;
INSERT INTO order_items VALUES (DEFAULT, 9007199254740992, 'Widget'), (DEFAULT, 9007199254740993, 'Gadget');
ANALYZE orders; ANALYZE order_items;

-- Stock PostgreSQL: returns BOTH rows (wrong!) and uses seq scan (slow!)
-- Application queries for Alice's order (9007199254740992) using float8 parameter
EXPLAIN (COSTS OFF)
SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;

SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;

-- With extension (behavior documented - index scan, 1 row)
CREATE EXTENSION pg_num2int_direct_comp;

-- Same exact SQL - should now return correct results with index scans
EXPLAIN (COSTS OFF)
SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;

SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;

DROP TABLE orders, order_items;

-- ============================================================================
-- README.md: Transitive Equality Violations not detected at the float8 2^53 boundary (different at the last digit) (~line 112)
-- Stock PostgreSQL violates transitivity with int8/float8 at 2^53 boundary.
-- We test both stock behavior (extension disabled) and extension behavior.
-- ============================================================================

-- First, test STOCK PostgreSQL behavior (matches README examples)
DROP EXTENSION pg_num2int_direct_comp;

SELECT 9007199254740993::int8 = 9007199254740993::float8 AS "a=b"  -- a=b TRUE (int8 cast to float8, rounds)
     , 9007199254740993::float8 = 9007199254740992::int8 AS "b=c"  -- b=c TRUE (int8 cast to float8, rounds)
     , 9007199254740993::int8 = 9007199254740992::int8   AS "a=c"; -- a=c FALSE!
-- Transitivity violated: a=b, b=c, but a≠c

-- Reinstall extension and test fixed behavior
CREATE EXTENSION pg_num2int_direct_comp;

SELECT 9007199254740993::int8 = 9007199254740993::float8 AS ext_a_eq_b;    -- FALSE (extension detects mismatch)
SELECT 9007199254740993::float8 = 9007199254740992::int8 AS ext_b_eq_c;    -- TRUE (float8 already 9007199254740992.0)
SELECT 9007199254740993::int8 = 9007199254740992::int8 AS ext_a_eq_c;      -- FALSE (same-type, no change)
-- Extension fixes first comparison; second is TRUE because float8 literal is not rounded

-- ============================================================================
-- README.md: Correct results with index scans and transitive equality (~line 201)
-- Single-table lookup and self-join with numeric parameter
-- ============================================================================

CREATE TEMPORARY TABLE products (
    id INT4 PRIMARY KEY,
    parent INT4,
    name TEXT
);
CREATE INDEX idx_products_parent ON products(parent);

-- Insert sample hierarchical data
INSERT INTO products VALUES
    (1, NULL, 'Electronics'),
    (2, 1, 'Phones'),
    (3, 1, 'Laptops'),
    (42, 2, 'iPhone'),
    (43, 2, 'Android'),
    (100, 3, 'MacBook');

ANALYZE products;

-- Prepared statement with numeric parameter (common API pattern)
PREPARE find_product(numeric) AS SELECT * FROM products WHERE id = $1;
PREPARE find_parent(numeric) AS
    SELECT p.name AS parent_name
    FROM products p, products c
    WHERE p.id = c.parent AND c.id = $1;

-- Test single-table lookup uses primary key index
EXPLAIN (COSTS OFF) EXECUTE find_product(42);

-- Verify result
SELECT * FROM products WHERE id = 42::numeric;

-- Test self-join uses indexes on both tables
EXPLAIN (COSTS OFF) EXECUTE find_parent(42);

-- Verify result
SELECT p.name AS parent_name
FROM products p, products c
WHERE p.id = c.parent AND c.id = 42::numeric;

-- Clean up
DEALLOCATE find_product;
DEALLOCATE find_parent;
DROP TABLE products;

-- ============================================================================
-- README.md: API passes product ID as numeric parameter (~line 263)
-- Support function transforms cross-type comparison to native integer comparison
-- ============================================================================

-- Test without extension
DROP EXTENSION pg_num2int_direct_comp;

CREATE TEMPORARY TABLE api_products(id int4 PRIMARY KEY, parent int4, name text);
-- Add more rows to ensure planner prefers index
INSERT INTO api_products SELECT g, CASE WHEN g = 1 THEN NULL ELSE 1 END, 'Product ' || g
FROM generate_series(1, 1000) g;
UPDATE api_products SET name = 'Product 42' WHERE id = 42;
ANALYZE api_products;

PREPARE api_find_product(numeric) AS SELECT * FROM api_products WHERE id = $1;

-- Without extension: sequential scan (casts indexed column)
EXPLAIN (COSTS OFF) EXECUTE api_find_product(42);

-- Test with extension
CREATE EXTENSION pg_num2int_direct_comp;

-- Re-prepare statement after extension install to use new operators
DEALLOCATE api_find_product;
PREPARE api_find_product(numeric) AS SELECT * FROM api_products WHERE id = $1;

-- With extension: index scan (transforms to integer comparison)
EXPLAIN (COSTS OFF) EXECUTE api_find_product(42);

-- Verify result
SELECT * FROM api_products WHERE id = 42::numeric;

-- Clean up
DEALLOCATE api_find_product;
DROP TABLE api_products;

-- ============================================================================
-- doc/installation.md: Detecting Float Precision Loss (~line 96)
-- The extension detects when int4 value differs from float4 representation.
-- 16777217::float4 rounds to 16777216.0, so:
-- - 16777216::int4 = 16777217::float4 → TRUE (both are 16777216)
-- - 16777217::int4 = 16777217::float4 → FALSE (16777217 ≠ 16777216.0)
-- ============================================================================

-- 16777217::float4 rounds down to 16777216.0
SELECT 16777216::int4 = 16777217::float4 AS both_are_16777216;  -- TRUE
SELECT 16777217::int4 = 16777217::float4 AS detects_mismatch;   -- FALSE (extension detects it!)

-- ============================================================================
-- doc/installation.md: Planner Transitivity Inference (~line 113)
-- ============================================================================

-- The query planner can infer transitive relationships across types for index optimization
CREATE TEMPORARY TABLE ex2_orders (id SERIAL, customer_id NUMERIC);
CREATE TEMPORARY TABLE ex2_customers (id INT4 PRIMARY KEY, name TEXT);
CREATE INDEX ON ex2_orders(customer_id);

INSERT INTO ex2_orders (customer_id) SELECT g::numeric FROM generate_series(1, 100) g;
INSERT INTO ex2_customers SELECT g, 'Customer ' || g FROM generate_series(1, 100) g;
ANALYZE ex2_orders;
ANALYZE ex2_customers;

-- Query with cross-type join: orders.customer_id (numeric) = customers.id (int4)
PREPARE find_orders(int4) AS
  SELECT o.* FROM ex2_orders o JOIN ex2_customers c ON o.customer_id = c.id WHERE c.id = $1;

-- With extension: planner infers o.customer_id = $1 across types, enabling index scan
-- The Index Cond should show (customer_id = 42), not (customer_id = (c.id)::numeric)
SET enable_seqscan = off;
EXPLAIN (COSTS OFF) EXECUTE find_orders(42);
RESET enable_seqscan;

DEALLOCATE find_orders;
DROP TABLE ex2_orders;
DROP TABLE ex2_customers;

-- ============================================================================
-- doc/installation.md: Index-Optimized Queries (~line 141)
-- ============================================================================

CREATE TEMPORARY TABLE measurements (id SERIAL, value INT4);
CREATE INDEX ON measurements(value);
INSERT INTO measurements (value) SELECT generate_series(1, 1000);
ANALYZE measurements;

-- Uses index efficiently (no cast on indexed column)
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 500::numeric;

-- ============================================================================
-- doc/installation.md: Fractional Comparisons (~line 159)
-- ============================================================================

-- Fractional values never equal integers
SELECT 10::int4 = 10.5::numeric AS should_be_false;
SELECT 10::int4 < 10.5::numeric AS should_be_true;

-- ============================================================================
-- doc/installation.md: Range Queries (~line 169)
-- ============================================================================

CREATE TEMPORARY TABLE inventory (item_id INT4, quantity INT4);
INSERT INTO inventory VALUES (1, 10), (2, 11), (3, 12);

-- Exact boundary handling: quantity > 10.5 returns quantities 11 and 12
SELECT * FROM inventory WHERE quantity > 10.5::float8;

DROP TABLE inventory;

-- ============================================================================
-- doc/installation.md: Hash Joins for Large Tables (~line 185)
-- ============================================================================

-- Hash joins work automatically for large table joins
CREATE TEMPORARY TABLE ex6_sales (id SERIAL, amount NUMERIC(10,2));
CREATE TEMPORARY TABLE ex6_targets (id SERIAL, threshold INT4);
INSERT INTO ex6_sales SELECT generate_series(1, 1000), (random() * 1000)::numeric(10,2);
INSERT INTO ex6_targets SELECT generate_series(1, 1000), (random() * 1000)::int4;
ANALYZE ex6_sales;
ANALYZE ex6_targets;

-- Planner can choose hash join for large equijoin (depends on cost estimates)
EXPLAIN (COSTS OFF) SELECT COUNT(*) FROM ex6_sales s JOIN ex6_targets t ON s.amount = t.threshold;

DROP TABLE ex6_sales;
DROP TABLE ex6_targets;

-- ============================================================================
-- doc/installation.md: Type Aliases (~line 208)
-- ============================================================================

-- Serial types work automatically
CREATE TEMPORARY TABLE ex7_users (id SERIAL, score INT4);
INSERT INTO ex7_users (score) VALUES (100);
SELECT * FROM ex7_users WHERE id = 1.0::numeric;  -- Uses exact comparison

-- Decimal type works automatically (decimal is alias for numeric)
SELECT 10::int4 = 10.0::decimal AS decimal_alias_test;

DROP TABLE ex7_users;

-- ============================================================================
-- doc/installation.md: Query Optimization - Impossible Predicate Detection (~line 222)
-- ============================================================================

-- Impossible predicate detection: integer can never equal fractional value
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 10.5::numeric;

-- Exact match transformation: uses native integer operator
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 100::numeric;

DROP TABLE measurements;

-- ============================================================================
-- doc/operator-reference.md: Impossible Predicate Detection (~line 251)
-- ============================================================================

-- 16777217::float4 rounds to 16777216.0, so comparison with 16777216 is TRUE
SELECT 16777216::bigint = 16777217::float4 AS both_are_16777216;  -- TRUE
SELECT 16777217::bigint = 16777217::float4 AS detects_mismatch;   -- FALSE (extension detects it!)

-- ============================================================================
-- doc/operator-reference.md: Exact Match Transformation (~line 270)
-- ============================================================================

CREATE TEMPORARY TABLE large_table (id INT4 PRIMARY KEY, data TEXT);
INSERT INTO large_table SELECT g, 'data' || g FROM generate_series(1, 1000) g;
ANALYZE large_table;

-- Uses index (fast)
EXPLAIN (COSTS OFF) SELECT * FROM large_table WHERE id = 12345::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM large_table WHERE id <= 100::float8;

DROP TABLE large_table;

-- ============================================================================
-- doc/operator-reference.md: Range Boundary Transformation (~line 290)
-- ============================================================================

-- Both directions work identically
SELECT 10::int4 = 100.0::numeric AS forward_eq;
SELECT 100.0::numeric = 10::int4 AS reverse_eq;

-- Works with all operators
SELECT 10::int4 < 20.5::float8 AS forward_lt;
SELECT 20.5::float8 > 10::int4 AS reverse_gt;

-- ============================================================================
-- doc/operator-reference.md: Special Cases - NULL Handling
-- ============================================================================

SELECT NULL::int4 = 10.0::numeric AS null_lhs;
SELECT 10::int4 = NULL::numeric AS null_rhs;
SELECT NULL::int4 = NULL::numeric AS both_null;

-- ============================================================================
-- doc/operator-reference.md: Special Cases - NaN and Infinity
-- ============================================================================

-- NaN comparisons with integers always return false for equality
SELECT 'NaN'::float4 = 10::int4 AS nan_eq_int;
SELECT 'NaN'::float8 = 10::int8 AS nan_eq_bigint;

-- NaN sorts higher than everything (including max integers)
SELECT 2147483647::int4 < 'NaN'::float4 AS max_int4_lt_nan;

-- Infinity comparisons
SELECT 'Infinity'::float4 > 10::int4 AS pos_inf_gt;
SELECT '-Infinity'::float8 < 10::int8 AS neg_inf_lt;

-- ============================================================================
-- doc/operator-reference.md: Special Cases - Fractional Parts
-- ============================================================================

SELECT 10.0::numeric = 10::int4 AS no_frac_eq;
SELECT 10.5::numeric = 10::int4 AS has_frac_eq;
SELECT 10.001::numeric = 10::int4 AS small_frac_eq;

-- ============================================================================
-- doc/operator-reference.md: Query Optimization - Impossible Predicate Detection (FR-015)
-- ============================================================================

CREATE TEMPORARY TABLE fr015_test (int_col INT4);
INSERT INTO fr015_test SELECT generate_series(1, 100);
ANALYZE fr015_test;

-- int_col can never equal 10.5 (fractional value)
EXPLAIN (COSTS OFF) SELECT * FROM fr015_test WHERE int_col = 10.5::numeric;

-- ============================================================================
-- doc/operator-reference.md: Query Optimization - Exact Match Transformation (FR-016)
-- ============================================================================

CREATE INDEX ON fr015_test(int_col);
ANALYZE fr015_test;

-- 100::numeric exactly equals 100::int4
EXPLAIN (COSTS OFF) SELECT * FROM fr015_test WHERE int_col = 100::numeric;

DROP TABLE fr015_test;

-- ============================================================================
-- doc/operator-reference.md: HASHES property example (~line 130)
-- ============================================================================

CREATE TEMPORARY TABLE hash_a (int_col INT4);
CREATE TEMPORARY TABLE hash_b (numeric_col NUMERIC);
INSERT INTO hash_a SELECT generate_series(1, 1000);
INSERT INTO hash_b SELECT generate_series(1, 1000)::numeric;
ANALYZE hash_a;
ANALYZE hash_b;

-- Can use Hash Join strategy
EXPLAIN (COSTS OFF) SELECT * FROM hash_a a JOIN hash_b b ON a.int_col = b.numeric_col WHERE a.int_col < 100;

DROP TABLE hash_a;
DROP TABLE hash_b;

-- ============================================================================
-- doc/operator-reference.md: Support function example (~line 163)
-- ============================================================================

CREATE TEMPORARY TABLE support_test (intkey1 INT4, intkey2 INT4);
CREATE INDEX ON support_test(intkey1, intkey2);
INSERT INTO support_test SELECT i, i FROM generate_series(1, 1000) i;
ANALYZE support_test;

-- Query with constants - num2int_support transforms predicates
EXPLAIN (COSTS OFF) SELECT * FROM support_test
    WHERE intkey1 = 100.0::numeric AND intkey2 > 10.5::numeric;

DROP TABLE support_test;

-- ============================================================================
-- doc/operator-reference.md: Precision Boundaries - float4 (~line 195)
-- ============================================================================

SELECT 16777216::int4 = 16777216::float4 AS within_range;
SELECT 16777217::int4 = 16777217::float4 AS beyond_range;

-- ============================================================================
-- doc/operator-reference.md: Precision Boundaries - float8 (~line 205)
-- ============================================================================

SELECT 9007199254740992::int8 = 9007199254740992::float8 AS within_range;
SELECT 9007199254740993::int8 = 9007199254740993::float8 AS beyond_range;

-- ============================================================================
-- doc/installation.md: Verify Installation (~line 68)
-- ============================================================================

-- Check extension is loaded
\dx pg_num2int_direct_comp

-- Test precision loss detection (stock PostgreSQL returns true for both)
SELECT 16777216::int4 = 16777217::float4;  -- true (float4 rounds to 16777216)
SELECT 16777217::int4 = 16777217::float4;  -- false (extension detects mismatch!)

-- ============================================================================
-- README.md: Configuration Examples (~line 540)
-- ============================================================================

-- Check current setting
SHOW pg_num2int_direct_comp.enable_support_functions;

-- Create test table for optimization verification
CREATE TEMPORARY TABLE config_test (integer_column int4);
INSERT INTO config_test VALUES (10);

-- Test with optimization disabled
SET pg_num2int_direct_comp.enable_support_functions = off;
EXPLAIN (COSTS off) SELECT * FROM config_test WHERE 10.0::float8 = integer_column;

-- Test with optimization enabled
SET pg_num2int_direct_comp.enable_support_functions = on;
EXPLAIN (COSTS off) SELECT * FROM config_test WHERE 10.0::float8 = integer_column;

-- Cleanup
DROP TABLE config_test;

-- ============================================================================
-- README.md: Equivalence Expressions (~line 390)
-- Examples from the equivalence table showing how to switch between extension
-- and stock PostgreSQL behavior
-- ============================================================================

-- Create test table with columns for equivalence testing
CREATE TEMPORARY TABLE equiv_test (
    int4_col int4,
    int8_col int8,
    float4_col float4,
    float8_col float8,
    numeric_col numeric
);

INSERT INTO equiv_test VALUES (42, 9223372036854775807, 3.14::float4, 2.718::float8, 123.456::numeric);

-- Extension operators vs. explicit casts (for demonstration)
-- int4 = float4: Extension vs. stock PostgreSQL behavior
SELECT int4_col = float4_col AS with_extension,
       int4_col::float8 = float4_col::float8 AS stock_postgresql
FROM equiv_test;

-- int8 = float8: Extension vs. stock PostgreSQL behavior
SELECT int8_col = float8_col AS with_extension,
       int8_col::float8 = float8_col AS stock_postgresql_same
FROM equiv_test;

-- int4 = numeric: Extension vs. numeric cast approach
SELECT int4_col = numeric_col AS with_extension,
       int4_col::numeric = numeric_col AS numeric_cast_approach
FROM equiv_test;

DROP TABLE equiv_test;

-- ============================================================================
-- README.md: Transitive Equality Violations detected at the float8 2^53 boundary (different at the last digit) (~line 132)
-- ============================================================================

SELECT 9007199254740993::int8 = 9007199254740993::float8 AS "a=b",  -- a=b FALSE (b lossy NOT EQUAL)
       9007199254740993::float8 = 9007199254740992::int8 AS "b=c",  -- b=c TRUE (int8 cast to rounded float8)
       9007199254740993::int8 = 9007199254740992::int8   AS "a=c";  -- a=c FALSE!
