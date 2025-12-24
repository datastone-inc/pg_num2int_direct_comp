-- Test: Special Values (NaN, Infinity) for Direct Float-Integer Comparison
-- Purpose: Verify IEEE 754 special value handling for float4 and float8 operators

-- NaN Tests: NaN should not equal anything (including itself in comparisons)
SELECT '''NaN''::float4 = 100::int4'::text AS test, 'NaN'::float4 = 100::int4 AS result;
SELECT '100::float4 = ''NaN''::int4 (should error)'::text AS test, 'NaN check skipped' AS result;  -- int4 cannot hold NaN
SELECT '''NaN''::float8 = 100::int4'::text AS test, 'NaN'::float8 = 100::int4 AS result;
SELECT '''NaN''::float4 <> 100::int4'::text AS test, 'NaN'::float4 <> 100::int4 AS result;
SELECT '''NaN''::float8 <> 100::int4'::text AS test, 'NaN'::float8 <> 100::int4 AS result;

-- NaN comparison with range operators (NaN comparisons always return false per IEEE 754)
SELECT '''NaN''::float4 < 100::int4'::text AS test, 'NaN'::float4 < 100::int4 AS result;
SELECT '''NaN''::float4 > 100::int4'::text AS test, 'NaN'::float4 > 100::int4 AS result;
SELECT '''NaN''::float4 <= 100::int4'::text AS test, 'NaN'::float4 <= 100::int4 AS result;
SELECT '''NaN''::float4 >= 100::int4'::text AS test, 'NaN'::float4 >= 100::int4 AS result;
SELECT '''NaN''::float8 < 100::int8'::text AS test, 'NaN'::float8 < 100::int8 AS result;
SELECT '''NaN''::float8 > 100::int8'::text AS test, 'NaN'::float8 > 100::int8 AS result;

-- Infinity Tests: +Infinity
SELECT '''Infinity''::float4 = 100::int4'::text AS test, 'Infinity'::float4 = 100::int4 AS result;  -- Should be false
SELECT '''Infinity''::float4 > 100::int4'::text AS test, 'Infinity'::float4 > 100::int4 AS result;  -- Should be true
SELECT '''Infinity''::float4 < 100::int4'::text AS test, 'Infinity'::float4 < 100::int4 AS result;  -- Should be false
SELECT '''Infinity''::float8 = 100::int8'::text AS test, 'Infinity'::float8 = 100::int8 AS result;  -- Should be false
SELECT '''Infinity''::float8 > 100::int8'::text AS test, 'Infinity'::float8 > 100::int8 AS result;  -- Should be true
SELECT '''Infinity''::float8 >= 100::int8'::text AS test, 'Infinity'::float8 >= 100::int8 AS result;  -- Should be true

-- Infinity Tests: -Infinity
SELECT '''-Infinity''::float4 = 100::int4'::text AS test, '-Infinity'::float4 = 100::int4 AS result;  -- Should be false
SELECT '''-Infinity''::float4 < 100::int4'::text AS test, '-Infinity'::float4 < 100::int4 AS result;  -- Should be true
SELECT '''-Infinity''::float4 > 100::int4'::text AS test, '-Infinity'::float4 > 100::int4 AS result;  -- Should be false
SELECT '''-Infinity''::float8 = 100::int8'::text AS test, '-Infinity'::float8 = 100::int8 AS result;  -- Should be false
SELECT '''-Infinity''::float8 < 100::int8'::text AS test, '-Infinity'::float8 < 100::int8 AS result;  -- Should be true
SELECT '''-Infinity''::float8 <= 100::int8'::text AS test, '-Infinity'::float8 <= 100::int8 AS result;  -- Should be true

-- Edge Case: Infinity compared to max integer values
SELECT '''Infinity''::float4 > 2147483647::int4'::text AS test, 'Infinity'::float4 > 2147483647::int4 AS result;  -- Should be true
SELECT '''Infinity''::float8 > 9223372036854775807::int8'::text AS test, 'Infinity'::float8 > 9223372036854775807::int8 AS result;  -- Should be true
SELECT '''-Infinity''::float4 < -2147483648::int4'::text AS test, '-Infinity'::float4 < (-2147483648)::int4 AS result;  -- Should be true
SELECT '''-Infinity''::float8 < -9223372036854775808::int8'::text AS test, '-Infinity'::float8 < (-9223372036854775808)::int8 AS result;  -- Should be true

-- Special Values in tables
CREATE TEMPORARY TABLE t_special (id INT4, val_f4 FLOAT4, val_f8 FLOAT8);
INSERT INTO t_special VALUES (1, 'NaN', 'NaN'), (2, 'Infinity', 'Infinity'), (3, '-Infinity', '-Infinity'), (4, 100, 100);

SELECT 'NaN in WHERE (should return 0)'::text AS test, COUNT(*) AS count FROM t_special WHERE val_f4 = 100::int4;
SELECT 'Infinity > int4 (should return 1)'::text AS test, COUNT(*) AS count FROM t_special WHERE val_f4 > 2147483647::int4;
SELECT '-Infinity < int4 (should return 1)'::text AS test, COUNT(*) AS count FROM t_special WHERE val_f4 < (-2147483648)::int4;
SELECT 'Normal value = int4 (should return 1)'::text AS test, COUNT(*) AS count FROM t_special WHERE val_f4 = 100::int4;

-- NaN vs Infinity
SELECT '''NaN''::float4 = ''Infinity''::float4'::text AS test, 'NaN'::float4 = 'Infinity'::float4 AS result;  -- Should be false
SELECT '''NaN''::float4 > ''Infinity''::float4'::text AS test, 'NaN'::float4 > 'Infinity'::float4 AS result;  -- Should be false (NaN comparisons are always false)
SELECT '''NaN''::float4 < ''-Infinity''::float4'::text AS test, 'NaN'::float4 < '-Infinity'::float4 AS result;  -- Should be false
