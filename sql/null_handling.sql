-- Test: NULL Handling for Direct Numeric-Integer Comparison
-- Purpose: Verify NULL propagation follows SQL standard for all 72 operators

-- NULL propagation for equality operators (=)
SELECT 'NULL::numeric = 100::int4'::text AS test, NULL::numeric = 100::int4 AS result;
SELECT '100::numeric = NULL::int4'::text AS test, 100::numeric = NULL::int4 AS result;
SELECT 'NULL::float4 = 100::int4'::text AS test, NULL::float4 = 100::int4 AS result;
SELECT '100::float4 = NULL::int4'::text AS test, 100::float4 = NULL::int4 AS result;
SELECT 'NULL::float8 = 100::int4'::text AS test, NULL::float8 = 100::int4 AS result;
SELECT '100::float8 = NULL::int4'::text AS test, 100::float8 = NULL::int4 AS result;

-- NULL propagation for inequality operators (<>)
SELECT 'NULL::numeric <> 100::int4'::text AS test, NULL::numeric <> 100::int4 AS result;
SELECT '100::numeric <> NULL::int4'::text AS test, 100::numeric <> NULL::int4 AS result;
SELECT 'NULL::float4 <> 100::int4'::text AS test, NULL::float4 <> 100::int4 AS result;
SELECT '100::float4 <> NULL::int4'::text AS test, 100::float4 <> NULL::int4 AS result;
SELECT 'NULL::float8 <> 100::int4'::text AS test, NULL::float8 <> 100::int4 AS result;
SELECT '100::float8 <> NULL::int4'::text AS test, 100::float8 <> NULL::int4 AS result;

-- NULL propagation for less-than operators (<)
SELECT 'NULL::numeric < 100::int4'::text AS test, NULL::numeric < 100::int4 AS result;
SELECT '100::numeric < NULL::int4'::text AS test, 100::numeric < NULL::int4 AS result;
SELECT 'NULL::float4 < 100::int4'::text AS test, NULL::float4 < 100::int4 AS result;
SELECT '100::float4 < NULL::int4'::text AS test, 100::float4 < NULL::int4 AS result;
SELECT 'NULL::float8 < 100::int4'::text AS test, NULL::float8 < 100::int4 AS result;
SELECT '100::float8 < NULL::int4'::text AS test, 100::float8 < NULL::int4 AS result;

-- NULL propagation for less-than-or-equal operators (<=)
SELECT 'NULL::numeric <= 100::int4'::text AS test, NULL::numeric <= 100::int4 AS result;
SELECT '100::numeric <= NULL::int4'::text AS test, 100::numeric <= NULL::int4 AS result;
SELECT 'NULL::float4 <= 100::int4'::text AS test, NULL::float4 <= 100::int4 AS result;
SELECT '100::float4 <= NULL::int4'::text AS test, 100::float4 <= NULL::int4 AS result;
SELECT 'NULL::float8 <= 100::int4'::text AS test, NULL::float8 <= 100::int4 AS result;
SELECT '100::float8 <= NULL::int4'::text AS test, 100::float8 <= NULL::int4 AS result;

-- NULL propagation for greater-than operators (>)
SELECT 'NULL::numeric > 100::int4'::text AS test, NULL::numeric > 100::int4 AS result;
SELECT '100::numeric > NULL::int4'::text AS test, 100::numeric > NULL::int4 AS result;
SELECT 'NULL::float4 > 100::int4'::text AS test, NULL::float4 > 100::int4 AS result;
SELECT '100::float4 > NULL::int4'::text AS test, 100::float4 > NULL::int4 AS result;
SELECT 'NULL::float8 > 100::int4'::text AS test, NULL::float8 > 100::int4 AS result;
SELECT '100::float8 > NULL::int4'::text AS test, 100::float8 > NULL::int4 AS result;

-- NULL propagation for greater-than-or-equal operators (>=)
SELECT 'NULL::numeric >= 100::int4'::text AS test, NULL::numeric >= 100::int4 AS result;
SELECT '100::numeric >= NULL::int4'::text AS test, 100::numeric >= NULL::int4 AS result;
SELECT 'NULL::float4 >= 100::int4'::text AS test, NULL::float4 >= 100::int4 AS result;
SELECT '100::float4 >= NULL::int4'::text AS test, 100::float4 >= NULL::int4 AS result;
SELECT 'NULL::float8 >= 100::int4'::text AS test, NULL::float8 >= 100::int4 AS result;
SELECT '100::float8 >= NULL::int4'::text AS test, 100::float8 >= NULL::int4 AS result;

-- NULL propagation with int2 and int8
SELECT 'NULL::numeric = 100::int2'::text AS test, NULL::numeric = 100::int2 AS result;
SELECT 'NULL::numeric = 100::int8'::text AS test, NULL::numeric = 100::int8 AS result;
SELECT '100::int2 = NULL::numeric'::text AS test, 100::int2 = NULL::numeric AS result;
SELECT '100::int8 = NULL::numeric'::text AS test, 100::int8 = NULL::numeric AS result;

