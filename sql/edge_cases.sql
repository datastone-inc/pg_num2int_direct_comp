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

-- Edge Case: Numerics with implicit zero digits (weight > ndigits - 1)
-- In base-10000, 100000000 has weight=2 but only one stored digit [1]
-- The middle and trailing digit positions are implicitly zero
SELECT '100000000::numeric = 100000000::int8'::text AS test,
       100000000::numeric = 100000000::int8 AS result;  -- Should be true
SELECT '100000000.0::numeric = 100000000::int8'::text AS test,
       100000000.0::numeric = 100000000::int8 AS result;  -- Should be true (trailing .0)
SELECT '100000000.5::numeric = 100000000::int8'::text AS test,
       100000000.5::numeric = 100000000::int8 AS result;  -- Should be false (has fraction)
SELECT '100000000.5::numeric > 100000000::int8'::text AS test,
       100000000.5::numeric > 100000000::int8 AS result;  -- Should be true
SELECT '10000::numeric = 10000::int4'::text AS test,
       10000::numeric = 10000::int4 AS result;  -- Exactly one base-10000 digit
SELECT '10001::numeric = 10001::int4'::text AS test,
       10001::numeric = 10001::int4 AS result;  -- Two digits: [1, 1]
SELECT '-100000000::numeric = -100000000::int8'::text AS test,
       (-100000000)::numeric = (-100000000)::int8 AS result;  -- Negative with implicit zeros
SELECT '-100000000.5::numeric < -100000000::int8'::text AS test,
       (-100000000.5)::numeric < (-100000000)::int8 AS result;  -- Should be true
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

-- Edge Case: Boundary values with fractional parts
-- These test floor/comparison logic at integer type boundaries

-- int2 boundary edge cases (MIN = -32768, MAX = 32767)
SELECT 'int2 MIN - 0.1 < MIN'::text AS test,
       '-32768.1'::numeric < (-32768)::int2 AS result;  -- Should be true (floor is -32769)
SELECT 'int2 MIN + 0.1 > MIN'::text AS test,
       '-32767.9'::numeric > (-32768)::int2 AS result;  -- Should be true (floor is -32768, has fraction)
SELECT 'int2 MAX + 0.1 > MAX'::text AS test,
       '32767.1'::numeric > 32767::int2 AS result;  -- Should be true (floor is 32767, has fraction)
SELECT 'int2 MAX + 0.1 = MAX'::text AS test,
       '32767.1'::numeric = 32767::int2 AS result;  -- Should be false

-- int4 boundary edge cases (MIN = -2147483648, MAX = 2147483647)
SELECT 'int4 MIN - 0.1 < MIN'::text AS test,
       '-2147483648.1'::numeric < (-2147483648)::int4 AS result;  -- Should be true (floor is -2147483649)
SELECT 'int4 MIN + 0.1 > MIN'::text AS test,
       '-2147483647.9'::numeric > (-2147483648)::int4 AS result;  -- Should be true (floor is -2147483648, has fraction)
SELECT 'int4 MAX + 0.1 > MAX'::text AS test,
       '2147483647.1'::numeric > 2147483647::int4 AS result;  -- Should be true (floor is 2147483647, has fraction)
SELECT 'int4 MAX + 0.1 = MAX'::text AS test,
       '2147483647.1'::numeric = 2147483647::int4 AS result;  -- Should be false

-- int8 boundary edge cases (MIN = -9223372036854775808, MAX = 9223372036854775807)
SELECT 'int8 MIN - 0.1 < MIN'::text AS test,
       '-9223372036854775808.1'::numeric < ('-9223372036854775808'::numeric)::int8 AS result;  -- Should be true (floor overflows)
SELECT 'int8 MIN + 0.1 > MIN'::text AS test,
       '-9223372036854775807.9'::numeric > ('-9223372036854775808'::numeric)::int8 AS result;  -- Should be true (floor is MIN, has fraction)
