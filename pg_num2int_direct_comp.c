/*
 * Copyright (c) 2026 dataStone Inc.
 *
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

/**
 * @file pg_num2int_direct_comp.c
 * @brief Exact comparison operators for numeric and integer types
 * @author Dave Sharpe
 * @date 2025-12-23
 *
 * This file was developed with assistance from AI tools.
 *
 * Implements exact comparison operators (=, <>, <, >, <=, >=) between
 * inexact numeric types (numeric, float4, float8) and integer types
 * (int2, int4, int8). Comparisons detect precision mismatches and
 * support btree index optimization via SupportRequestSimplify.
 */

#include "pg_num2int_direct_comp.h"
#include "catalog/pg_proc_d.h"
#include "catalog/pg_type_d.h"
#include "catalog/namespace.h"
#include "catalog/pg_operator.h"
#include "common/hashfn.h"
#include "nodes/supportnodes.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "optimizer/optimizer.h"
#include "utils/fmgrprotos.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/syscache.h"
#include "utils/inval.h"
#include "utils/guc.h"
#include "fmgr.h"
#include <math.h>

PG_MODULE_MAGIC;

/* GUC variables */
static bool enableSupportFunctions = true;

/* Static per-backend OID cache */
static OperatorOidCache oidCache = {
  .count = 0,
  .ops = {{.funcid = 0, .oid = 0, .type = OP_TYPE_UNKNOWN}}
};

/* Static per-backend Numeric boundary cache */
static NumericBoundaryCache numericBounds = {
  .initialized = false,
  .int2Min = NULL,
  .int2Max = NULL,
  .int4Min = NULL,
  .int4Max = NULL,
  .int8Min = NULL,
  .int8Max = NULL
};

/*
 * Pre-computed float boundaries for integer range checks.
 * These avoid runtime int-to-float conversions in hot paths.
 * Note: float4 can exactly represent all int16 values and most int32 values,
 * but int64 boundaries lose precision (which is fine for our range checks).
 */
static const float4 FLOAT4_INT16_MIN = (float4) PG_INT16_MIN;
static const float4 FLOAT4_INT16_MAX = (float4) PG_INT16_MAX;
static const float4 FLOAT4_INT32_MIN = (float4) PG_INT32_MIN;
static const float4 FLOAT4_INT32_MAX = (float4) PG_INT32_MAX;

static const float8 FLOAT8_INT16_MIN = (float8) PG_INT16_MIN;
static const float8 FLOAT8_INT16_MAX = (float8) PG_INT16_MAX;
static const float8 FLOAT8_INT32_MIN = (float8) PG_INT32_MIN;
static const float8 FLOAT8_INT32_MAX = (float8) PG_INT32_MAX;
static const float8 FLOAT8_INT64_MIN = (float8) PG_INT64_MIN;
static const float8 FLOAT8_INT64_MAX = (float8) PG_INT64_MAX;

/**
 * @brief Initialize cached Numeric boundary values
 * @param cache Pointer to cache structure to populate
 *
 * Converts integer MIN/MAX values to Numeric once and stores in TopMemoryContext.
 * These cached values avoid repeated int64_to_numeric() calls during range checks.
 */
void
initNumericBoundaries(NumericBoundaryCache *cache)
{
    MemoryContext oldcontext;

    if (cache->initialized)
        return;

    /*
     * Allocate Numeric values in TopMemoryContext so they persist for
     * the lifetime of the backend process.
     */
    oldcontext = MemoryContextSwitchTo(TopMemoryContext);

    cache->int2Min = int64_to_numeric((int64) PG_INT16_MIN);
    cache->int2Max = int64_to_numeric((int64) PG_INT16_MAX);
    cache->int4Min = int64_to_numeric((int64) PG_INT32_MIN);
    cache->int4Max = int64_to_numeric((int64) PG_INT32_MAX);
    cache->int8Min = int64_to_numeric(PG_INT64_MIN);
    cache->int8Max = int64_to_numeric(PG_INT64_MAX);

    MemoryContextSwitchTo(oldcontext);

    cache->initialized = true;

    ereport(DEBUG1,
            (errmsg("pg_num2int_direct_comp: initialized Numeric boundary cache")));
}

/* ============================================================================
 * Optimized Numeric Inspection Functions
 * ============================================================================
 * These functions use direct structure access via NUM2INT_NUMERIC_* macros
 * for efficiency, avoiding function call overhead for common operations.
 */

/**
 * @brief Get the sign of a Numeric value using direct header inspection
 * @param num Numeric value to inspect
 * @return 1 if positive, -1 if negative, 0 if zero
 *
 * Uses NUM2INT_NUMERIC_SIGN macro for direct header access.
 * For NaN, returns 0 (caller should check NUM2INT_NUMERIC_IS_NAN first).
 * For +Inf returns 1, for -Inf returns -1.
 *
 * Based on: src/backend/utils/adt/numeric.c numeric_sign()
 */
int
num2int_numeric_sign(Numeric num)
{
    int sign;
    int ndigits;

    /* Handle special values (NaN, Inf) */
    if (NUM2INT_NUMERIC_IS_SPECIAL(num))
    {
        if (NUM2INT_NUMERIC_IS_NAN(num))
            return 0;  /* NaN has no meaningful sign */
        if (NUM2INT_NUMERIC_IS_PINF(num))
            return 1;
        /* Must be -Inf */
        return -1;
    }

    /* Check for zero: no digits means zero */
    ndigits = NUM2INT_NUMERIC_NDIGITS(num);
    if (ndigits == 0)
        return 0;

    /* Get sign from header */
    sign = NUM2INT_NUMERIC_SIGN(num);
    return (sign == NUM2INT_NUMERIC_POS) ? 1 : -1;
}

/**
 * @brief Check if Numeric value is integral (no fractional part)
 * @param num Numeric value to check
 * @return true if integral, false if has fractional digits or is NaN
 *
 * Uses the relationship: a value is integral when all digits fit within
 * the weight. If ndigits <= weight + 1, there are no fractional digits.
 * Infinities are considered integral (matches PostgreSQL behavior).
 *
 * Based on: src/backend/utils/adt/numeric.c numeric_is_integral()
 *           (introduced in PostgreSQL 15)
 */
bool
num2int_numeric_is_integral(Numeric num)
{
    int weight;
    int ndigits;

    /* Handle special values */
    if (NUM2INT_NUMERIC_IS_SPECIAL(num))
    {
        /* NaN is not integral */
        return !NUM2INT_NUMERIC_IS_NAN(num);
    }

    ndigits = NUM2INT_NUMERIC_NDIGITS(num);

    /* Zero (no digits) is integral */
    if (ndigits == 0)
        return true;

    weight = NUM2INT_NUMERIC_WEIGHT(num);

    /*
     * Numeric stores base-10000 digits. Weight indicates position of first
     * digit (0 = units place, 1 = 10000s place, etc).
     * If ndigits <= weight + 1, all digits are in integral positions.
     */
    return ndigits <= weight + 1;
}

/**
 * @brief Convert Numeric to int64 if possible using direct digit access
 * @param num Numeric value to convert
 * @param result Output: converted value
 * @return true if conversion succeeded, false if out of range, has fraction, or special
 *
 * This function directly accesses the Numeric digit array for efficient
 * conversion, avoiding the overhead of the standard numeric_int8() path.
 */
bool
numericToint64(Numeric num, int64 *result)
{
  int weight;
  int ndigits;
  int sign;
  int i;
  int64 val;
  Num2IntNumericDigit *digits;

  /* Reject special values */
  if (NUM2INT_NUMERIC_IS_SPECIAL(num))
    return false;

  ndigits = NUM2INT_NUMERIC_NDIGITS(num);

  /* Zero case */
  if (ndigits == 0) {
    *result = 0;
    return true;
  }

  weight = NUM2INT_NUMERIC_WEIGHT(num);
  sign = NUM2INT_NUMERIC_SIGN(num);

  /* Must be integral */
  if (ndigits > weight + 1)
    return false;

  /*
   * Quick range check: int64 max is about 9.2e18
   * Each weight unit represents 10000^weight, so weight 4 = 10^16, weight 5 = 10^20
   * Weight > 4 could overflow (but depends on leading digit)
   * Weight <= 3 is definitely safe
   */
  if (weight > 4)
    return false;

  digits = NUM2INT_NUMERIC_DIGITS(num);
  val = 0;
  /* Accumulate digits, checking for overflow */
  for (i = 0; i <= weight; i++) {
    int64 digit = (i < ndigits) ? digits[i] : 0;
    int64 newval;
    /* Check multiplication overflow */
    if (val > PG_INT64_MAX / NUM2INT_NBASE)
      return false;
    newval = val * NUM2INT_NBASE;

    /* Check addition overflow */
    if (sign == NUM2INT_NUMERIC_POS) {
      if (digit > PG_INT64_MAX - newval)
        return false;
      newval += digit;
    }
    else {
      /* For negative, we build positive then negate */
      if (digit > PG_INT64_MAX - newval) {
        /*
         * Special case: PG_INT64_MIN has magnitude one greater than
         * PG_INT64_MAX, so we can represent -9223372036854775808
         */
        if (newval == PG_INT64_MAX - digit + 1 && i == weight) {
          *result = PG_INT64_MIN;
          return true;
        }
        return false;
      }
      newval += digit;
    }
    val = newval;
  }

  *result = (sign == NUM2INT_NUMERIC_POS) ? val : -val;
  return true;
}

