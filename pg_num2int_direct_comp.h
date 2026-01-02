/*
 * Copyright (c) 2026 dataStone Inc.
 *
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

/**
 * @file pg_num2int_direct_comp.h
 * @brief Header file for exact comparison operators between numeric and integer types
 * @author Dave Sharpe
 * @date 2025-12-23
 *
 * This file was developed with assistance from AI tools.
 *
 * Provides function declarations and structure definitions for the
 * pg_num2int_direct_comp PostgreSQL extension, which implements exact
 * comparison operators between inexact numeric types (numeric, float4, float8)
 * and integer types (int2, int4, int8).
 */

#ifndef PG_NUM2INT_DIRECT_COMP_H
#define PG_NUM2INT_DIRECT_COMP_H

#include "postgres.h"
#include "fmgr.h"
#include "utils/numeric.h"
#include "utils/float.h"
#include "optimizer/optimizer.h"
#include "nodes/supportnodes.h"

/*
 * ============================================================================
 * PostgreSQL Built-in OIDs and Internal Structures
 * ============================================================================
 *
 * This header redefines PostgreSQL built-in OIDs and internal Numeric format
 * structures with a NUM2INT_ prefix to avoid symbol collisions.
 *
 * STABILITY: PostgreSQL built-in OIDs are never changed or reused across
 * versions. The Numeric on-disk format has been stable since PostgreSQL 8.3.
 *
 * TESTING: All definitions are validated by regression tests in
 * extension_lifecycle.sql. When adding new symbols, add corresponding tests.
 */

/* PostgreSQL Built-in Function OIDs
 *
 * These OIDs are stable across PostgreSQL versions and represent the
 * built-in cast functions from numeric to integer types. These functions
 * are defined in the PostgreSQL system catalog and perform standard
 * banker's rounding during conversion.
 *
 * PostgreSQL does not define symbolic constants for these in pg_proc_d.h,
 * so we define them here with NUM2INT_ prefix to avoid potential conflicts.
 *
 * Sources: pg_proc system catalog, defined in PostgreSQL source:
 *   src/include/catalog/pg_proc.dat
 * These OIDs are stable across all supported PostgreSQL versions (12+).
 * Built-in OIDs are never changed or reused; doing so would break the
 * PostgreSQL ecosystem.
 */
#define NUM2INT_CAST_NUMERIC_INT2  1783  /* int2(numeric) - cast numeric to smallint */
#define NUM2INT_CAST_NUMERIC_INT4  1744  /* int4(numeric) - cast numeric to integer */
#define NUM2INT_CAST_NUMERIC_INT8  1779  /* int8(numeric) - cast numeric to bigint */
#define NUM2INT_NUMERIC_TRUNC      1710  /* trunc(numeric) - truncate toward zero */
#define NUM2INT_NUMERIC_FLOOR      1712  /* floor(numeric) - round toward negative infinity */
#define NUM2INT_NUMERIC_EQ         1718  /* numeric_eq(numeric,numeric) - equality comparison */
#define NUM2INT_NUMERIC_CMP        1769  /* numeric_cmp(numeric,numeric) - comparison function */

/*
 * PostgreSQL Standard Btree Operator OIDs
 *
 * These OIDs represent the built-in comparison operators for integer types.
 * They are used when transforming index conditions in the support function,
 * allowing the query planner to use existing btree indexes on integer columns.
 *
 * PostgreSQL does not define symbolic constants for these in pg_operator_d.h
 * (which only defines OID_*_OP for special operators), so we define them here
 * with NUM2INT_ prefix to avoid potential conflicts with future PostgreSQL versions.
 *
 * Sources: pg_operator system catalog, defined in PostgreSQL source:
 *   src/include/catalog/pg_operator.dat
 * These OIDs are stable across all supported PostgreSQL versions (12+).
 * Built-in OIDs are never changed or reused; doing so would break the
 * PostgreSQL ecosystem.
 *
 * Operator Naming Convention:
 *   NUM2INT_INT<bits><op>_OID where:
 *     <bits> = 2 (int2/smallint), 4 (int4/integer), 8 (int8/bigint)
 *     <op> = EQ (=), NE (<>), LT (<), GT (>), LE (<=), GE (>=)
 */