SELECT 'int8 MAX + 0.1 > MAX'::text AS test,
       '9223372036854775807.1'::numeric > 9223372036854775807::int8 AS result;  -- Should be true (floor is MAX, has fraction)
SELECT 'int8 MAX + 0.1 = MAX'::text AS test,
       '9223372036854775807.1'::numeric = 9223372036854775807::int8 AS result;  -- Should be false

-- float4 boundary edge cases with int types
-- Note: float4 has limited precision, but boundary comparisons should still work
SELECT 'float4 at int2 MIN - 0.5'::text AS test,
       (-32768.5)::float4 < (-32768)::int2 AS result;  -- Should be true
SELECT 'float4 at int2 MAX + 0.5'::text AS test,
       (32767.5)::float4 > 32767::int2 AS result;  -- Should be true
SELECT 'float4 at int4 MIN - 0.5'::text AS test,
       (-2147483648.5)::float4 < (-2147483648)::int4 AS result;  -- false: float4 rounds -2147483648.5 to > int4 MIN

-- float8 boundary edge cases with int types
SELECT 'float8 at int2 MIN - 0.1'::text AS test,
       (-32768.1)::float8 < (-32768)::int2 AS result;  -- Should be true
SELECT 'float8 at int2 MAX + 0.1'::text AS test,
       (32767.1)::float8 > 32767::int2 AS result;  -- Should be true
SELECT 'float8 at int4 MIN - 0.1'::text AS test,
       (-2147483648.1)::float8 < (-2147483648)::int4 AS result;  -- Should be true
SELECT 'float8 at int4 MAX + 0.1'::text AS test,
       (2147483647.1)::float8 > 2147483647::int4 AS result;  -- Should be true
SELECT 'float8 at int8 MIN - 0.1'::text AS test,
       '-9223372036854775808.1'::float8 < ('-9223372036854775808'::numeric)::int8 AS result;  -- false: float8 rounds to same value as int8 MIN
SELECT 'float8 at int8 MAX + 0.1'::text AS test,
       '9223372036854775807.1'::float8 > 9223372036854775807::int8 AS result;  -- false: float8 rounds to same value as int8 MAX

-- Edge Case: SupportRequestSimplify (SRS) path (var op const)
-- These require a table column to trigger the optimizer support function
CREATE TEMPORARY TABLE t_boundary_int8 (val int8);
INSERT INTO t_boundary_int8 VALUES
  (('-9223372036854775808'::numeric)::int8),  -- MIN
  (9223372036854775807);                       -- MAX

-- Tests that go through SupportRequestSimplify (int8 column vs numeric constant)
SELECT 'SRS: int8 col > numeric below MIN'::text AS test,
       val > '-9223372036854775808.1'::numeric AS result
  FROM t_boundary_int8 WHERE val = ('-9223372036854775808'::numeric)::int8;  -- Should be true

SELECT 'SRS: int8 col < numeric above MAX'::text AS test,
       val < '9223372036854775807.1'::numeric AS result
  FROM t_boundary_int8 WHERE val = 9223372036854775807;  -- Should be true

SELECT 'SRS: int8 col = numeric at MIN'::text AS test,
       val = '-9223372036854775808'::numeric AS result
  FROM t_boundary_int8 WHERE val = ('-9223372036854775808'::numeric)::int8;  -- Should be true

SELECT 'SRS: int8 col = numeric below MIN'::text AS test,
       val = '-9223372036854775808.1'::numeric AS result
  FROM t_boundary_int8 WHERE val = ('-9223372036854775808'::numeric)::int8;  -- Should be false

-- Verify EXPLAIN shows the transformation (should be Result or use native int8 ops)
EXPLAIN (COSTS OFF) SELECT * FROM t_boundary_int8 WHERE val > '-9223372036854775808.1'::numeric;
EXPLAIN (COSTS OFF) SELECT * FROM t_boundary_int8 WHERE val < '9223372036854775807.1'::numeric;