/**
 * @brief Extract floor of Numeric as int64 using direct digit access
 * @param num Numeric value (must not be NaN/Inf - caller should check)
 * @param result Output: floor value as int64
 * @return true if floor fits in int64, false if out of range
 *
 * For integral values, returns the value itself.
 * For fractional values, extracts only the integral digits and adjusts
 * for negative numbers (floor rounds toward -infinity).
 */
static bool
numericFloorToInt64(Numeric num, int64 *result)
{
  int weight;
  int ndigits;
  int sign;
  int integralNdigits;
  int64 floorVal;
  int i;
  Num2IntNumericDigit *digits;

  ndigits = NUM2INT_NUMERIC_NDIGITS(num);

  /* Zero case */
  if (ndigits == 0) {
    *result = 0;
    return true;
  }

  weight = NUM2INT_NUMERIC_WEIGHT(num);
  sign = NUM2INT_NUMERIC_SIGN(num);
  digits = NUM2INT_NUMERIC_DIGITS(num);

  /*
   * If weight > 4, the integral part exceeds int64 range.
   * (weight 5 means 10000^5 = 10^20, but int64 max is ~9.2e18)
   */
  if (weight > 4)
    return false;

  /* Number of integral digits is weight + 1 */
  integralNdigits = weight + 1;

  if (integralNdigits <= 0) {
    /*
     * No integral digits (e.g., 0.5 has weight=-1).
     * floor(positive fraction) = 0, floor(negative fraction) = -1
     */
    *result = (sign == NUM2INT_NUMERIC_POS) ? 0 : -1;
    return true;
  }

  /* Check if value is integral (no fractional part) */
  if (ndigits <= integralNdigits) {
    /* Integral - use existing extraction */
    return numericToint64(num, result);
  }

  /*
   * Has fractional part - extract only integral digits
   * padding with zeros if ndigits < integralNdigits
   */
  floorVal = 0;
  for (i = 0; i < integralNdigits; i++) {
    Num2IntNumericDigit digit = (i < ndigits) ? digits[i] : 0;

    /* Overflow check for multiplication */
    if (floorVal > PG_INT64_MAX / NUM2INT_NBASE)
      return false;
    floorVal = floorVal * NUM2INT_NBASE;

    /* Overflow check for addition */
    if (digit > PG_INT64_MAX - floorVal)
      return false;
    floorVal += digit;
  }

  /* Apply sign and floor adjustment for negative */
  if (sign == NUM2INT_NUMERIC_POS) {
    *result = floorVal;
  } else {
    /*
     * For negative with fractional part: floor = -trunc - 1
     * e.g., floor(-100.5) = -101, not -100
     *
     * No overflow: floorVal <= PG_INT64_MAX, so
     * -floorVal - 1 >= PG_INT64_MIN.
     */
    *result = -floorVal - 1;
  }

  return true;
}

/**
 * @brief Syscache invalidation callback for operator changes
 *
 * Called by PostgreSQL when any operator is created, dropped, or modified.
 * Invalidates our cached OIDs so they are re-looked up on next use.
 */
static void
operatorCacheInvalidationCallback(Datum arg, int cacheid, uint32 hashvalue)
{
    oidCache.count = 0;
}

/**
 * @brief Module initialization - called once when shared library is loaded
 *
 * Registers a syscache invalidation callback so our OID cache is automatically
 * invalidated when operators are created/dropped (e.g., DROP/CREATE EXTENSION).
 * Also initializes the Numeric boundary cache for efficient range checking.
 */
void
_PG_init(void)
{
    ereport(DEBUG1,
            (errmsg("pg_num2int_direct_comp: _PG_init() - registering OPEROID syscache callback")));

    /* Define GUC for enabling/disabling support functions */
    DefineCustomBoolVariable("pg_num2int_direct_comp.enableSupportFunctions",
                           "Enable SupportRequestSimplify optimization",
                           "When enabled, allows the extension to optimize queries by transforming "
                           "cross-type comparisons into same-type comparisons at plan time. "
                           "Disable for testing or if optimization causes issues.",
                           &enableSupportFunctions,
                           true,                /* boot_val */
                           PGC_USERSET,        /* context */
                           0,                  /* flags */
                           NULL,               /* check_hook */
                           NULL,               /* assign_hook */
                           NULL);              /* show_hook */

    CacheRegisterSyscacheCallback(OPEROID,
                                  operatorCacheInvalidationCallback,
                                  (Datum) 0);

    /* Initialize Numeric boundary cache for range checking */
    initNumericBoundaries(&numericBounds);
}

/**
 * @brief Initialize the operator OID cache
 * @param cache Pointer to cache structure to populate
 *
 * Performs lazy initialization of operator OIDs by looking up operator
 * names in the system catalog. This function is called once per backend
 * on first invocation of the support function and after cache invalidation.
 */
