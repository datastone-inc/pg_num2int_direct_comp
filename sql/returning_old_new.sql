-- RETURNING OLD/NEW references (PostgreSQL 18 and later; the Makefile runs
-- this test only there). Split from null_handling so that test's output
-- does not depend on the server version.
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;

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