/* int2 (smallint) comparison operators */
#define NUM2INT_INT2EQ_OID   94   /* int2 = int2 */
#define NUM2INT_INT2LT_OID   95   /* int2 < int2 */
#define NUM2INT_INT2GT_OID  520   /* int2 > int2 */
#define NUM2INT_INT2LE_OID  522   /* int2 <= int2 */
#define NUM2INT_INT2GE_OID  524   /* int2 >= int2 */
#define NUM2INT_INT2NE_OID  519   /* int2 <> int2 */

/* int4 (integer) comparison operators */
#define NUM2INT_INT4EQ_OID   96   /* int4 = int4 */
#define NUM2INT_INT4LT_OID   97   /* int4 < int4 */
#define NUM2INT_INT4GT_OID  521   /* int4 > int4 */
#define NUM2INT_INT4LE_OID  523   /* int4 <= int4 */
#define NUM2INT_INT4GE_OID  525   /* int4 >= int4 */
#define NUM2INT_INT4NE_OID  518   /* int4 <> int4 */

/* int8 (bigint) comparison operators */
#define NUM2INT_INT8EQ_OID  410   /* int8 = int8 */
#define NUM2INT_INT8LT_OID  412   /* int8 < int8 */
#define NUM2INT_INT8GT_OID  413   /* int8 > int8 */
#define NUM2INT_INT8LE_OID  414   /* int8 <= int8 */
#define NUM2INT_INT8GE_OID  415   /* int8 >= int8 */
#define NUM2INT_INT8NE_OID  411   /* int8 <> int8 */

/*
 * ============================================================================
 * Internal Numeric Format Definitions
 * ============================================================================
 *
 * These definitions are copied from PostgreSQL's src/backend/utils/adt/numeric.c
 * to enable direct inspection of Numeric values without function call overhead.
 * The Numeric on-disk format has been stable since PostgreSQL 8.3 (2008).
 *
 * WARNING: These definitions must match the PostgreSQL version being compiled
 * against. They are verified at compile time via static assertions where possible.
 *
 * All symbols use NUM2INT_ prefix to avoid collisions with internal PostgreSQL
 * symbols that may be exposed in future versions.
 */

/* Numeric header flag bits */
#define NUM2INT_NUMERIC_SIGN_MASK   0xC000
#define NUM2INT_NUMERIC_POS         0x0000
#define NUM2INT_NUMERIC_NEG         0x4000
#define NUM2INT_NUMERIC_SHORT       0x8000
#define NUM2INT_NUMERIC_SPECIAL     0xC000

/* Special value encodings (NaN, +Inf, -Inf) */
#define NUM2INT_NUMERIC_EXT_SIGN_MASK   0xF000
#define NUM2INT_NUMERIC_NAN             0xC000
#define NUM2INT_NUMERIC_PINF            0xD000
#define NUM2INT_NUMERIC_NINF            0xF000
#define NUM2INT_NUMERIC_INF_SIGN_MASK   0x2000

/* Short format field masks */
#define NUM2INT_NUMERIC_SHORT_SIGN_MASK         0x2000
#define NUM2INT_NUMERIC_SHORT_DSCALE_MASK       0x1F80
#define NUM2INT_NUMERIC_SHORT_DSCALE_SHIFT      7
#define NUM2INT_NUMERIC_SHORT_WEIGHT_SIGN_MASK  0x0040
#define NUM2INT_NUMERIC_SHORT_WEIGHT_MASK       0x003F

/* Long format field mask */
#define NUM2INT_NUMERIC_DSCALE_MASK     0x3FFF

/* Base and digit definitions */
#define NUM2INT_DEC_DIGITS  4
#define NUM2INT_NBASE       10000
typedef int16 Num2IntNumericDigit;

/*
 * Internal Numeric structure definitions
 * These mirror the private NumericChoice/NumericData in numeric.c
 */
typedef struct Num2IntNumericShort {
    uint16      n_header;       /* Sign + dscale + weight */
    Num2IntNumericDigit n_data[FLEXIBLE_ARRAY_MEMBER];
} Num2IntNumericShort;

typedef struct Num2IntNumericLong {
    uint16      n_sign_dscale;  /* Sign + dscale */
    int16       n_weight;       /* Weight of first digit */
    Num2IntNumericDigit n_data[FLEXIBLE_ARRAY_MEMBER];
} Num2IntNumericLong;