void
initOidCache(OperatorOidCache *cache) {
  MemoryContext oldcontext;
  MemoryContext tmpcontext;

  if (cache->count > 0) {
    return;
  }

  /*
   * Create a temporary memory context for intermediate allocations.
   * OpernameGetOprid's list_make1(makeString(...)) calls allocate List and
   * String nodes that we don't need after extracting the Oid values.
   */
  tmpcontext = AllocSetContextCreate(CurrentMemoryContext,
                                     "OID cache init",
                                     ALLOCSET_SMALL_SIZES);
  oldcontext = MemoryContextSwitchTo(tmpcontext);

  /*
   * Helper macro to add an operator to the cache.
   * Looks up the operator by name and type pair, stores OID, funcid, and type.
   * Only adds to cache if operator is found (OidIsValid).
   */
  #define ADD_OP(op_str, left_type, right_type, opType) \
    do { \
      Oid _oid = OpernameGetOprid(list_make1(makeString(op_str)), \
                                  left_type, right_type); \
      if (OidIsValid(_oid)) { \
        cache->ops[cache->count].oid = _oid; \
        cache->ops[cache->count].funcid = get_opcode(_oid); \
        cache->ops[cache->count].type = opType; \
        cache->count++; \
      } \
    } while(0)

  /* Equality operators (=) - forward direction */
  ADD_OP("=", NUMERICOID, INT2OID, OP_TYPE_EQ);
  ADD_OP("=", NUMERICOID, INT4OID, OP_TYPE_EQ);
  ADD_OP("=", NUMERICOID, INT8OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT4OID, INT2OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT4OID, INT4OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT4OID, INT8OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT8OID, INT2OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT8OID, INT4OID, OP_TYPE_EQ);
  ADD_OP("=", FLOAT8OID, INT8OID, OP_TYPE_EQ);
  /* Equality operators (=) - commutator direction */
  ADD_OP("=", INT2OID, NUMERICOID, OP_TYPE_EQ);
  ADD_OP("=", INT2OID, FLOAT4OID, OP_TYPE_EQ);
  ADD_OP("=", INT2OID, FLOAT8OID, OP_TYPE_EQ);
  ADD_OP("=", INT4OID, NUMERICOID, OP_TYPE_EQ);
  ADD_OP("=", INT4OID, FLOAT4OID, OP_TYPE_EQ);
  ADD_OP("=", INT4OID, FLOAT8OID, OP_TYPE_EQ);
  ADD_OP("=", INT8OID, NUMERICOID, OP_TYPE_EQ);
  ADD_OP("=", INT8OID, FLOAT4OID, OP_TYPE_EQ);
  ADD_OP("=", INT8OID, FLOAT8OID, OP_TYPE_EQ);

  /* Inequality operators (<>) - forward direction */
  ADD_OP("<>", NUMERICOID, INT2OID, OP_TYPE_NE);
  ADD_OP("<>", NUMERICOID, INT4OID, OP_TYPE_NE);
  ADD_OP("<>", NUMERICOID, INT8OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT4OID, INT2OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT4OID, INT4OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT4OID, INT8OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT8OID, INT2OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT8OID, INT4OID, OP_TYPE_NE);
  ADD_OP("<>", FLOAT8OID, INT8OID, OP_TYPE_NE);
  /* Inequality operators (<>) - commutator direction */
  ADD_OP("<>", INT2OID, NUMERICOID, OP_TYPE_NE);
  ADD_OP("<>", INT2OID, FLOAT4OID, OP_TYPE_NE);
  ADD_OP("<>", INT2OID, FLOAT8OID, OP_TYPE_NE);
  ADD_OP("<>", INT4OID, NUMERICOID, OP_TYPE_NE);
  ADD_OP("<>", INT4OID, FLOAT4OID, OP_TYPE_NE);
  ADD_OP("<>", INT4OID, FLOAT8OID, OP_TYPE_NE);
  ADD_OP("<>", INT8OID, NUMERICOID, OP_TYPE_NE);
  ADD_OP("<>", INT8OID, FLOAT4OID, OP_TYPE_NE);
  ADD_OP("<>", INT8OID, FLOAT8OID, OP_TYPE_NE);

  /* Less than operators (<) - forward direction */
  ADD_OP("<", NUMERICOID, INT2OID, OP_TYPE_LT);
  ADD_OP("<", NUMERICOID, INT4OID, OP_TYPE_LT);
  ADD_OP("<", NUMERICOID, INT8OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT4OID, INT2OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT4OID, INT4OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT4OID, INT8OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT8OID, INT2OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT8OID, INT4OID, OP_TYPE_LT);
  ADD_OP("<", FLOAT8OID, INT8OID, OP_TYPE_LT);
  /* Less than operators (<) - commutator direction */
  ADD_OP("<", INT2OID, NUMERICOID, OP_TYPE_LT);
  ADD_OP("<", INT2OID, FLOAT4OID, OP_TYPE_LT);
  ADD_OP("<", INT2OID, FLOAT8OID, OP_TYPE_LT);
  ADD_OP("<", INT4OID, NUMERICOID, OP_TYPE_LT);
  ADD_OP("<", INT4OID, FLOAT4OID, OP_TYPE_LT);
  ADD_OP("<", INT4OID, FLOAT8OID, OP_TYPE_LT);
  ADD_OP("<", INT8OID, NUMERICOID, OP_TYPE_LT);
  ADD_OP("<", INT8OID, FLOAT4OID, OP_TYPE_LT);
  ADD_OP("<", INT8OID, FLOAT8OID, OP_TYPE_LT);

  /* Greater than operators (>) - forward direction */
  ADD_OP(">", NUMERICOID, INT2OID, OP_TYPE_GT);
  ADD_OP(">", NUMERICOID, INT4OID, OP_TYPE_GT);
  ADD_OP(">", NUMERICOID, INT8OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT4OID, INT2OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT4OID, INT4OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT4OID, INT8OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT8OID, INT2OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT8OID, INT4OID, OP_TYPE_GT);
  ADD_OP(">", FLOAT8OID, INT8OID, OP_TYPE_GT);
  /* Greater than operators (>) - commutator direction */
  ADD_OP(">", INT2OID, NUMERICOID, OP_TYPE_GT);
  ADD_OP(">", INT2OID, FLOAT4OID, OP_TYPE_GT);
  ADD_OP(">", INT2OID, FLOAT8OID, OP_TYPE_GT);
  ADD_OP(">", INT4OID, NUMERICOID, OP_TYPE_GT);
  ADD_OP(">", INT4OID, FLOAT4OID, OP_TYPE_GT);
  ADD_OP(">", INT4OID, FLOAT8OID, OP_TYPE_GT);
  ADD_OP(">", INT8OID, NUMERICOID, OP_TYPE_GT);
  ADD_OP(">", INT8OID, FLOAT4OID, OP_TYPE_GT);
  ADD_OP(">", INT8OID, FLOAT8OID, OP_TYPE_GT);

  /* Less than or equal operators (<=) - forward direction */
  ADD_OP("<=", NUMERICOID, INT2OID, OP_TYPE_LE);
  ADD_OP("<=", NUMERICOID, INT4OID, OP_TYPE_LE);
  ADD_OP("<=", NUMERICOID, INT8OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT4OID, INT2OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT4OID, INT4OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT4OID, INT8OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT8OID, INT2OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT8OID, INT4OID, OP_TYPE_LE);
  ADD_OP("<=", FLOAT8OID, INT8OID, OP_TYPE_LE);
  /* Less than or equal operators (<=) - commutator direction */
  ADD_OP("<=", INT2OID, NUMERICOID, OP_TYPE_LE);
  ADD_OP("<=", INT2OID, FLOAT4OID, OP_TYPE_LE);
  ADD_OP("<=", INT2OID, FLOAT8OID, OP_TYPE_LE);
  ADD_OP("<=", INT4OID, NUMERICOID, OP_TYPE_LE);
  ADD_OP("<=", INT4OID, FLOAT4OID, OP_TYPE_LE);
  ADD_OP("<=", INT4OID, FLOAT8OID, OP_TYPE_LE);
  ADD_OP("<=", INT8OID, NUMERICOID, OP_TYPE_LE);
  ADD_OP("<=", INT8OID, FLOAT4OID, OP_TYPE_LE);
  ADD_OP("<=", INT8OID, FLOAT8OID, OP_TYPE_LE);

  /* Greater than or equal operators (>=) - forward direction */
  ADD_OP(">=", NUMERICOID, INT2OID, OP_TYPE_GE);
  ADD_OP(">=", NUMERICOID, INT4OID, OP_TYPE_GE);
  ADD_OP(">=", NUMERICOID, INT8OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT4OID, INT2OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT4OID, INT4OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT4OID, INT8OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT8OID, INT2OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT8OID, INT4OID, OP_TYPE_GE);
  ADD_OP(">=", FLOAT8OID, INT8OID, OP_TYPE_GE);
  /* Greater than or equal operators (>=) - commutator direction */
  ADD_OP(">=", INT2OID, NUMERICOID, OP_TYPE_GE);
  ADD_OP(">=", INT2OID, FLOAT4OID, OP_TYPE_GE);
  ADD_OP(">=", INT2OID, FLOAT8OID, OP_TYPE_GE);
  ADD_OP(">=", INT4OID, NUMERICOID, OP_TYPE_GE);
  ADD_OP(">=", INT4OID, FLOAT4OID, OP_TYPE_GE);
  ADD_OP(">=", INT4OID, FLOAT8OID, OP_TYPE_GE);
  ADD_OP(">=", INT8OID, NUMERICOID, OP_TYPE_GE);
  ADD_OP(">=", INT8OID, FLOAT4OID, OP_TYPE_GE);
  ADD_OP(">=", INT8OID, FLOAT8OID, OP_TYPE_GE);

  #undef ADD_OP

  /* Switch back and delete temp context (frees all List/String nodes) */
  MemoryContextSwitchTo(oldcontext);
  MemoryContextDelete(tmpcontext);
}

/*
 * ============================================================================
 * Support Function Helper Types and Functions
 * ============================================================================
 * These helpers are used by the SupportRequestSimplify handler.
 */

/**
 * @brief Result of constant conversion
 */
typedef struct {
  bool valid;            /**< True if conversion succeeded */
  bool hasFraction;      /**< True if constant has fractional part */
  bool outOfRangeHigh;   /**< True if floor exceeds integer type max */
  bool outOfRangeLow;    /**< True if floor is below integer type min */
  int64 intVal;          /**< Converted integer value (floor) */
} ConstConversion;

/**
 * @brief Find operator OID from function OID by searching cached operators
 * @param funcid Function OID to match
 * @param opType Output: operator type classification
 * @return Matching operator OID or InvalidOid
 *
 * Searches the cached operator OIDs to find one whose implementing function
 * matches the given function OID. This is used to identify which operator
 * is being called when the support function receives a function call.
 */
static Oid
findOperatorByFuncid(Oid funcid, OpType *opType) {

  /* Initialize cache if needed */
  initOidCache(&oidCache);

  /* Linear scan through the lookup array */
  for (int i = 0; i < oidCache.count; i++) {
    if (oidCache.ops[i].funcid == funcid) {
      *opType = oidCache.ops[i].type;
      return oidCache.ops[i].oid;
    }
  }

  *opType = OP_TYPE_UNKNOWN;
  return InvalidOid;
}

/**
 * @brief Convert a numeric/float constant to integer
 * @param constNode Constant node to convert
 * @param intType Target integer type OID (INT2OID, INT4OID, INT8OID)
 * @return Conversion result with validity flag, fractional flag,
 *         out of range high/low flags, and value
 */
static ConstConversion
convertConstToInt(Const *constNode, Oid intType) {
  ConstConversion result = {
    .valid = false,
    .hasFraction = false,
    .outOfRangeHigh = false,
    .outOfRangeLow = false,
    .intVal = 0
  };
  Oid constType = constNode->consttype;

  /* NULL constants: return invalid to preserve original operator for NULL semantics */
  if (constNode->constisnull) {
    return result;
  }

  if (constType == NUMERICOID) {
    Numeric num = DatumGetNumeric(constNode->constvalue);

    /* Special value NaN, +Inf, -Inf */
    if (NUM2INT_NUMERIC_IS_SPECIAL(num))
      return result;

    /* Check if value has fractional part */
    result.hasFraction = !num2int_numeric_is_integral(num);

    /*
     * Extract floor value directly from digit array.
     */
    if (numericFloorToInt64(num, &result.intVal)) {
      /* Check narrower ranges for int2/int4 */
      if (intType == INT2OID) {
        if (result.intVal < PG_INT16_MIN) {
          result.outOfRangeLow = true;
        } else if (result.intVal > PG_INT16_MAX) {
          result.outOfRangeHigh = true;
        } else {
          result.valid = true;
        }
      } else if (intType == INT4OID) {
        if (result.intVal < PG_INT32_MIN) {
          result.outOfRangeLow = true;
        } else if (result.intVal > PG_INT32_MAX) {
          result.outOfRangeHigh = true;
        } else {
          result.valid = true;
        }
      } else {
        /* int8 - already in range since extraction succeeded */
        result.valid = true;
      }
    } else {
      /*
       * Direct extraction failed - value is outside int64 range.
       * Determine direction using sign.
       */
      if (num2int_numeric_sign(num) < 0)
        result.outOfRangeLow = true;
      else
        result.outOfRangeHigh = true;
    }
  } else if (constType == FLOAT4OID) {
    float4 fval = DatumGetFloat4(constNode->constvalue);
    float4 floorVal;

    if (isnan(fval) || isinf(fval)) {
      return result;
    }

    /* Use floor for consistent rounding toward negative infinity */
    floorVal = floorf(fval);
    result.hasFraction = (fval != floorVal);

    if (intType == INT2OID) {
      if (floorVal < FLOAT4_INT16_MIN) {
        result.outOfRangeLow = true;
      } else if (floorVal > FLOAT4_INT16_MAX) {
        result.outOfRangeHigh = true;
      } else {
        result.intVal = (int16) floorVal;
        result.valid = true;
      }
    } else if (intType == INT4OID) {
      if (floorVal < FLOAT4_INT32_MIN) {
        result.outOfRangeLow = true;
      } else if (floorVal > FLOAT4_INT32_MAX) {
        result.outOfRangeHigh = true;
      } else {
        result.intVal = (int32) floorVal;
        result.valid = true;
      }
    } else if (intType == INT8OID) {
      /* float4 always fits in int8 range */
      result.intVal = (int64) floorVal;
      result.valid = true;
    }
  } else if (constType == FLOAT8OID) {
    float8 dval = DatumGetFloat8(constNode->constvalue);
    float8 floorVal;

    if (isnan(dval) || isinf(dval)) {
      return result;
    }

    /* Use floor for consistent rounding toward negative infinity */
    floorVal = floor(dval);
    result.hasFraction = (dval != floorVal);

    if (intType == INT2OID) {
      if (floorVal < FLOAT8_INT16_MIN) {
        result.outOfRangeLow = true;
      } else if (floorVal > FLOAT8_INT16_MAX) {
        result.outOfRangeHigh = true;
      } else {
        result.intVal = (int16) floorVal;
        result.valid = true;
      }
    } else if (intType == INT4OID) {
      if (floorVal < FLOAT8_INT32_MIN) {
        result.outOfRangeLow = true;
      } else if (floorVal > FLOAT8_INT32_MAX) {
        result.outOfRangeHigh = true;
      } else {
        result.intVal = (int32) floorVal;
        result.valid = true;
      }
    } else if (intType == INT8OID) {
      if (floorVal < FLOAT8_INT64_MIN) {
        result.outOfRangeLow = true;
      } else if (floorVal > FLOAT8_INT64_MAX) {
        result.outOfRangeHigh = true;
      } else {
        result.intVal = (int64) floorVal;
        result.valid = true;
      }
    }
  }

  return result;
}