-- NULL in table queries
CREATE TEMPORARY TABLE t_null (id INT4, val_num NUMERIC, val_f4 FLOAT4, val_f8 FLOAT8);
INSERT INTO t_null VALUES (1, 100, 100, 100), (2, NULL, NULL, NULL), (3, 200, 200, 200);

SELECT 'NULL in WHERE clause (numeric)'::text AS test, COUNT(*) AS count FROM t_null WHERE val_num = 100::int4;
SELECT 'NULL in WHERE clause (float4)'::text AS test, COUNT(*) AS count FROM t_null WHERE val_f4 = 100::int4;
SELECT 'NULL in WHERE clause (float8)'::text AS test, COUNT(*) AS count FROM t_null WHERE val_f8 = 100::int4;

-- NULL should not match in inequality
SELECT 'NULL <> in WHERE clause'::text AS test, COUNT(*) AS count FROM t_null WHERE val_num <> 100::int4;

-- IS NULL and IS NOT NULL should work normally
SELECT 'IS NULL check'::text AS test, COUNT(*) AS count FROM t_null WHERE val_num IS NULL;
SELECT 'IS NOT NULL check'::text AS test, COUNT(*) AS count FROM t_null WHERE val_num IS NOT NULL;

-- ============================================================================
-- NULL preservation under SupportRequestSimplify in non-WHERE expression
-- contexts. The support function rewrites cross-type comparisons during
-- planning and substitutes the new node in every context (SELECT list,
-- NOT, CASE, COALESCE), not just WHERE. On a nullable column, collapsing
-- to a bare boolean constant would change NULL rows from NULL to
-- TRUE/FALSE. These tests assert that NULL semantics survive each
-- rewrite class: impossible-fractional, out-of-range, range-boundary
-- always-true/false.
-- ============================================================================
CREATE TEMPORARY TABLE t_rewrite_ctx (
  nic INT4,           -- nullable
  nin INT4 NOT NULL   -- non-nullable
);
INSERT INTO t_rewrite_ctx VALUES (10, 10), (NULL, 20), (200, 200);

-- Impossible-fractional EQ on nullable column in SELECT list:
-- NULL row must produce NULL (was FALSE pre-fix).
SELECT nic, nic = 10.5::numeric AS frac_eq FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- Impossible-fractional NE on nullable column wrapped in NOT:
-- NULL row must produce NULL (was FALSE pre-fix since NOT TRUE = FALSE).
SELECT nic, NOT (nic <> 10.5::numeric) AS not_frac_ne FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- Out-of-range constant in SELECT list on nullable column:
-- NULL row must produce NULL for every operator (was constant pre-fix).
SELECT nic,
       nic = 99999999999::numeric  AS oor_eq,
       nic <> 99999999999::numeric AS oor_ne,
       nic <  99999999999::numeric AS oor_lt,
       nic >  99999999999::numeric AS oor_gt
  FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- Out-of-range below int4 min on nullable column:
SELECT nic,
       nic = (-99999999999)::numeric  AS oor_low_eq,
       nic >= (-99999999999)::numeric AS oor_low_ge
  FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- Range-boundary always-true on nullable column:
-- nic <= INT4_MAX (collapses to always-TRUE in current code); NULL must stay NULL.
SELECT nic, nic <= 2147483647.5::numeric AS bound_always_true
  FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- CASE expression on nullable column with both EQ and NE arms:
-- NULL row must reach ELSE branch (both arms NULL), not the NE arm.
SELECT nic,
       CASE WHEN nic = 10.5::numeric THEN 'eq'
            WHEN nic <> 10.5::numeric THEN 'ne'
            ELSE 'null' END AS arm
  FROM t_rewrite_ctx ORDER BY nic NULLS LAST;

-- NOT NULL column: bare-bool collapse still applies (plan optimization preserved).
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nin = 10.5::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nin <> 10.5::numeric;

-- Nullable column with same predicates: cross-type op stays in plan (no bare-bool collapse).
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nic = 10.5::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nic <> 10.5::numeric;

-- Out-of-range on nullable column: synthesized same-side bound on the column.
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nic = 99999999999::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM t_rewrite_ctx WHERE nic >  99999999999::numeric;

-- WHERE-clause result-set regression: WHERE NOT (nic = 10.5) must filter out
-- NULL rows (pre-fix it matched them because bare FALSE inverted to TRUE).
SELECT 'WHERE NOT (frac eq) count'::text AS test,
       COUNT(*) AS count
  FROM t_rewrite_ctx WHERE NOT (nic = 10.5::numeric);

DROP TABLE t_rewrite_ctx;