typedef union Num2IntNumericChoice {
    uint16      n_header;       /* Header word for quick type checking */
    Num2IntNumericShort n_short;
    Num2IntNumericLong  n_long;
} Num2IntNumericChoice;

typedef struct Num2IntNumericData {
    int32       vl_len_;        /* varlena header (do not touch directly!) */
    Num2IntNumericChoice choice;
} Num2IntNumericData;

/*
 * Macros to extract header flags and detect format/special values
 */
#define NUM2INT_NUMERIC_FLAGBITS(n) \
    (((Num2IntNumericData *)(n))->choice.n_header & NUM2INT_NUMERIC_SIGN_MASK)

#define NUM2INT_NUMERIC_IS_SHORT(n) \
    (NUM2INT_NUMERIC_FLAGBITS(n) == NUM2INT_NUMERIC_SHORT)

#define NUM2INT_NUMERIC_IS_SPECIAL(n) \
    (NUM2INT_NUMERIC_FLAGBITS(n) == NUM2INT_NUMERIC_SPECIAL)

#define NUM2INT_NUMERIC_IS_NAN(n) \
    (((Num2IntNumericData *)(n))->choice.n_header == NUM2INT_NUMERIC_NAN)

#define NUM2INT_NUMERIC_IS_PINF(n) \
    (((Num2IntNumericData *)(n))->choice.n_header == NUM2INT_NUMERIC_PINF)

#define NUM2INT_NUMERIC_IS_NINF(n) \
    (((Num2IntNumericData *)(n))->choice.n_header == NUM2INT_NUMERIC_NINF)

#define NUM2INT_NUMERIC_IS_INF(n) \
    ((((Num2IntNumericData *)(n))->choice.n_header & ~NUM2INT_NUMERIC_INF_SIGN_MASK) == NUM2INT_NUMERIC_PINF)

/*
 * Extract sign from packed Numeric
 * Returns NUM2INT_NUMERIC_POS, NUM2INT_NUMERIC_NEG, or a special value flag
 */
#define NUM2INT_NUMERIC_SIGN(n) \
    (NUM2INT_NUMERIC_IS_SHORT(n) ? \
        ((((Num2IntNumericData *)(n))->choice.n_short.n_header & NUM2INT_NUMERIC_SHORT_SIGN_MASK) ? \
            NUM2INT_NUMERIC_NEG : NUM2INT_NUMERIC_POS) : \
        (NUM2INT_NUMERIC_IS_SPECIAL(n) ? \
            (((Num2IntNumericData *)(n))->choice.n_header & NUM2INT_NUMERIC_EXT_SIGN_MASK) : \
            (((Num2IntNumericData *)(n))->choice.n_long.n_sign_dscale & NUM2INT_NUMERIC_SIGN_MASK)))

/*
 * Extract display scale (number of digits after decimal point)
 */
#define NUM2INT_NUMERIC_DSCALE(n) \
    (NUM2INT_NUMERIC_IS_SHORT(n) ? \
        ((((Num2IntNumericData *)(n))->choice.n_short.n_header & NUM2INT_NUMERIC_SHORT_DSCALE_MASK) \
            >> NUM2INT_NUMERIC_SHORT_DSCALE_SHIFT) : \
        (((Num2IntNumericData *)(n))->choice.n_long.n_sign_dscale & NUM2INT_NUMERIC_DSCALE_MASK))

/*
 * Extract weight of first digit (position relative to decimal point)
 * Weight 0 means first digit is units, 1 means first digit is thousands, etc.
 */
#define NUM2INT_NUMERIC_WEIGHT(n) \
    (NUM2INT_NUMERIC_IS_SHORT(n) ? \
        ((((Num2IntNumericData *)(n))->choice.n_short.n_header & NUM2INT_NUMERIC_SHORT_WEIGHT_SIGN_MASK) ? \
            (~NUM2INT_NUMERIC_SHORT_WEIGHT_MASK | \
             (((Num2IntNumericData *)(n))->choice.n_short.n_header & NUM2INT_NUMERIC_SHORT_WEIGHT_MASK)) : \
            (((Num2IntNumericData *)(n))->choice.n_short.n_header & NUM2INT_NUMERIC_SHORT_WEIGHT_MASK)) : \
        (((Num2IntNumericData *)(n))->choice.n_long.n_weight))

/*
 * Header size depends on format (short vs long)
 */