/**
 * @brief Map integer type OID to lookup table index
 * @param intType Integer type OID (INT2OID, INT4OID, or INT8OID)
 * @return Index 0, 1, or 2 for int2, int4, int8 respectively
 */
static inline int
intTypeIndex(Oid intType) {
  return (intType == INT2OID) ? 0 :
         (intType == INT4OID) ? 1 : /* INT8OID */ 2;
}

/**
 * @brief Native operator OID lookup table [opType - 1][intTypeIndex]
 *
 * Indexed by (OpType - 1) for rows and intTypeIndex() for columns.
 * OP_TYPE_UNKNOWN (0) is handled separately before table lookup.
 */
static const Oid NativeOpOids[6][3] = {
  /* OP_TYPE_EQ (1) */ { NUM2INT_INT2EQ_OID, NUM2INT_INT4EQ_OID, NUM2INT_INT8EQ_OID },
  /* OP_TYPE_NE (2) */ { NUM2INT_INT2NE_OID, NUM2INT_INT4NE_OID, NUM2INT_INT8NE_OID },
  /* OP_TYPE_LT (3) */ { NUM2INT_INT2LT_OID, NUM2INT_INT4LT_OID, NUM2INT_INT8LT_OID },
  /* OP_TYPE_GT (4) */ { NUM2INT_INT2GT_OID, NUM2INT_INT4GT_OID, NUM2INT_INT8GT_OID },
  /* OP_TYPE_LE (5) */ { NUM2INT_INT2LE_OID, NUM2INT_INT4LE_OID, NUM2INT_INT8LE_OID },
  /* OP_TYPE_GE (6) */ { NUM2INT_INT2GE_OID, NUM2INT_INT4GE_OID, NUM2INT_INT8GE_OID },
};

/**
 * @brief Get native integer operator OID for given operator type and integer type
 * @param opType Operator type classification
 * @param intType Integer type OID
 * @return Native operator OID for same-type comparison
 */
static Oid
getNativeOpOid(OpType opType, Oid intType) {
  if (opType == OP_TYPE_UNKNOWN)
    return InvalidOid;
  return NativeOpOids[opType - 1][intTypeIndex(intType)];
}

/**
 * @brief Create an integer constant node
 * @param intType Integer type OID
 * @param intVal Integer value
 * @return New Const node
 */
static Const *
makeIntConst(Oid intType, int64 intVal) {
  switch(intType) {
  case INT2OID:
    return makeConst(INT2OID, -1, InvalidOid, sizeof(int16),
                     Int16GetDatum((int16) intVal), false, true);
  case INT4OID:
    return makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                     Int32GetDatum((int32) intVal), false, true);
  case INT8OID:
  default:
    return makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
                     Int64GetDatum(intVal), false, true);
  }
}

/**
 * @brief Build transformed OpExpr using native integer operator
 * @param nativeOpOid Native operator OID
 * @param varNode Variable node (will be copied)
 * @param intVal Integer constant value
 * @param intType Integer type OID
 * @param location Source location for new expression
 * @param inputcollid Input collation ID
 * @return New OpExpr node
 */
static OpExpr *
buildNativeOpExpr(Oid nativeOpOid, Var *varNode, int64 intVal,
                  Oid intType, int location, Oid inputcollid) {
  Var *newVar = (Var *) copyObject(varNode);
  Const *newConst = makeIntConst(intType, intVal);

  OpExpr *newClause = (OpExpr *) make_opclause(nativeOpOid,
                                                BOOLOID,
                                                false,
                                                (Expr *) newVar,
                                                (Expr *) newConst,
                                                InvalidOid,
                                                InvalidOid);
  newClause->inputcollid = inputcollid;
  newClause->location = location;

  return newClause;
}

/**
 * @brief Compute adjusted value and operator for range predicates with fractions
 * @param opType Original operator type
 * @param intType Integer type OID
 * @param intVal Floor of original constant value
 * @param hasFraction Whether constant has fractional part
 * @param adjustedVal Output: adjusted integer value
 * @return Native operator OID to use
 */
static Oid
computeRangeTransform(OpType opType, Oid intType, int64 intVal,
                      bool hasFraction, int64 *adjustedVal,
                      bool *isAlwaysTrue, bool *isAlwaysFalse) {
  int64 typeMax;

  *adjustedVal = intVal;
  *isAlwaysTrue = false;
  *isAlwaysFalse = false;

  /* Determine type bounds */
  if (intType == INT2OID) {
    typeMax = PG_INT16_MAX;
  } else if (intType == INT4OID) {
    typeMax = PG_INT32_MAX;
  } else {
    typeMax = PG_INT64_MAX;
  }

  if (hasFraction) {
    if (opType == OP_TYPE_GT || opType == OP_TYPE_GE) {
      /*
       * > 10.5 or >= 10.5 becomes >= 11
       * But if intVal == typeMax, intVal + 1 overflows, and no value can satisfy >= (max+1)
       * So the predicate is always FALSE.
       */
      if (intVal >= typeMax) {
        *isAlwaysFalse = true;
        return InvalidOid;
      }
      *adjustedVal = intVal + 1;
      return getNativeOpOid(OP_TYPE_GE, intType);
    } else {
      /*
       * < 10.5 or <= 10.5 becomes <= 10
       * If intVal == typeMax, then <= typeMax is always TRUE for that type.
       * Similarly if intVal > typeMax (already handled by outOfRange).
       */
      if (intVal >= typeMax) {
        *isAlwaysTrue = true;
        return InvalidOid;
      }
      return getNativeOpOid(OP_TYPE_LE, intType);
    }
  }

  /* No fraction - preserve original operator */
  return getNativeOpOid(opType, intType);
}

/*
 * ============================================================================
 * Main Support Function
 * ============================================================================
 */

/**
 * @brief Support function for query optimization via SupportRequestSimplify
 * @param fcinfo Function call context
 * @return Node pointer for simplified expression or NULL
 *
 * Simplifies constant predicates during query planning:
 * - Equality with fractional value → FALSE (no integer can equal a fraction)
 * - Equality with exact integer → native integer operator (enables index use)
 * - Range with fraction → adjusted boundary (e.g., > 10.5 becomes >= 11)
 * - Out-of-range constants → TRUE/FALSE based on operator semantics
 *
 * Index optimization works because transformed expressions use native integer
 * operators which the planner automatically recognizes for btree index scans.
 */
