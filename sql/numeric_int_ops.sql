-- Test exact comparison operators for numeric × integer combinations
-- This test file covers equality (=) and inequality (<>) operators

-- Load extension (needed for first test in the sequence)
CREATE EXTENSION IF NOT EXISTS pg_num2int_direct_comp;

-- Test 1: Basic equality - numeric with no fractional part equals integer
SELECT 10::numeric = 10::int2 AS "numeric=int2_exact";
SELECT 10::numeric = 10::int4 AS "numeric=int4_exact";
SELECT 10::numeric = 10::int8 AS "numeric=int8_exact";

-- Test 2: Fractional part prevents equality
SELECT 10.5::numeric = 10::int2 AS "numeric_frac=int2_false";
SELECT 10.5::numeric = 10::int4 AS "numeric_frac=int4_false";
SELECT 10.5::numeric = 10::int8 AS "numeric_frac=int8_false";

-- Test 3: Negative values
SELECT (-5)::numeric = (-5)::int2 AS "numeric_neg=int2";
SELECT (-5)::numeric = (-5)::int4 AS "numeric_neg=int4";
SELECT (-5)::numeric = (-5)::int8 AS "numeric_neg=int8";

-- Test 4: Zero
SELECT 0::numeric = 0::int2 AS "numeric_zero=int2";
SELECT 0::numeric = 0::int4 AS "numeric_zero=int4";
SELECT 0::numeric = 0::int8 AS "numeric_zero=int8";

-- Test 5: Inequality operator - different values
SELECT 10::numeric <> 5::int2 AS "numeric<>int2_diff";
SELECT 10::numeric <> 5::int4 AS "numeric<>int4_diff";
SELECT 10::numeric <> 5::int8 AS "numeric<>int8_diff";

-- Test 6: Inequality with fractional part
SELECT 10.1::numeric <> 10::int2 AS "numeric_frac<>int2";
SELECT 10.1::numeric <> 10::int4 AS "numeric_frac<>int4";
SELECT 10.1::numeric <> 10::int8 AS "numeric_frac<>int8";

-- Test 7: Inequality returns false for equal values
SELECT 10::numeric <> 10::int2 AS "numeric<>int2_equal_false";
SELECT 10::numeric <> 10::int4 AS "numeric<>int4_equal_false";
SELECT 10::numeric <> 10::int8 AS "numeric<>int8_equal_false";

-- Test 8: Commutative property (reversed operands)
SELECT 10::int2 = 10::numeric AS "int2=numeric_commute";
SELECT 10::int4 = 10::numeric AS "int4=numeric_commute";
SELECT 10::int8 = 10::numeric AS "int8=numeric_commute";

-- Test 9: Large values within integer ranges
SELECT 32767::numeric = 32767::int2 AS "numeric=int2_max";
SELECT 2147483647::numeric = 2147483647::int4 AS "numeric=int4_max";
SELECT 9223372036854775807::numeric = 9223372036854775807::int8 AS "numeric=int8_max";

-- Test 10: Minimum integer values
SELECT (-32768)::numeric = (-32768)::int2 AS "numeric=int2_min";
SELECT (-2147483648)::numeric = (-2147483648)::int4 AS "numeric=int4_min";
SELECT (-9223372036854775808)::numeric = (-9223372036854775808)::int8 AS "numeric=int8_min";

-- Test 11: Decimal type (alias for numeric)
SELECT 10.0::decimal = 10::int4 AS "decimal=int4";
SELECT 10.5::decimal <> 10::int4 AS "decimal_frac<>int4";

-- ============================================================================
-- Range Comparison Tests (<, >, <=, >=)
-- ============================================================================