#define NUM2INT_NUMERIC_HEADER_IS_SHORT(n) \
    ((((Num2IntNumericData *)(n))->choice.n_header & NUM2INT_NUMERIC_SHORT) != 0)

#define NUM2INT_NUMERIC_HEADER_SIZE(n) \
    (VARHDRSZ + sizeof(uint16) + (NUM2INT_NUMERIC_HEADER_IS_SHORT(n) ? 0 : sizeof(int16)))

/*
 * Get pointer to digit array and count of digits
 */
#define NUM2INT_NUMERIC_DIGITS(n) \
    (NUM2INT_NUMERIC_IS_SHORT(n) ? \
        ((Num2IntNumericData *)(n))->choice.n_short.n_data : \
        ((Num2IntNumericData *)(n))->choice.n_long.n_data)

#define NUM2INT_NUMERIC_NDIGITS(n) \
    ((VARSIZE(n) - NUM2INT_NUMERIC_HEADER_SIZE(n)) / sizeof(Num2IntNumericDigit))

/*
 * Quick check if Numeric is integral (no fractional part)
 * A Numeric is integral if ndigits <= weight + 1 (all digits are before decimal point)
 * Note: Must check for special values (NaN/Inf) separately before calling
 */
#define NUM2INT_NUMERIC_IS_INTEGRAL(n) \
    (NUM2INT_NUMERIC_NDIGITS(n) <= (NUM2INT_NUMERIC_WEIGHT(n) + 1))

/* End of PostgreSQL built-in redefinitions */

/*
 * Operator type classification
 */
typedef enum {
  OP_TYPE_UNKNOWN = 0,
  OP_TYPE_EQ,
  OP_TYPE_NE,
  OP_TYPE_LT,
  OP_TYPE_GT,
  OP_TYPE_LE,
  OP_TYPE_GE
} OpType;

/** Number of cross-type comparison operators (6 ops × 18 type combinations) */
#define NUM2INT_OP_COUNT 108

/**
 * @brief Entry for operator lookup array
 */
typedef struct {
  Oid oid;       /**< Operator OID */
  Oid funcid;    /**< Implementing function OID */
  OpType type;   /**< Operator type classification */
} OperatorEntry;

/**
 * @brief Cache structure for operator OIDs
 *
 * Stores OIDs for all 108 operators to enable fast lookup in the support
 * function. Cache is populated lazily on first use and persists for the
 * lifetime of the backend process.
 */
typedef struct OperatorOidCache {
  int count;        /**< Total number of operators in the cache */
  OperatorEntry ops[NUM2INT_OP_COUNT];  /**< Operator OID and type lookup array */
} OperatorOidCache;

/**
 * @brief Pre-computed Numeric boundary values for range checking
 *
 * These Numeric representations of integer boundary values are computed once
 * at init time and cached for the lifetime of the backend. Using pre-computed
 * boundaries avoids per-call int64_to_numeric() conversions during range checks.
 */
typedef struct NumericBoundaryCache {
  bool initialized;       /**< True after init_numeric_boundaries() completes */
  Numeric int2_min;       /**< Numeric representation of PG_INT16_MIN */
  Numeric int2_max;       /**< Numeric representation of PG_INT16_MAX */
  Numeric int4_min;       /**< Numeric representation of PG_INT32_MIN */
  Numeric int4_max;       /**< Numeric representation of PG_INT32_MAX */
  Numeric int8_min;       /**< Numeric representation of PG_INT64_MIN */
  Numeric int8_max;       /**< Numeric representation of PG_INT64_MAX */
} NumericBoundaryCache;

/**
 * @brief Initialize the Numeric boundary cache
 *
 * Converts integer MIN/MAX boundary values to Numeric and stores them
 * in long-lived memory for repeated use. Must be called once per backend.
 *
 * @param cache Pointer to the cache structure to initialize
 */
extern void init_numeric_boundaries(NumericBoundaryCache *cache);

/*
 * ============================================================================
 * Optimized Numeric Inspection Functions
 * ============================================================================
 * These functions use direct structure access for efficiency, avoiding
 * function call overhead for common operations.
 */

/**
 * @brief Get the sign of a Numeric value
 * @param num Numeric value to inspect
 * @return 1 if positive, -1 if negative, 0 if zero
 * @note Returns 0 for NaN (caller should check NUM2INT_NUMERIC_IS_NAN first)
 *       Returns 1 for +Inf, -1 for -Inf
 */