-- ============================================================================
-- Outer-join nullability: a Var referencing a NOT NULL column can still be
-- NULL at runtime when the row is the unmatched side of an outer join. The
-- support function must consult Var.varnullingrels (PG 16+), not just
-- pg_attribute.attnotnull, before collapsing a cross-type predicate to a
-- bare boolean constant. Without this check, the no-match row in the
-- LEFT JOIN below returns FALSE/TRUE instead of NULL.
-- ============================================================================
CREATE TEMPORARY TABLE oj_left (id INT, k INT NOT NULL);
CREATE TEMPORARY TABLE oj_right (id INT);
INSERT INTO oj_right VALUES (1);
INSERT INTO oj_left VALUES (2, 5);  -- no row for id=1

-- Each of these must yield NULL for the id=1 row (no match in oj_left).
SELECT r.id,
       l.k =  10.5::numeric         AS frac_eq,
       l.k <> 10.5::numeric         AS frac_ne,
       l.k <= 2147483647.5::numeric AS bound_always_true,
       l.k =  99999999999::numeric  AS oor_eq
  FROM oj_right r LEFT JOIN oj_left l USING (id)
 ORDER BY r.id;

DROP TABLE oj_left;
DROP TABLE oj_right;

-- ============================================================================
-- Index-eligibility of the synthesized out-of-range bound on a nullable
-- column. The docs claim the rewrite remains index-eligible via the
-- integer_ops btree family; prove it by forcing the planner to choose an
-- index path. enable_seqscan=off is set locally; the synthesized
-- predicate (col > typeMax / col < typeMin) must show up as an Index Cond.
-- ============================================================================
CREATE TEMPORARY TABLE idx_nullable (k INT4);
CREATE INDEX ON idx_nullable (k);
INSERT INTO idx_nullable SELECT g FROM generate_series(1, 1000) g;
INSERT INTO idx_nullable VALUES (NULL), (NULL);
ANALYZE idx_nullable;

BEGIN;
SET LOCAL enable_seqscan = off;
SET LOCAL enable_bitmapscan = off;
EXPLAIN (COSTS OFF) SELECT * FROM idx_nullable WHERE k =  99999999999::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM idx_nullable WHERE k <  (-99999999999)::numeric;
COMMIT;

DROP TABLE idx_nullable;

-- ============================================================================
-- Correlated subquery: outer Var seen with varlevelsup > 0. isVarNotNull
-- declines the rewrite when varlevelsup != 0 even if the column is NOT
-- NULL, because the planner state for the inner level can't be safely
-- consulted. Pins that guard so a future refactor can't silently regress
-- it. Functional result: FALSE on every non-NULL outer row.
-- ============================================================================
CREATE TEMPORARY TABLE outer_t (k INT NOT NULL);
INSERT INTO outer_t VALUES (1), (2);
SELECT k,
       (SELECT k = 10.5::numeric) AS frac_eq_corr,
       (SELECT k <> 10.5::numeric) AS frac_ne_corr
  FROM outer_t ORDER BY k;
DROP TABLE outer_t;

-- ============================================================================
-- View DDL: the support function substitutes the rewritten node into the
-- planner tree of every query that references the view, so the same
-- NULL-preservation rules must hold when the predicate lives in a view's
-- SELECT list. Nullable column view: NULL row must produce NULL.
-- NOT NULL column view: FALSE for every row.
-- ============================================================================
CREATE TEMPORARY TABLE vt (nic INT, nin INT NOT NULL);
INSERT INTO vt VALUES (10, 10), (NULL, 20);
CREATE TEMPORARY VIEW v_nic AS SELECT nic, nic = 10.5::numeric AS frac FROM vt;
CREATE TEMPORARY VIEW v_nin AS SELECT nin, nin = 10.5::numeric AS frac FROM vt;
SELECT * FROM v_nic ORDER BY nic NULLS LAST;
SELECT * FROM v_nin ORDER BY nin;
DROP VIEW v_nic;
DROP VIEW v_nin;
DROP TABLE vt;

-- ============================================================================
-- RETURNING OLD/NEW: a Var with varreturningtype != VAR_RETURNING_DEFAULT
-- can be NULL at runtime (OLD on INSERT, NEW on DELETE) even when the
-- underlying column is NOT NULL. isVarNotNull declines the rewrite in
-- that case; without the guard the predicate would collapse to a bare
-- boolean and lose NULL semantics in the RETURNING list.
-- ============================================================================
CREATE TEMPORARY TABLE rt (k INT NOT NULL);
-- INSERT: OLD is NULL for every column → frac_on_old must be NULL.
INSERT INTO rt VALUES (1)
  RETURNING OLD.k AS old_k,
            OLD.k =  10.5::numeric AS frac_on_old,
            OLD.k <> 10.5::numeric AS frac_ne_on_old,
            NEW.k =  10.5::numeric AS frac_on_new;
-- DELETE: NEW is NULL for every column → frac_on_new must be NULL.
DELETE FROM rt
  RETURNING OLD.k AS old_k,
            OLD.k =  10.5::numeric AS frac_on_old,
            NEW.k =  10.5::numeric AS frac_on_new,
            NEW.k <> 10.5::numeric AS frac_ne_on_new;
DROP TABLE rt;
