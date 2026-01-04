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