-- Test 12: Less than operator (<)
SELECT 5::numeric < 10::int2 AS "numeric_lt_int2_true";
SELECT 10::numeric < 10::int2 AS "numeric_lt_int2_equal_false";
SELECT 15::numeric < 10::int2 AS "numeric_lt_int2_false";
SELECT 5::numeric < 10::int4 AS "numeric_lt_int4_true";
SELECT 10::numeric < 10::int4 AS "numeric_lt_int4_equal_false";
SELECT 15::numeric < 10::int4 AS "numeric_lt_int4_false";
SELECT 5::numeric < 10::int8 AS "numeric_lt_int8_true";
SELECT 10::numeric < 10::int8 AS "numeric_lt_int8_equal_false";
SELECT 15::numeric < 10::int8 AS "numeric_lt_int8_false";

-- Test 13: Greater than operator (>)
SELECT 15::numeric > 10::int2 AS "numeric_gt_int2_true";
SELECT 10::numeric > 10::int2 AS "numeric_gt_int2_equal_false";
SELECT 5::numeric > 10::int2 AS "numeric_gt_int2_false";
SELECT 15::numeric > 10::int4 AS "numeric_gt_int4_true";
SELECT 10::numeric > 10::int4 AS "numeric_gt_int4_equal_false";
SELECT 5::numeric > 10::int4 AS "numeric_gt_int4_false";
SELECT 15::numeric > 10::int8 AS "numeric_gt_int8_true";
SELECT 10::numeric > 10::int8 AS "numeric_gt_int8_equal_false";
SELECT 5::numeric > 10::int8 AS "numeric_gt_int8_false";

-- Test 14: Less than or equal operator (<=)
SELECT 5::numeric <= 10::int2 AS "numeric_le_int2_less";
SELECT 10::numeric <= 10::int2 AS "numeric_le_int2_equal";
SELECT 15::numeric <= 10::int2 AS "numeric_le_int2_false";
SELECT 5::numeric <= 10::int4 AS "numeric_le_int4_less";
SELECT 10::numeric <= 10::int4 AS "numeric_le_int4_equal";
SELECT 15::numeric <= 10::int4 AS "numeric_le_int4_false";
SELECT 5::numeric <= 10::int8 AS "numeric_le_int8_less";
SELECT 10::numeric <= 10::int8 AS "numeric_le_int8_equal";
SELECT 15::numeric <= 10::int8 AS "numeric_le_int8_false";

-- Test 15: Greater than or equal operator (>=)
SELECT 15::numeric >= 10::int2 AS "numeric_ge_int2_greater";
SELECT 10::numeric >= 10::int2 AS "numeric_ge_int2_equal";
SELECT 5::numeric >= 10::int2 AS "numeric_ge_int2_false";
SELECT 15::numeric >= 10::int4 AS "numeric_ge_int4_greater";
SELECT 10::numeric >= 10::int4 AS "numeric_ge_int4_equal";
SELECT 5::numeric >= 10::int4 AS "numeric_ge_int4_false";
SELECT 15::numeric >= 10::int8 AS "numeric_ge_int8_greater";
SELECT 10::numeric >= 10::int8 AS "numeric_ge_int8_equal";
SELECT 5::numeric >= 10::int8 AS "numeric_ge_int8_false";

-- Test 16: Range operators with negative values
SELECT (-10)::numeric < (-5)::int4 AS "numeric_neg_lt_int4";
SELECT (-5)::numeric > (-10)::int4 AS "numeric_neg_gt_int4";
SELECT (-10)::numeric <= (-10)::int4 AS "numeric_neg_le_int4";
SELECT (-10)::numeric >= (-10)::int4 AS "numeric_neg_ge_int4";

-- Test 17: Range operators with fractional parts (strict comparison)
SELECT 10.5::numeric < 11::int4 AS "numeric_frac_lt_ceil_true";
SELECT 10.5::numeric < 10::int4 AS "numeric_frac_lt_floor_false";
SELECT 10.5::numeric > 10::int4 AS "numeric_frac_gt_floor_true";
SELECT 10.5::numeric > 11::int4 AS "numeric_frac_gt_ceil_false";