PG_FUNCTION_INFO_V1(num2int_support);
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  Node *ret = NULL;

  /* Initialize OID cache on first use */
  initOidCache(&oidCache);

  if (IsA(rawreq, SupportRequestSimplify)) {
    /*
     * SupportRequestSimplify: Transform constant predicates at simplification time
     *
     * - FR-015: Equality with fractional value → FALSE
     * - FR-016: Equality with exact integer → native operator
     * - FR-017: Range with fraction → adjusted boundary
     */
    SupportRequestSimplify *req = (SupportRequestSimplify *) rawreq;
    FuncExpr *func = req->fcall;
    Node *leftop;
    Node *rightop;
    Var *varNode = NULL;
    Const *constNode = NULL;
    OpType opType;
    Oid opno;
    Oid intType;
    ConstConversion conv;

    /* Check if support functions are disabled via GUC */
    if (!enableSupportFunctions) {
      elog(DEBUG1, "SupportRequestSimplify disabled by pg_num2int_direct_comp.enableSupportFunctions");
      PG_RETURN_POINTER(NULL);
    }

    elog(DEBUG1, "SupportRequestSimplify called for function %u", func->funcid);

    if (list_length(func->args) != 2) {
      PG_RETURN_POINTER(NULL);
    }

    leftop = (Node *) linitial(func->args);
    rightop = (Node *) lsecond(func->args);

    /* Identify Var and Const positions and determine their types */
    if (IsA(leftop, Const) && IsA(rightop, Var)) {
      constNode = (Const *) leftop;
      varNode = (Var *) rightop;
    } else if (IsA(leftop, Var) && IsA(rightop, Const)) {
      varNode = (Var *) leftop;
      constNode = (Const *) rightop;
    } else {
      PG_RETURN_POINTER(NULL);
    }

    if (constNode->constisnull) {
      PG_RETURN_POINTER(NULL);
    }

    /* Find the operator that uses this function */
    opno = findOperatorByFuncid(func->funcid, &opType);
    if (opno == InvalidOid) {
      PG_RETURN_POINTER(NULL);
    }

    /* Convert constant to integer */
    intType = varNode->vartype;
    conv = convertConstToInt(constNode, intType);

    elog(DEBUG1,
       "SupportRequestSimplify: constType=%u, varType=%u, valid=%d, "
       "hasFraction=%d, outOfRangeLow=%d, outOfRangeHigh=%d, "
       "intVal=%ld",
       constNode->consttype, varNode->vartype, conv.valid,
       conv.hasFraction, conv.outOfRangeLow, conv.outOfRangeHigh,
       conv.intVal);

    /* Handle out-of-range constants */
    if (conv.outOfRangeHigh || conv.outOfRangeLow) {
      /*
       * Constant is outside the range of the integer type.
       * We can determine the result based on operator type:
       * - EQ: always FALSE (no integer can equal an out-of-range value)
       * - NE: always TRUE (every integer differs from an out-of-range value)
       * - LT/LE: TRUE if const is high, FALSE if const is low
       * - GT/GE: FALSE if const is high, TRUE if const is low
       */
      switch (opType) {
        case OP_TYPE_EQ:
          ret = (Node *) makeBoolConst(false, false);
          break;
        case OP_TYPE_NE:
          ret = (Node *) makeBoolConst(true, false);
          break;
        case OP_TYPE_LT:
        case OP_TYPE_LE:
          /* intCol < hugeValue is always TRUE; intCol < tinyValue is always FALSE */
          ret = (Node *) makeBoolConst(conv.outOfRangeHigh, false);
          break;
        case OP_TYPE_GT:
        case OP_TYPE_GE:
          /* intCol > tinyValue is always TRUE; intCol > hugeValue is always FALSE */
          ret = (Node *) makeBoolConst(conv.outOfRangeLow, false);
          break;
        default:
          break;
      }
      PG_RETURN_POINTER(ret);
    }

    if (!conv.valid) {
      PG_RETURN_POINTER(NULL);
    }

    /* Apply transformation based on operator type and fractional status */
    if (opType == OP_TYPE_EQ) {
      if (conv.hasFraction) {
        /* FR-015: Equality with fractional value is always FALSE */
        ret = (Node *) makeBoolConst(false, false);
      } else {
        /* FR-016: Equality with exact integer - transform to native operator */
        Oid nativeOpOid = getNativeOpOid(OP_TYPE_EQ, intType);
        ret = (Node *) buildNativeOpExpr(nativeOpOid, varNode, conv.intVal,
                                         intType, func->location, InvalidOid);
      }
    } else if (opType == OP_TYPE_NE) {
      if (conv.hasFraction) {
        /* Inequality with fractional value is always TRUE */
        ret = (Node *) makeBoolConst(true, false);
      } else {
        /* Transform to native inequality operator */
        Oid nativeOpOid = getNativeOpOid(OP_TYPE_NE, intType);
        ret = (Node *) buildNativeOpExpr(nativeOpOid, varNode, conv.intVal,
                                         intType, func->location, InvalidOid);
      }
    } else {
      /* FR-017: Range boundary transformation */
      int64 adjustedVal;
      bool isAlwaysTrue, isAlwaysFalse;
      Oid nativeOpOid = computeRangeTransform(opType, intType, conv.intVal,
                                              conv.hasFraction, &adjustedVal,
                                              &isAlwaysTrue, &isAlwaysFalse);
      if (isAlwaysTrue) {
        ret = (Node *) makeBoolConst(true, false);
      } else if (isAlwaysFalse) {
        ret = (Node *) makeBoolConst(false, false);
      } else {
        ret = (Node *) buildNativeOpExpr(nativeOpOid, varNode, adjustedVal,
                                         intType, func->location, InvalidOid);
      }
    }
  }

  PG_RETURN_POINTER(ret);
}

/*
 * Core comparison functions (9 total)
 *
 * These functions implement the actual comparison logic, returning:
 *   -1 if left < right
 *    0 if left == right
 *    1 if left > right
 *
 * Implementation converts integer to numeric/float and uses native comparison
 * to avoid calling internal PostgreSQL functions that may not be exported.
 */

/**
 * @brief Compare numeric value with int64 value using direct structure access
 * @param num Numeric value (left operand)
 * @param val int64 value (right operand)
 * @return -1 if num < val, 0 if num == val, 1 if num > val
 *
 * This optimized version avoids int64_to_numeric() and OidFunctionCall overhead
 * by directly inspecting the Numeric structure.
 */
static int
numericCmpInt64Direct(Numeric num, int64 val)
{
  int numSign;
  int valSign;
  int64 numAsInt64;

  /* Handle special values first */
  if (NUM2INT_NUMERIC_IS_SPECIAL(num)) {
    if (NUM2INT_NUMERIC_IS_NAN(num))
      return 1;  /* NaN > everything for consistent ordering */
    if (NUM2INT_NUMERIC_IS_PINF(num))
      return 1;  /* +Inf > any integer */
    /* -Inf */
    return -1;   /* -Inf < any integer */
  }

  /* Get signs for quick comparison */
  numSign = num2int_numeric_sign(num);
  valSign = (val > 0) ? 1 : ((val < 0) ? -1 : 0);

  /* Different signs - easy comparison */
  if (numSign != valSign) {
    if (numSign < valSign)
      return -1;
    // (numSign > valSign)
    return 1;
  }

  /* Both zero */
  if (numSign == 0)
    return 0;

  /* Try direct int64 extraction */
  if (numericToint64(num, &numAsInt64)) {
    /* Successfully converted - simple integer comparison */
    if (numAsInt64 < val)
      return -1;
    if (numAsInt64 > val)
      return 1;
    return 0;
  }

  /*
   * Numeric is either:
   * 1. Out of int64 range (huge), or
   * 2. Has fractional part
   *
   * For case 1: sign already tells us the answer
   * For case 2: we need more careful comparison
   */
  if (num2int_numeric_is_integral(num)) {
    /* Huge integral value - sign determines result */
    return numSign;  /* positive huge > val, negative huge < val */
  }

  /*
   * Has fractional part. Compute floor locally by extracting only integral digits.
   * For negative numbers with fractional part, floor = trunc - 1.
   */
  {
    int weight = NUM2INT_NUMERIC_WEIGHT(num);
    Num2IntNumericDigit *digits = NUM2INT_NUMERIC_DIGITS(num);
    int ndigits = NUM2INT_NUMERIC_NDIGITS(num);
    int integralNdigits;
    int64 floorVal;
    int i;

    /*
     * If weight > 4, the integral part exceeds int64 range.
     * (weight 5 means 10000^5 = 10^20, but int64 max is ~9.2e18)
     * Fall back to sign-based comparison for huge values.
     */
    if (weight > 4)
      return numSign;

    /* Number of integral digits is weight + 1 */
    integralNdigits = weight + 1;
    if (integralNdigits <= 0) {
      /*
       * No integral digits (e.g., 0.5 has weight=-1).
       * floor(positive fraction) = 0, floor(negative fraction) = -1
       */
      floorVal = (numSign > 0) ? 0 : -1;
    } else {
      /* Extract integral portion (padding with zeros if ndigits < integralNdigits) */
      floorVal = 0;
      for (i = 0; i < integralNdigits; i++) {
        Num2IntNumericDigit digit = (i < ndigits) ? digits[i] : 0;

        /* Overflow check for multiplication */
        if (floorVal > PG_INT64_MAX / NUM2INT_NBASE)
          return numSign;  /* Huge value */
        floorVal = floorVal * NUM2INT_NBASE;

        /* Overflow check for addition */
        if (digit > PG_INT64_MAX - floorVal)
          return numSign;  /* Huge value */
        floorVal += digit;
      }

      /* Apply sign */
      if (numSign < 0) {
        /*
         * For negative with fractional part: floor = -trunc - 1
         * e.g., floor(-100.5) = -101, not -100
         *
         * No overflow: floorVal <= PG_INT64_MAX from loop checks, so
         * -floorVal >= -PG_INT64_MAX = PG_INT64_MIN + 1, and
         * -floorVal - 1 >= PG_INT64_MIN (exactly representable).
         */
        floorVal = -floorVal - 1;
      }
    }

    /* Compare floorVal compensated with fractional part with val */
    if (floorVal < val)
      return -1;
    /* (floorVal > val) ... not equal here, that was handled above */
    return 1;
  }
}

