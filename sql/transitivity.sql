/*
 * Transitivity tests: verify PostgreSQL does NOT apply transitive inference
 * across cross-type comparisons
 * 
 * Background: In standard same-type comparisons, if A = B and B = C, then A = C.
 * The query optimizer may use this to simplify queries. However, for cross-type
 * comparisons with different precision, this can be unsafe.
 * 
 * Our operators deliberately DO NOT belong to any btree operator family to prevent
 * the optimizer from making transitive inferences that could yield incorrect results.
 */

-- Setup test tables
CREATE TEMP TABLE t_numeric (id int4, val_numeric numeric);
CREATE TEMP TABLE t_integer (id int4, val_int int4);
CREATE TEMP TABLE t_float (id int4, val_float float8);

INSERT INTO t_numeric VALUES (1, 10.5), (2, 10.0), (3, 11.0);
INSERT INTO t_integer VALUES (1, 10), (2, 11), (3, 12);
INSERT INTO t_float VALUES (1, 10.0), (2, 10.5), (3, 11.0);

-- Test 1: Chained equality comparisons
-- Query: numeric_col = int_col AND int_col = float_col
-- Expected: All three conditions should appear in the plan (no transitivity)
SELECT n.id, n.val_numeric, i.val_int, f.val_float
FROM t_numeric n, t_integer i, t_float f
WHERE n.val_numeric = i.val_int
  AND i.val_int = f.val_float
  AND n.id = 1 AND i.id = 1 AND f.id = 1;

-- Test 2: EXPLAIN to verify no transitive predicate inference
-- Should show both n.val_numeric = i.val_int AND i.val_int = f.val_float
-- Should NOT show inferred n.val_numeric = f.val_float
EXPLAIN (COSTS OFF) 
SELECT n.id, n.val_numeric, i.val_int, f.val_float
FROM t_numeric n, t_integer i, t_float f
WHERE n.val_numeric = i.val_int
  AND i.val_int = f.val_float
  AND n.id = 1 AND i.id = 1 AND f.id = 1;

-- Test 3: Inequality chain with range operators
-- Query: numeric_col < int_col AND int_col < float_col
-- Expected: Both conditions in plan, no transitive inference
SELECT n.id, n.val_numeric, i.val_int, f.val_float
FROM t_numeric n, t_integer i, t_float f
WHERE n.val_numeric < i.val_int
  AND i.val_int < f.val_float
  AND n.id = 1 AND i.id = 1 AND f.id = 1;

-- Test 4: EXPLAIN for range operator chain
EXPLAIN (COSTS OFF)
SELECT n.id, n.val_numeric, i.val_int, f.val_float
FROM t_numeric n, t_integer i, t_float f
WHERE n.val_numeric < i.val_int
  AND i.val_int < f.val_float
  AND n.id = 1 AND i.id = 1 AND f.id = 1;

-- Test 5: Mixed operators (transitivity should not apply across different operators)
-- Query: numeric_col >= int_col AND int_col > float_col
SELECT n.id, n.val_numeric, i.val_int, f.val_float
FROM t_numeric n, t_integer i, t_float f
WHERE n.val_numeric >= i.val_int
  AND i.val_int > f.val_float
  AND n.id = 2 AND i.id = 1 AND f.id = 1;

-- Test 6: Verify our operators are NOT in btree operator families
-- This query checks pg_amop to see if our operators are registered in any btree opfamily
-- Expected: No rows returned (our operators should NOT be in btree families)
SELECT o.oprname, opf.opfname
FROM pg_operator o
  LEFT JOIN pg_amop amop ON o.oid = amop.amopopr
  LEFT JOIN pg_opfamily opf ON amop.amopfamily = opf.oid
  LEFT JOIN pg_am am ON opf.opfmethod = am.oid AND am.amname = 'btree'
WHERE o.oprname IN ('=', '<>', '<', '>', '<=', '>=')
  AND (o.oprleft = 'numeric'::regtype AND o.oprright IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
       OR o.oprleft IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND o.oprright = 'numeric'::regtype
       OR o.oprleft = 'float4'::regtype AND o.oprright IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
       OR o.oprleft IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND o.oprright = 'float4'::regtype
       OR o.oprleft = 'float8'::regtype AND o.oprright IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype)
       OR o.oprleft IN ('int2'::regtype, 'int4'::regtype, 'int8'::regtype) AND o.oprright = 'float8'::regtype)
  AND am.amname = 'btree'
ORDER BY o.oprname, opf.opfname;

-- Test 7: Demonstrate why transitivity would be unsafe
-- If 10.5::numeric = 10::int (false) and 10::int = 10.0::float8 (true)
-- Transitivity would incorrectly infer 10.5::numeric = 10.0::float8 (false)
SELECT 
  10.5::numeric = 10::int4 AS "10.5=10 (should be false)",
  10::int4 = 10.0::float8 AS "10=10.0 (true)",
  10.5::numeric = 10.0::float8 AS "10.5=10.0 (should be false - transitive would be wrong)";

-- Test 8: Another transitivity safety example with range operators
-- If 10.5::numeric > 10::int (true) and 10::int >= 10.0::float8 (true)
-- Does NOT imply 10.5::numeric > 10.0::float8 (actually true, but can't be inferred safely)
SELECT
  10.5::numeric > 10::int4 AS "10.5>10 (true)",
  10::int4 >= 10.0::float8 AS "10>=10.0 (true)",
  10.5::numeric > 10.0::float8 AS "10.5>10.0 (happens to be true, but not inferrable)";

DROP TABLE t_numeric;
DROP TABLE t_integer;
DROP TABLE t_float;