extern int num2int_numeric_sign(Numeric num);

/**
 * @brief Check if Numeric value is integral (no fractional part)
 * @param num Numeric value to check
 * @return true if integral, false if has fractional digits or is NaN
 * @note Infinities are considered integral (matches PostgreSQL behavior)
 */
extern bool num2int_numeric_is_integral(Numeric num);

/**
 * @brief Convert Numeric to int64 if possible
 * @param num Numeric value to convert
 * @param result Output: converted value
 * @return true if conversion succeeded, false if out of range or has fraction
 * @note Returns false for NaN/Inf values
 */
extern bool num2int_numeric_to_int64(Numeric num, int64 *result);

/* Function declarations */

/**
 * @brief Initialize the operator OID cache
 *
 * Populates the cache with OIDs for all 108 operators by looking up operator
 * names in the system catalog. Called lazily on first invocation of the
 * support function.
 *
 * @param cache Pointer to the cache structure to initialize
 */
extern void init_oid_cache(OperatorOidCache *cache);

/**
 * @brief Generic support function for all comparison operators
 * @param fcinfo Function call information structure
 * @return Node pointer for index condition or NULL if not supported
 *
 * Implements SupportRequestIndexCondition to enable btree index usage for
 * exact comparison predicates. Inspects the operator OID to determine the
 * type combination being compared, validates that the constant operand is
 * within range, and transforms the predicate for index scanning.
 */
extern Datum num2int_support(PG_FUNCTION_ARGS);

/* Core comparison functions - return -1, 0, or 1 */
extern int numeric_cmp_int2_internal(Numeric num, int16 val);
extern int numeric_cmp_int4_internal(Numeric num, int32 val);
extern int numeric_cmp_int8_internal(Numeric num, int64 val);
extern int float4_cmp_int2_internal(float4 fval, int16 ival);
extern int float4_cmp_int4_internal(float4 fval, int32 ival);
extern int float4_cmp_int8_internal(float4 fval, int64 ival);
extern int float8_cmp_int2_internal(float8 fval, int16 ival);
extern int float8_cmp_int4_internal(float8 fval, int32 ival);
extern int float8_cmp_int8_internal(float8 fval, int64 ival);

/* Equality operator functions (=) */
extern Datum numeric_eq_int2(PG_FUNCTION_ARGS);
extern Datum numeric_eq_int4(PG_FUNCTION_ARGS);
extern Datum int4_eq_numeric(PG_FUNCTION_ARGS);
extern Datum numeric_eq_int8(PG_FUNCTION_ARGS);
extern Datum float4_eq_int2(PG_FUNCTION_ARGS);
extern Datum float4_eq_int4(PG_FUNCTION_ARGS);
extern Datum float4_eq_int8(PG_FUNCTION_ARGS);
extern Datum float8_eq_int2(PG_FUNCTION_ARGS);
extern Datum float8_eq_int4(PG_FUNCTION_ARGS);
extern Datum float8_eq_int8(PG_FUNCTION_ARGS);

/* Commutator equality functions (int X = numeric/float) */
extern Datum int2_eq_numeric(PG_FUNCTION_ARGS);
extern Datum int2_eq_float4(PG_FUNCTION_ARGS);
extern Datum int2_eq_float8(PG_FUNCTION_ARGS);
extern Datum int4_eq_numeric(PG_FUNCTION_ARGS);
extern Datum int4_eq_float4(PG_FUNCTION_ARGS);
extern Datum int4_eq_float8(PG_FUNCTION_ARGS);
extern Datum int8_eq_numeric(PG_FUNCTION_ARGS);
extern Datum int8_eq_float4(PG_FUNCTION_ARGS);
extern Datum int8_eq_float8(PG_FUNCTION_ARGS);

/* Inequality operator functions (<>) */
extern Datum numeric_ne_int2(PG_FUNCTION_ARGS);
extern Datum numeric_ne_int4(PG_FUNCTION_ARGS);
extern Datum numeric_ne_int8(PG_FUNCTION_ARGS);
extern Datum float4_ne_int2(PG_FUNCTION_ARGS);
extern Datum float4_ne_int4(PG_FUNCTION_ARGS);
extern Datum float4_ne_int8(PG_FUNCTION_ARGS);
extern Datum float8_ne_int2(PG_FUNCTION_ARGS);
extern Datum float8_ne_int4(PG_FUNCTION_ARGS);
extern Datum float8_ne_int8(PG_FUNCTION_ARGS);