/**
 * @brief Check equality of numeric with int64 - optimized for early-out
 * @param num Numeric value
 * @param val int64 value
 * @return true if num == val exactly, false otherwise
 *
 * Optimized equality check that avoids floor extraction for non-equal cases:
 * - NaN/Inf: immediately false (never equal to integers)
 * - Different signs: immediately false
 * - Fractional values: immediately false (can't equal an integer)
 * - Out-of-range: immediately false (extraction fails)
 */
static bool
numericEqInt64Direct(Numeric num, int64 val) {
  int64 numAsInt64;
  int numSign, valSign;

  // NaN/Inf never equal integers
  if (NUM2INT_NUMERIC_IS_SPECIAL(num))
    return false;

  // Sign check - different signs can't be equal
  numSign = num2int_numeric_sign(num);
  valSign = (val > 0) ? 1 : ((val < 0) ? -1 : 0);
  if (numSign != valSign)
    return false;

  // Both zero
  if (numSign == 0)
    return true;

  // Try extraction - fails for fractions and out-of-range → not equal
  if (!numericToint64(num, &numAsInt64))
    return false;

  return numAsInt64 == val;
}

/**
 * @brief Compare numeric value with int2 value
 * @param num Numeric value (left operand)
 * @param val int2 value (right operand)
 * @return -1 if num < val, 0 if num == val, 1 if num > val
 */
int
numeric_cmp_int2_internal(Numeric num, int16 val) {
  return numericCmpInt64Direct(num, (int64)val);
}

/**
 * @brief Compare numeric value with int4 value
 * @param num Numeric value (left operand)
 * @param val int4 value (right operand)
 * @return -1 if num < val, 0 if num == val, 1 if num > val
 */
int
numeric_cmp_int4_internal(Numeric num, int32 val) {
  return numericCmpInt64Direct(num, (int64)val);
}

/**
 * @brief Compare numeric value with int8 value
 */
int
numeric_cmp_int8_internal(Numeric num, int64 val) {
  return numericCmpInt64Direct(num, val);
}

/**
 * @brief Compare float4 value with int2 value
 */
int
float4_cmp_int2_internal(float4 fval, int16 ival) {
  float4 ivalAsFloat4;

  /* Handle NaN - NaN never equals anything */
  if (isnan(fval)) {
    return 1;  /* Treat NaN as greater for consistent ordering */
  }

  /* Handle infinity */
  if (isinf(fval)) {
    return (fval > 0) ? 1 : -1;
  }

  /* Convert integer to float4 and compare */
  ivalAsFloat4 = (float4)ival;

  /* Check if representation lost precision */
  if ((int16)ivalAsFloat4 != ival) {
    /* This shouldn't happen for int2 values, but handle it */
    return (fval < ival) ? -1 : 1;
  }

  if (fval < ivalAsFloat4) return -1;
  if (fval > ivalAsFloat4) return 1;
  return 0;
}

/**
 * @brief Compare float4 value with int4 value
 */
int
float4_cmp_int4_internal(float4 fval, int32 ival) {
  float4 ivalAsFloat4;

  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;

  ivalAsFloat4 = (float4)ival;

  /* Check if integer exceeds float4 precision (>2^24) */
  if (labs((long)ival) > 16777216L) {
    /* Check if round-trip conversion preserves value */
    if ((int32)ivalAsFloat4 != ival) {
      /* Precision lost - can't be equal */
      /* Compare based on which direction the rounding went */
      if (fval < ivalAsFloat4) return -1;
      if (fval > ivalAsFloat4) return 1;
      /* If fval == ivalAsFloat4, they still can't be truly equal since ivalAsFloat4 != ival */
      return (ivalAsFloat4 < ival) ? -1 : 1;
    }
  }

  if (fval < ivalAsFloat4) return -1;
  if (fval > ivalAsFloat4) return 1;
  return 0;
}

/**
 * @brief Compare float4 value with int8 value
 */
int
float4_cmp_int8_internal(float4 fval, int64 ival) {
  float4 ivalAsFloat4;

  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;

  ivalAsFloat4 = (float4)ival;

  /* Check if integer exceeds float4 precision */
  if (llabs(ival) > 16777216LL) {
    if ((int64)ivalAsFloat4 != ival) {
      if (fval < ivalAsFloat4) return -1;
      if (fval > ivalAsFloat4) return 1;
      return (ivalAsFloat4 < ival) ? -1 : 1;
    }
  }

  if (fval < ivalAsFloat4) return -1;
  if (fval > ivalAsFloat4) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int2 value
 */
int
float8_cmp_int2_internal(float8 fval, int16 ival) {
  float8 ivalAsFloat8;

  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;

  ivalAsFloat8 = (float8)ival;

  if (fval < ivalAsFloat8) return -1;
  if (fval > ivalAsFloat8) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int4 value
 */
int
float8_cmp_int4_internal(float8 fval, int32 ival) {
  float8 ivalAsFloat8;

  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;

  ivalAsFloat8 = (float8)ival;

  if (fval < ivalAsFloat8) return -1;
  if (fval > ivalAsFloat8) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int8 value
 */
int
float8_cmp_int8_internal(float8 fval, int64 ival) {
  float8 ivalAsFloat8;
  float8 fvalRounded;

  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;

  /* Check if float has fractional part - never equals integer */
  fvalRounded = trunc(fval);
  if (fval != fvalRounded) {
    return (fval < ival) ? -1 : 1;
  }

  ivalAsFloat8 = (float8)ival;

  /* float8 has 53-bit mantissa, check precision beyond 2^53 */
  if (llabs(ival) > 9007199254740992LL) {
    /* Integer is beyond exact float8 representation */
    if ((int64)ivalAsFloat8 != ival) {
      /* Integer cannot be exactly represented as float8 */
      /* For equality, this means they can never be equal */
      if (fval < ivalAsFloat8) return -1;
      if (fval > ivalAsFloat8) return 1;
      /* fval == ivalAsFloat8, but ivalAsFloat8 != ival due to rounding */
      /* Therefore fval != ival (precision loss detected) */
      return (ivalAsFloat8 < ival) ? -1 : 1;
    }
  }

  if (fval < ivalAsFloat8) return -1;
  if (fval > ivalAsFloat8) return 1;
  return 0;
}

/*
 * Btree support function wrappers
 *
 * These SQL-callable wrappers expose the internal comparison functions
 * for btree operator family support function registration.
 * Each returns int4: -1 for <, 0 for =, 1 for >
 */

PG_FUNCTION_INFO_V1(numeric_cmp_int2);
Datum
numeric_cmp_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  PG_RETURN_INT32(numeric_cmp_int2_internal(num, val));
}

PG_FUNCTION_INFO_V1(numeric_cmp_int4);
Datum
numeric_cmp_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  PG_RETURN_INT32(numeric_cmp_int4_internal(num, val));
}

PG_FUNCTION_INFO_V1(numeric_cmp_int8);
Datum
numeric_cmp_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  PG_RETURN_INT32(numeric_cmp_int8_internal(num, val));
}

/*
 * Reverse comparison functions for int <op> numeric
 * These are needed for the integer_ops btree family where the integer
 * is the left (indexed) operand.
 * int_cmp_numeric(i, n) = -numeric_cmp_int(n, i)
 */
PG_FUNCTION_INFO_V1(int2_cmp_numeric);
Datum
int2_cmp_numeric(PG_FUNCTION_ARGS) {
  int16 val = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_INT32(-numeric_cmp_int2_internal(num, val));
}

PG_FUNCTION_INFO_V1(int4_cmp_numeric);
Datum
int4_cmp_numeric(PG_FUNCTION_ARGS) {
  int32 val = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_INT32(-numeric_cmp_int4_internal(num, val));
}

PG_FUNCTION_INFO_V1(int8_cmp_numeric);
Datum
int8_cmp_numeric(PG_FUNCTION_ARGS) {
  int64 val = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_INT32(-numeric_cmp_int8_internal(num, val));
}

