/*
 * Copyright (c) 2025 Dave Sharpe
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
 * PostgreSQL Built-in Function OIDs
 *
 * These OIDs are stable across PostgreSQL versions and represent the
 * built-in cast functions from numeric to integer types. These functions
 * are defined in the PostgreSQL system catalog and perform standard
 * banker's rounding during conversion.
 *
 * PostgreSQL does not define symbolic constants for these in pg_proc_d.h,
 * so we define them here with NUM2INT_ prefix to avoid potential conflicts.
 *
 * Sources: pg_proc system catalog, documented in PostgreSQL source:
 *   src/include/catalog/pg_proc.dat
 */
#define NUM2INT_CAST_NUMERIC_INT2  1783  /* int2(numeric) - cast numeric to smallint */
#define NUM2INT_CAST_NUMERIC_INT4  1744  /* int4(numeric) - cast numeric to integer */
#define NUM2INT_CAST_NUMERIC_INT8  1779  /* int8(numeric) - cast numeric to bigint */

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
 * Sources: pg_operator system catalog, documented in PostgreSQL source:
 *   src/include/catalog/pg_operator.dat
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

/**
 * @brief Cache structure for operator OIDs
 *
 * Stores OIDs for all 54 operators to enable fast lookup in the support
 * function. Cache is populated lazily on first use and persists for the
 * lifetime of the backend process.
 */
typedef struct OperatorOidCache {
  bool initialized;  /**< True if cache has been populated */
  
  /* Equality operators (=) - 18 operators (forward + commutator) */
  Oid numeric_eq_int2_oid;
  Oid numeric_eq_int4_oid;
  Oid numeric_eq_int8_oid;
  Oid float4_eq_int2_oid;
  Oid float4_eq_int4_oid;
  Oid float4_eq_int8_oid;
  Oid float8_eq_int2_oid;
  Oid float8_eq_int4_oid;
  Oid float8_eq_int8_oid;
  /* Commutator equality operators (int X = numeric/float) */
  Oid int2_eq_numeric_oid;
  Oid int2_eq_float4_oid;
  Oid int2_eq_float8_oid;
  Oid int4_eq_numeric_oid;
  Oid int4_eq_float4_oid;
  Oid int4_eq_float8_oid;
  Oid int8_eq_numeric_oid;
  Oid int8_eq_float4_oid;
  Oid int8_eq_float8_oid;
  
  /* Inequality operators (<>) - 18 operators (forward + commutator) */
  Oid numeric_ne_int2_oid;
  Oid numeric_ne_int4_oid;
  Oid numeric_ne_int8_oid;
  Oid float4_ne_int2_oid;
  Oid float4_ne_int4_oid;
  Oid float4_ne_int8_oid;
  Oid float8_ne_int2_oid;
  Oid float8_ne_int4_oid;
  Oid float8_ne_int8_oid;
  /* Commutator inequality operators (int X <> numeric/float) */
  Oid int2_ne_numeric_oid;
  Oid int2_ne_float4_oid;
  Oid int2_ne_float8_oid;
  Oid int4_ne_numeric_oid;
  Oid int4_ne_float4_oid;
  Oid int4_ne_float8_oid;
  Oid int8_ne_numeric_oid;
  Oid int8_ne_float4_oid;
  Oid int8_ne_float8_oid;
  
  /* Less than operators (<) - 9 operators */
  Oid numeric_lt_int2_oid;
  Oid numeric_lt_int4_oid;
  Oid numeric_lt_int8_oid;
  Oid float4_lt_int2_oid;
  Oid float4_lt_int4_oid;
  Oid float4_lt_int8_oid;
  Oid float8_lt_int2_oid;
  Oid float8_lt_int4_oid;
  Oid float8_lt_int8_oid;
  
  /* Greater than operators (>) - 9 operators */
  Oid numeric_gt_int2_oid;
  Oid numeric_gt_int4_oid;
  Oid numeric_gt_int8_oid;
  Oid float4_gt_int2_oid;
  Oid float4_gt_int4_oid;
  Oid float4_gt_int8_oid;
  Oid float8_gt_int2_oid;
  Oid float8_gt_int4_oid;
  Oid float8_gt_int8_oid;
  
  /* Less than or equal operators (<=) - 9 operators */
  Oid numeric_le_int2_oid;
  Oid numeric_le_int4_oid;
  Oid numeric_le_int8_oid;
  Oid float4_le_int2_oid;
  Oid float4_le_int4_oid;
  Oid float4_le_int8_oid;
  Oid float8_le_int2_oid;
  Oid float8_le_int4_oid;
  Oid float8_le_int8_oid;
  
  /* Greater than or equal operators (>=) - 9 operators */
  Oid numeric_ge_int2_oid;
  Oid numeric_ge_int4_oid;
  Oid numeric_ge_int8_oid;
  Oid float4_ge_int2_oid;
  Oid float4_ge_int4_oid;
  Oid float4_ge_int8_oid;
  Oid float8_ge_int2_oid;
  Oid float8_ge_int4_oid;
  Oid float8_ge_int8_oid;
} OperatorOidCache;

/* Function declarations */

/**
 * @brief Initialize the operator OID cache
 * 
 * Populates the cache with OIDs for all 54 operators by looking up operator
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
