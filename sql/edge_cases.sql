-- Test: Edge Cases for Direct Numeric-Integer Comparison
-- Purpose: Verify precision boundaries, overflow, and type alias handling

-- Precision Boundary Tests for float4 (mantissa can exactly represent up to 2^24)
-- float4 can exactly represent integers up to 16,777,216
SELECT '16777216::float4 = 16777216::int8'::text AS test, 
       16777216::float4 = 16777216::int8 AS result;
SELECT '16777217::float4 = 16777217::int8'::text AS test,
       16777217::float4 = 16777217::int8 AS result;  -- Should be false (precision loss)

-- Precision Boundary Tests for float8 (mantissa can exactly represent up to 2^53)
-- float8 can exactly represent integers up to 9,007,199,254,740,992
SELECT '9007199254740992::float8 = 9007199254740992::int8'::text AS test,
       9007199254740992::float8 = 9007199254740992::int8 AS result;
SELECT '9007199254740993::float8 = 9007199254740993::int8'::text AS test,
       9007199254740993::float8 = 9007199254740993::int8 AS result;  -- Should be false (precision loss)

-- Overflow Tests: numeric/float values outside integer type ranges
SELECT '2147483648::numeric = 2147483647::int4'::text AS test,
       2147483648::numeric = 2147483647::int4 AS result;  -- numeric > int4_max
SELECT '2147483648::numeric > 2147483647::int4'::text AS test,
       2147483648::numeric > 2147483647::int4 AS result;  -- Should be true
SELECT '-2147483649::numeric = -2147483648::int4'::text AS test,
       (-2147483649.0)::numeric = (-2147483648)::int4 AS result;  -- numeric < int4_min
SELECT '-2147483649::numeric < -2147483648::int4'::text AS test,
       (-2147483649.0)::numeric < (-2147483648)::int4 AS result;  -- Should be true

-- Serial Type Alias Tests (smallserial = int2, serial = int4, bigserial = int8)
CREATE TEMPORARY TABLE t_serial (
  id_small SMALLSERIAL,
  id_normal SERIAL,
  id_big BIGSERIAL
);
INSERT INTO t_serial DEFAULT VALUES;
INSERT INTO t_serial DEFAULT VALUES;

SELECT 'smallserial = numeric'::text AS test, id_small = 1::numeric AS result FROM t_serial WHERE id_small = 1;
SELECT 'serial = numeric'::text AS test, id_normal = 2::numeric AS result FROM t_serial WHERE id_normal = 2;
SELECT 'bigserial = numeric'::text AS test, id_big = 1::numeric AS result FROM t_serial WHERE id_big = 1;

-- Decimal Type Alias Test (decimal = numeric)
CREATE TEMPORARY TABLE t_decimal (val DECIMAL(10,2));
INSERT INTO t_decimal VALUES (100.00), (200.50);

SELECT 'decimal = int4 (exact)'::text AS test, val = 100::int4 AS result FROM t_decimal WHERE val = 100::int4;
SELECT 'decimal = int4 (fractional)'::text AS test, val = 200::int4 AS result FROM t_decimal WHERE val = 200::int4;  -- Should be false

-- Edge Case: Zero in different representations
SELECT '0::numeric = 0::int4'::text AS test, 0::numeric = 0::int4 AS result;
SELECT '0.0::numeric = 0::int4'::text AS test, 0.0::numeric = 0::int4 AS result;
SELECT '-0::numeric = 0::int4'::text AS test, (-0)::numeric = 0::int4 AS result;
SELECT '0::float4 = 0::int4'::text AS test, 0::float4 = 0::int4 AS result;
SELECT '0::float8 = 0::int4'::text AS test, 0::float8 = 0::int4 AS result;

-- Edge Case: Negative numbers with fractional parts
SELECT '-100.5::numeric = -100::int4'::text AS test, (-100.5)::numeric = (-100)::int4 AS result;  -- Should be false
SELECT '-100.5::numeric <> -100::int4'::text AS test, (-100.5)::numeric <> (-100)::int4 AS result;  -- Should be true
SELECT '-100.5::numeric < -100::int4'::text AS test, (-100.5)::numeric < (-100)::int4 AS result;  -- Should be true
SELECT '-100.5::numeric > -101::int4'::text AS test, (-100.5)::numeric > (-101)::int4 AS result;  -- Should be true

-- Edge Case: Very small fractional parts
SELECT '100.0000000001::numeric = 100::int4'::text AS test, 100.0000000001::numeric = 100::int4 AS result;  -- Should be false
SELECT '100.0000000001::numeric > 100::int4'::text AS test, 100.0000000001::numeric > 100::int4 AS result;  -- Should be true
SELECT '99.9999999999::numeric < 100::int4'::text AS test, 99.9999999999::numeric < 100::int4 AS result;  -- Should be true

-- Edge Case: Maximum values for integer types
SELECT '32767::numeric = 32767::int2'::text AS test, 32767::numeric = 32767::int2 AS result;  -- int2 max
SELECT '2147483647::numeric = 2147483647::int4'::text AS test, 2147483647::numeric = 2147483647::int4 AS result;  -- int4 max
SELECT '9223372036854775807::numeric = 9223372036854775807::int8'::text AS test, 9223372036854775807::numeric = 9223372036854775807::int8 AS result;  -- int8 max

-- Edge Case: Minimum values for integer types
SELECT '-32768::numeric = -32768::int2'::text AS test, (-32768)::numeric = (-32768)::int2 AS result;  -- int2 min
SELECT '-2147483648::numeric = -2147483648::int4'::text AS test, (-2147483648)::numeric = (-2147483648)::int4 AS result;  -- int4 min
SELECT '-9223372036854775808::numeric = -9223372036854775808::int8'::text AS test, (-9223372036854775808)::numeric = (-9223372036854775808)::int8 AS result;  -- int8 min