/* Commutator inequality functions (int X <> numeric/float) */
extern Datum int2_ne_numeric(PG_FUNCTION_ARGS);
extern Datum int2_ne_float4(PG_FUNCTION_ARGS);
extern Datum int2_ne_float8(PG_FUNCTION_ARGS);
extern Datum int4_ne_numeric(PG_FUNCTION_ARGS);
extern Datum int4_ne_float4(PG_FUNCTION_ARGS);
extern Datum int4_ne_float8(PG_FUNCTION_ARGS);
extern Datum int8_ne_numeric(PG_FUNCTION_ARGS);
extern Datum int8_ne_float4(PG_FUNCTION_ARGS);
extern Datum int8_ne_float8(PG_FUNCTION_ARGS);
extern Datum float4_ne_int4(PG_FUNCTION_ARGS);
extern Datum float4_ne_int8(PG_FUNCTION_ARGS);
extern Datum float8_ne_int2(PG_FUNCTION_ARGS);
extern Datum float8_ne_int4(PG_FUNCTION_ARGS);
extern Datum float8_ne_int8(PG_FUNCTION_ARGS);

/* Less than operator functions (<) */
extern Datum numeric_lt_int2(PG_FUNCTION_ARGS);
extern Datum numeric_lt_int4(PG_FUNCTION_ARGS);
extern Datum numeric_lt_int8(PG_FUNCTION_ARGS);
extern Datum float4_lt_int2(PG_FUNCTION_ARGS);
extern Datum float4_lt_int4(PG_FUNCTION_ARGS);
extern Datum float4_lt_int8(PG_FUNCTION_ARGS);
extern Datum float8_lt_int2(PG_FUNCTION_ARGS);
extern Datum float8_lt_int4(PG_FUNCTION_ARGS);
extern Datum float8_lt_int8(PG_FUNCTION_ARGS);

/* Greater than operator functions (>) */
extern Datum numeric_gt_int2(PG_FUNCTION_ARGS);
extern Datum numeric_gt_int4(PG_FUNCTION_ARGS);
extern Datum numeric_gt_int8(PG_FUNCTION_ARGS);
extern Datum float4_gt_int2(PG_FUNCTION_ARGS);
extern Datum float4_gt_int4(PG_FUNCTION_ARGS);
extern Datum float4_gt_int8(PG_FUNCTION_ARGS);
extern Datum float8_gt_int2(PG_FUNCTION_ARGS);
extern Datum float8_gt_int4(PG_FUNCTION_ARGS);
extern Datum float8_gt_int8(PG_FUNCTION_ARGS);

/* Less than or equal operator functions (<=) */
extern Datum numeric_le_int2(PG_FUNCTION_ARGS);
extern Datum numeric_le_int4(PG_FUNCTION_ARGS);
extern Datum numeric_le_int8(PG_FUNCTION_ARGS);
extern Datum float4_le_int2(PG_FUNCTION_ARGS);
extern Datum float4_le_int4(PG_FUNCTION_ARGS);
extern Datum float4_le_int8(PG_FUNCTION_ARGS);
extern Datum float8_le_int2(PG_FUNCTION_ARGS);
extern Datum float8_le_int4(PG_FUNCTION_ARGS);
extern Datum float8_le_int8(PG_FUNCTION_ARGS);

/* Greater than or equal operator functions (>=) */
extern Datum numeric_ge_int2(PG_FUNCTION_ARGS);
extern Datum numeric_ge_int4(PG_FUNCTION_ARGS);
extern Datum numeric_ge_int8(PG_FUNCTION_ARGS);
extern Datum float4_ge_int2(PG_FUNCTION_ARGS);
extern Datum float4_ge_int4(PG_FUNCTION_ARGS);
extern Datum float4_ge_int8(PG_FUNCTION_ARGS);
extern Datum float8_ge_int2(PG_FUNCTION_ARGS);
extern Datum float8_ge_int4(PG_FUNCTION_ARGS);
extern Datum float8_ge_int8(PG_FUNCTION_ARGS);

#endif /* PG_NUM2INT_DIRECT_COMP_H */