PG_FUNCTION_INFO_V1(float4_cmp_int2);
Datum
float4_cmp_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  PG_RETURN_INT32(float4_cmp_int2_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(float4_cmp_int4);
Datum
float4_cmp_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  PG_RETURN_INT32(float4_cmp_int4_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(float4_cmp_int8);
Datum
float4_cmp_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  PG_RETURN_INT32(float4_cmp_int8_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(float8_cmp_int2);
Datum
float8_cmp_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  PG_RETURN_INT32(float8_cmp_int2_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(float8_cmp_int4);
Datum
float8_cmp_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  PG_RETURN_INT32(float8_cmp_int4_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(float8_cmp_int8);
Datum
float8_cmp_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  PG_RETURN_INT32(float8_cmp_int8_internal(fval, ival));
}

/*
 * Reverse comparison functions for btree operator families
 *
 * These functions provide int x float comparisons by calling the existing
 * float x int comparison functions with swapped arguments and negated result.
 */

PG_FUNCTION_INFO_V1(int2_cmp_float4);
Datum
int2_cmp_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  /* Reverse the comparison: int2_cmp_float4(i, f) = -float4_cmp_int2(f, i) */
  PG_RETURN_INT32(-float4_cmp_int2_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(int4_cmp_float4);
Datum
int4_cmp_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  /* Reverse the comparison: int4_cmp_float4(i, f) = -float4_cmp_int4(f, i) */
  PG_RETURN_INT32(-float4_cmp_int4_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(int8_cmp_float4);
Datum
int8_cmp_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  /* Reverse the comparison: int8_cmp_float4(i, f) = -float4_cmp_int8(f, i) */
  PG_RETURN_INT32(-float4_cmp_int8_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(int2_cmp_float8);
Datum
int2_cmp_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  /* Reverse the comparison: int2_cmp_float8(i, f) = -float8_cmp_int2(f, i) */
  PG_RETURN_INT32(-float8_cmp_int2_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(int4_cmp_float8);
Datum
int4_cmp_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  /* Reverse the comparison: int4_cmp_float8(i, f) = -float8_cmp_int4(f, i) */
  PG_RETURN_INT32(-float8_cmp_int4_internal(fval, ival));
}

PG_FUNCTION_INFO_V1(int8_cmp_float8);
Datum
int8_cmp_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  /* Reverse the comparison: int8_cmp_float8(i, f) = -float8_cmp_int8(f, i) */
  PG_RETURN_INT32(-float8_cmp_int8_internal(fval, ival));
}

/*
 * Equality operator wrappers (=)
 *
 * These wrapper functions call the corresponding _cmp_internal function
 * and test for equality (result == 0).
 */

PG_FUNCTION_INFO_V1(numeric_eq_int2);
Datum
numeric_eq_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(numeric_eq_int4);
Datum
numeric_eq_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(numeric_eq_int8);
Datum
numeric_eq_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, val));
}

PG_FUNCTION_INFO_V1(float4_eq_int2);
Datum
float4_eq_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(float4_eq_int4);
Datum
float4_eq_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(float4_eq_int8);
Datum
float4_eq_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(float8_eq_int2);
Datum
float8_eq_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(float8_eq_int4);
Datum
float8_eq_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(float8_eq_int8);
Datum
float8_eq_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

/*
 * Inequality operator wrappers (<>)
 */

PG_FUNCTION_INFO_V1(numeric_ne_int2);
Datum
numeric_ne_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(numeric_ne_int4);
Datum
numeric_ne_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(numeric_ne_int8);
Datum
numeric_ne_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, val));
}

PG_FUNCTION_INFO_V1(float4_ne_int2);
Datum
float4_ne_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(float4_ne_int4);
Datum
float4_ne_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(float4_ne_int8);
Datum
float4_ne_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(float8_ne_int2);
Datum
float8_ne_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(float8_ne_int4);
Datum
float8_ne_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(float8_ne_int8);
Datum
float8_ne_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

/*
 * Commutator wrappers for reverse-direction operators (int X = numeric/float)
 * These simply swap arguments and call the main functions
 */

/* int2 = numeric/float */
PG_FUNCTION_INFO_V1(int2_eq_numeric);
Datum
int2_eq_numeric(PG_FUNCTION_ARGS) {
  int16 val = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(int2_eq_float4);
Datum
int2_eq_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(int2_eq_float8);
Datum
int2_eq_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

/* int4 = numeric/float */
PG_FUNCTION_INFO_V1(int4_eq_numeric);
Datum
int4_eq_numeric(PG_FUNCTION_ARGS) {
  int32 val = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(int4_eq_float4);
Datum
int4_eq_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(int4_eq_float8);
Datum
int4_eq_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

/* int8 = numeric/float */
PG_FUNCTION_INFO_V1(int8_eq_numeric);
Datum
int8_eq_numeric(PG_FUNCTION_ARGS) {
  int64 val = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(numericEqInt64Direct(num, val));
}

PG_FUNCTION_INFO_V1(int8_eq_float4);
Datum
int8_eq_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(int8_eq_float8);
Datum
int8_eq_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp == 0);
}

/* int X <> numeric/float commutators */
PG_FUNCTION_INFO_V1(int2_ne_numeric);
Datum
int2_ne_numeric(PG_FUNCTION_ARGS) {
  int16 val = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(int2_ne_float4);
Datum
int2_ne_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(int2_ne_float8);
Datum
int2_ne_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(int4_ne_numeric);
Datum
int4_ne_numeric(PG_FUNCTION_ARGS) {
  int32 val = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, (int64)val));
}

PG_FUNCTION_INFO_V1(int4_ne_float4);
Datum
int4_ne_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(int4_ne_float8);
Datum
int4_ne_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(int8_ne_numeric);
Datum
int8_ne_numeric(PG_FUNCTION_ARGS) {
  int64 val = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  PG_RETURN_BOOL(!numericEqInt64Direct(num, val));
}

PG_FUNCTION_INFO_V1(int8_ne_float4);
Datum
int8_ne_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(int8_ne_float8);
Datum
int8_ne_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp != 0);
}

/*
 * Less than operator wrappers (<)
 */

PG_FUNCTION_INFO_V1(numeric_lt_int2);
Datum
numeric_lt_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(numeric_lt_int4);
Datum
numeric_lt_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(numeric_lt_int8);
Datum
numeric_lt_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float4_lt_int2);
Datum
float4_lt_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float4_lt_int4);
Datum
float4_lt_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float4_lt_int8);
Datum
float4_lt_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float8_lt_int2);
Datum
float8_lt_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float8_lt_int4);
Datum
float8_lt_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(float8_lt_int8);
Datum
float8_lt_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

/*
 * Greater than operator wrappers (>)
 */

PG_FUNCTION_INFO_V1(numeric_gt_int2);
Datum
numeric_gt_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(numeric_gt_int4);
Datum
numeric_gt_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(numeric_gt_int8);
Datum
numeric_gt_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float4_gt_int2);
Datum
float4_gt_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float4_gt_int4);
Datum
float4_gt_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float4_gt_int8);
Datum
float4_gt_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float8_gt_int2);
Datum
float8_gt_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float8_gt_int4);
Datum
float8_gt_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(float8_gt_int8);
Datum
float8_gt_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

/*
 * Less than or equal operator wrappers (<=)
 */

PG_FUNCTION_INFO_V1(numeric_le_int2);
Datum
numeric_le_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(numeric_le_int4);
Datum
numeric_le_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(numeric_le_int8);
Datum
numeric_le_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float4_le_int2);
Datum
float4_le_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float4_le_int4);
Datum
float4_le_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float4_le_int8);
Datum
float4_le_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float8_le_int2);
Datum
float8_le_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float8_le_int4);
Datum
float8_le_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(float8_le_int8);
Datum
float8_le_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

/*
 * Greater than or equal operator wrappers (>=)
 */

PG_FUNCTION_INFO_V1(numeric_ge_int2);
Datum
numeric_ge_int2(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int16 val = PG_GETARG_INT16(1);
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(numeric_ge_int4);
Datum
numeric_ge_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(numeric_ge_int8);
Datum
numeric_ge_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float4_ge_int2);
Datum
float4_ge_int2(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float4_ge_int4);
Datum
float4_ge_int4(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float4_ge_int8);
Datum
float4_ge_int8(PG_FUNCTION_ARGS) {
  float4 fval = PG_GETARG_FLOAT4(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float8_ge_int2);
Datum
float8_ge_int2(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int16 ival = PG_GETARG_INT16(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float8_ge_int4);
Datum
float8_ge_int4(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int32 ival = PG_GETARG_INT32(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(float8_ge_int8);
Datum
float8_ge_int8(PG_FUNCTION_ARGS) {
  float8 fval = PG_GETARG_FLOAT8(0);
  int64 ival = PG_GETARG_INT64(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

/*
 * Commutator range operator wrappers (int X op numeric/float)
 * These reverse the operands and flip the comparison operator.
 * For example: int2 < numeric is implemented as numeric > int2
 */

/* int2 < numeric/float operators */
PG_FUNCTION_INFO_V1(int2_lt_numeric);
Datum
int2_lt_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int2_internal(num, ival);
  PG_RETURN_BOOL(cmp > 0);  /* numeric > int2 means int2 < numeric */
}

PG_FUNCTION_INFO_V1(int2_lt_float4);
Datum
int2_lt_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int2_lt_float8);
Datum
int2_lt_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int4_lt_numeric);
Datum
int4_lt_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int4_internal(num, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int4_lt_float4);
Datum
int4_lt_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int4_lt_float8);
Datum
int4_lt_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int8_lt_numeric);
Datum
int8_lt_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int8_internal(num, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int8_lt_float4);
Datum
int8_lt_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

PG_FUNCTION_INFO_V1(int8_lt_float8);
Datum
int8_lt_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp > 0);
}

/* int2/4/8 > numeric/float operators */
PG_FUNCTION_INFO_V1(int2_gt_numeric);
Datum
int2_gt_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int2_internal(num, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int2_gt_float4);
Datum
int2_gt_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int2_gt_float8);
Datum
int2_gt_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int4_gt_numeric);
Datum
int4_gt_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int4_internal(num, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int4_gt_float4);
Datum
int4_gt_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int4_gt_float8);
Datum
int4_gt_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int8_gt_numeric);
Datum
int8_gt_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int8_internal(num, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int8_gt_float4);
Datum
int8_gt_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

PG_FUNCTION_INFO_V1(int8_gt_float8);
Datum
int8_gt_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp < 0);
}

/* int2/4/8 <= numeric/float operators */
PG_FUNCTION_INFO_V1(int2_le_numeric);
Datum
int2_le_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int2_internal(num, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int2_le_float4);
Datum
int2_le_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int2_le_float8);
Datum
int2_le_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int4_le_numeric);
Datum
int4_le_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int4_internal(num, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int4_le_float4);
Datum
int4_le_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int4_le_float8);
Datum
int4_le_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int8_le_numeric);
Datum
int8_le_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int8_internal(num, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int8_le_float4);
Datum
int8_le_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

PG_FUNCTION_INFO_V1(int8_le_float8);
Datum
int8_le_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp >= 0);
}

/* int2/4/8 >= numeric/float operators */
PG_FUNCTION_INFO_V1(int2_ge_numeric);
Datum
int2_ge_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int2_internal(num, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int2_ge_float4);
Datum
int2_ge_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int2_ge_float8);
Datum
int2_ge_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int2_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int4_ge_numeric);
Datum
int4_ge_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int4_internal(num, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int4_ge_float4);
Datum
int4_ge_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int4_ge_float8);
Datum
int4_ge_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int4_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int8_ge_numeric);
Datum
int8_ge_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  int cmp = numeric_cmp_int8_internal(num, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int8_ge_float4);
Datum
int8_ge_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = PG_GETARG_FLOAT4(1);
  int cmp = float4_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

PG_FUNCTION_INFO_V1(int8_ge_float8);
Datum
int8_ge_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = PG_GETARG_FLOAT8(1);
  int cmp = float8_cmp_int8_internal(fval, ival);
  PG_RETURN_BOOL(cmp <= 0);
}

/* ============================================================================
 * Hash Functions for Cross-Type Equality
 * ============================================================================
 * These wrapper functions ensure that equal values hash consistently across
 * types. For example, 10::int4 and 10.0::numeric must produce the same hash.
 *
 * Strategy: Compute the same hash that hash_numeric would produce, but
 * without allocating a Numeric structure. We build the base-10000 digit
 * representation on the stack and hash it directly.
 *
 * hash_numeric algorithm:
 *   1. Strip leading/trailing zero digits
 *   2. hash_any() on the remaining digit array
 *   3. XOR in the adjusted weight
 *
 * For int64, we need at most 5 base-10000 digits (10000^5 > 2^63).
 */

// Compute hash for int64 matching hash_numeric's output
// Returns the hash value directly (no palloc)
static Datum
hash_int64_as_numeric_internal(int64 val)
{
  Num2IntNumericDigit digits[5];  // max 5 digits for int64 in base-10000
  int ndigits = 0;
  int weight;
  uint64 uval;
  Datum digit_hash;
  int i;

  // Handle zero specially - hash_numeric returns -1 for zero
  if (val == 0)
    return UInt32GetDatum((uint32) -1);

  // Work with absolute value
  if (val < 0) {
    // Handle PG_INT64_MIN carefully: -PG_INT64_MIN overflows
    if (val == PG_INT64_MIN)
      uval = ((uint64) PG_INT64_MAX) + 1;
    else
      uval = (uint64) (-val);
  } else {
    uval = (uint64) val;
  }

  // Extract base-10000 digits (least significant first)
  while (uval > 0) {
    digits[ndigits++] = (Num2IntNumericDigit)(uval % NUM2INT_NBASE);
    uval /= NUM2INT_NBASE;
  }

  // Reverse to get most significant first (as Numeric stores them)
  for (i = 0; i < ndigits / 2; i++) {
    Num2IntNumericDigit tmp = digits[i];
    digits[i] = digits[ndigits - 1 - i];
    digits[ndigits - 1 - i] = tmp;
  }

  // Weight is ndigits - 1 (position of most significant digit)
  weight = ndigits - 1;

  // Strip trailing zeros (hash_numeric does this)
  while (ndigits > 0 && digits[ndigits - 1] == 0)
    ndigits--;

  // Hash the digit array and XOR with weight
  digit_hash = hash_any((unsigned char *) digits,
                        ndigits * sizeof(Num2IntNumericDigit));
  return UInt32GetDatum(DatumGetUInt32(digit_hash) ^ weight);
}

// Extended version with seed
static Datum
hash_int64_as_numeric_extended_internal(int64 val, uint64 seed)
{
  Num2IntNumericDigit digits[5];
  int ndigits = 0;
  int weight;
  uint64 uval;
  Datum digit_hash;
  int i;

  // Handle zero - hash_numeric_extended returns seed - 1 for zero
  if (val == 0)
    return UInt64GetDatum(seed - 1);

  // Work with absolute value
  if (val < 0) {
    if (val == PG_INT64_MIN)
      uval = ((uint64) PG_INT64_MAX) + 1;
    else
      uval = (uint64) (-val);
  } else {
    uval = (uint64) val;
  }

  // Extract base-10000 digits (least significant first)
  while (uval > 0) {
    digits[ndigits++] = (Num2IntNumericDigit)(uval % NUM2INT_NBASE);
    uval /= NUM2INT_NBASE;
  }

  // Reverse to most significant first
  for (i = 0; i < ndigits / 2; i++) {
    Num2IntNumericDigit tmp = digits[i];
    digits[i] = digits[ndigits - 1 - i];
    digits[ndigits - 1 - i] = tmp;
  }

  weight = ndigits - 1;

  // Strip trailing zeros
  while (ndigits > 0 && digits[ndigits - 1] == 0)
    ndigits--;

  digit_hash = hash_any_extended((unsigned char *) digits,
                                 ndigits * sizeof(Num2IntNumericDigit),
                                 seed);
  return UInt64GetDatum(DatumGetUInt64(digit_hash) ^ weight);
}

/* Numeric hash wrappers - compute hash matching hash_numeric without palloc */
PG_FUNCTION_INFO_V1(hash_int2_as_numeric);
Datum
hash_int2_as_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  return hash_int64_as_numeric_internal((int64) ival);
}

PG_FUNCTION_INFO_V1(hash_int2_as_numeric_extended);
Datum
hash_int2_as_numeric_extended(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  uint64 seed = PG_GETARG_INT64(1);
  return hash_int64_as_numeric_extended_internal((int64) ival, seed);
}

PG_FUNCTION_INFO_V1(hash_int4_as_numeric);
Datum
hash_int4_as_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  return hash_int64_as_numeric_internal((int64) ival);
}

PG_FUNCTION_INFO_V1(hash_int4_as_numeric_extended);
Datum
hash_int4_as_numeric_extended(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  uint64 seed = PG_GETARG_INT64(1);
  return hash_int64_as_numeric_extended_internal((int64) ival, seed);
}

PG_FUNCTION_INFO_V1(hash_int8_as_numeric);
Datum
hash_int8_as_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  return hash_int64_as_numeric_internal(ival);
}

PG_FUNCTION_INFO_V1(hash_int8_as_numeric_extended);
Datum
hash_int8_as_numeric_extended(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  uint64 seed = PG_GETARG_INT64(1);
  return hash_int64_as_numeric_extended_internal(ival, seed);
}

/* Float hash wrappers - cast int to float, then hash */
PG_FUNCTION_INFO_V1(hash_int2_as_float4);
Datum
hash_int2_as_float4(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float4 fval = (float4) ival;
  return DirectFunctionCall1(hashfloat4, Float4GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int2_as_float4_extended);
Datum
hash_int2_as_float4_extended(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  int64 seed = PG_GETARG_INT64(1);
  float4 fval = (float4) ival;
  return DirectFunctionCall2(hashfloat4extended, Float4GetDatum(fval), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int4_as_float4);
Datum
hash_int4_as_float4(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float4 fval = (float4) ival;
  return DirectFunctionCall1(hashfloat4, Float4GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int4_as_float4_extended);
Datum
hash_int4_as_float4_extended(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  int64 seed = PG_GETARG_INT64(1);
  float4 fval = (float4) ival;
  return DirectFunctionCall2(hashfloat4extended, Float4GetDatum(fval), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int8_as_float4);
Datum
hash_int8_as_float4(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float4 fval = (float4) ival;
  return DirectFunctionCall1(hashfloat4, Float4GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int8_as_float4_extended);
Datum
hash_int8_as_float4_extended(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  int64 seed = PG_GETARG_INT64(1);
  float4 fval = (float4) ival;
  return DirectFunctionCall2(hashfloat4extended, Float4GetDatum(fval), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int2_as_float8);
Datum
hash_int2_as_float8(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  float8 fval = (float8) ival;
  return DirectFunctionCall1(hashfloat8, Float8GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int2_as_float8_extended);
Datum
hash_int2_as_float8_extended(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  int64 seed = PG_GETARG_INT64(1);
  float8 fval = (float8) ival;
  return DirectFunctionCall2(hashfloat8extended, Float8GetDatum(fval), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int4_as_float8);
Datum
hash_int4_as_float8(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  float8 fval = (float8) ival;
  return DirectFunctionCall1(hashfloat8, Float8GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int4_as_float8_extended);
Datum
hash_int4_as_float8_extended(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  int64 seed = PG_GETARG_INT64(1);
  float8 fval = (float8) ival;
  return DirectFunctionCall2(hashfloat8extended, Float8GetDatum(fval), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int8_as_float8);
Datum
hash_int8_as_float8(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  float8 fval = (float8) ival;
  return DirectFunctionCall1(hashfloat8, Float8GetDatum(fval));
}

PG_FUNCTION_INFO_V1(hash_int8_as_float8_extended);
Datum
hash_int8_as_float8_extended(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  int64 seed = PG_GETARG_INT64(1);
  float8 fval = (float8) ival;
  return DirectFunctionCall2(hashfloat8extended, Float8GetDatum(fval), Int64GetDatum(seed));
}

