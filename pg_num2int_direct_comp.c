/*
 * Copyright (c) 2025 Dave Sharpe
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
 * support btree index optimization via SupportRequestIndexCondition.
 */

#include "pg_num2int_direct_comp.h"
#include "catalog/pg_proc_d.h"
#include "catalog/pg_type_d.h"
#include "catalog/namespace.h"
#include "catalog/pg_operator.h"
#include "nodes/supportnodes.h"
#include "nodes/nodeFuncs.h"
#include "nodes/makefuncs.h"
#include "optimizer/optimizer.h"
#include "utils/fmgrprotos.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/syscache.h"
#include "fmgr.h"
#include <math.h>

PG_MODULE_MAGIC;

/* Static per-backend OID cache */
static OperatorOidCache oid_cache = {false};

/**
 * @brief Initialize the operator OID cache
 * @param cache Pointer to cache structure to populate
 *
 * Performs lazy initialization of operator OIDs by looking up operator
 * names in the system catalog. This function is called once per backend
 * on first invocation of the support function.
 */
void
init_oid_cache(OperatorOidCache *cache) {
  if (cache->initialized) {
    return;
  }
  
  /* Populate equality operator OIDs (=) */
  cache->numeric_eq_int2_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_eq_int4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_eq_int8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  NUMERICOID, INT8OID);
  cache->float4_eq_int2_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT4OID, INT2OID);
  cache->float4_eq_int4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT4OID, INT4OID);
  cache->float4_eq_int8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT4OID, INT8OID);
  cache->float8_eq_int2_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT8OID, INT2OID);
  cache->float8_eq_int4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT8OID, INT4OID);
  cache->float8_eq_int8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate commutator equality operator OIDs (int X = numeric/float) */
  cache->int2_eq_numeric_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  INT2OID, NUMERICOID);
  cache->int2_eq_float4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT2OID, FLOAT4OID);
  cache->int2_eq_float8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT2OID, FLOAT8OID);
  cache->int4_eq_numeric_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  INT4OID, NUMERICOID);
  cache->int4_eq_float4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT4OID, FLOAT4OID);
  cache->int4_eq_float8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT4OID, FLOAT8OID);
  cache->int8_eq_numeric_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                  INT8OID, NUMERICOID);
  cache->int8_eq_float4_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT8OID, FLOAT4OID);
  cache->int8_eq_float8_oid = OpernameGetOprid(list_make1(makeString("=")),
                                                INT8OID, FLOAT8OID);
  
  /* Populate inequality operator OIDs (<>) */
  cache->numeric_ne_int2_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_ne_int4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_ne_int8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  NUMERICOID, INT8OID);
  cache->float4_ne_int2_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT4OID, INT2OID);
  cache->float4_ne_int4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT4OID, INT4OID);
  cache->float4_ne_int8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT4OID, INT8OID);
  cache->float8_ne_int2_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT8OID, INT2OID);
  cache->float8_ne_int4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT8OID, INT4OID);
  cache->float8_ne_int8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate commutator inequality operator OIDs (int X <> numeric/float) */
  cache->int2_ne_numeric_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  INT2OID, NUMERICOID);
  cache->int2_ne_float4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT2OID, FLOAT4OID);
  cache->int2_ne_float8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT2OID, FLOAT8OID);
  cache->int4_ne_numeric_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  INT4OID, NUMERICOID);
  cache->int4_ne_float4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT4OID, FLOAT4OID);
  cache->int4_ne_float8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT4OID, FLOAT8OID);
  cache->int8_ne_numeric_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                  INT8OID, NUMERICOID);
  cache->int8_ne_float4_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT8OID, FLOAT4OID);
  cache->int8_ne_float8_oid = OpernameGetOprid(list_make1(makeString("<>")),
                                                INT8OID, FLOAT8OID);
  
  /* Populate less than operator OIDs (<) */
  cache->numeric_lt_int2_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_lt_int4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_lt_int8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  NUMERICOID, INT8OID);
  cache->float4_lt_int2_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT4OID, INT2OID);
  cache->float4_lt_int4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT4OID, INT4OID);
  cache->float4_lt_int8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT4OID, INT8OID);
  cache->float8_lt_int2_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT8OID, INT2OID);
  cache->float8_lt_int4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT8OID, INT4OID);
  cache->float8_lt_int8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate greater than operator OIDs (>) */
  cache->numeric_gt_int2_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_gt_int4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_gt_int8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  NUMERICOID, INT8OID);
  cache->float4_gt_int2_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT4OID, INT2OID);
  cache->float4_gt_int4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT4OID, INT4OID);
  cache->float4_gt_int8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT4OID, INT8OID);
  cache->float8_gt_int2_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT8OID, INT2OID);
  cache->float8_gt_int4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT8OID, INT4OID);
  cache->float8_gt_int8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate less than or equal operator OIDs (<=) */
  cache->numeric_le_int2_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_le_int4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_le_int8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  NUMERICOID, INT8OID);
  cache->float4_le_int2_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT4OID, INT2OID);
  cache->float4_le_int4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT4OID, INT4OID);
  cache->float4_le_int8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT4OID, INT8OID);
  cache->float8_le_int2_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT8OID, INT2OID);
  cache->float8_le_int4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT8OID, INT4OID);
  cache->float8_le_int8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate greater than or equal operator OIDs (>=) */
  cache->numeric_ge_int2_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  NUMERICOID, INT2OID);
  cache->numeric_ge_int4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  NUMERICOID, INT4OID);
  cache->numeric_ge_int8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  NUMERICOID, INT8OID);
  cache->float4_ge_int2_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT4OID, INT2OID);
  cache->float4_ge_int4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT4OID, INT4OID);
  cache->float4_ge_int8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT4OID, INT8OID);
  cache->float8_ge_int2_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT8OID, INT2OID);
  cache->float8_ge_int4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT8OID, INT4OID);
  cache->float8_ge_int8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                FLOAT8OID, INT8OID);
  
  /* Populate commutator less than operator OIDs (int X < numeric/float) */
  cache->int2_lt_numeric_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  INT2OID, NUMERICOID);
  cache->int2_lt_float4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT2OID, FLOAT4OID);
  cache->int2_lt_float8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT2OID, FLOAT8OID);
  cache->int4_lt_numeric_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  INT4OID, NUMERICOID);
  cache->int4_lt_float4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT4OID, FLOAT4OID);
  cache->int4_lt_float8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT4OID, FLOAT8OID);
  cache->int8_lt_numeric_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                  INT8OID, NUMERICOID);
  cache->int8_lt_float4_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT8OID, FLOAT4OID);
  cache->int8_lt_float8_oid = OpernameGetOprid(list_make1(makeString("<")),
                                                INT8OID, FLOAT8OID);
  
  /* Populate commutator greater than operator OIDs (int X > numeric/float) */
  cache->int2_gt_numeric_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  INT2OID, NUMERICOID);
  cache->int2_gt_float4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT2OID, FLOAT4OID);
  cache->int2_gt_float8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT2OID, FLOAT8OID);
  cache->int4_gt_numeric_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  INT4OID, NUMERICOID);
  cache->int4_gt_float4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT4OID, FLOAT4OID);
  cache->int4_gt_float8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT4OID, FLOAT8OID);
  cache->int8_gt_numeric_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                  INT8OID, NUMERICOID);
  cache->int8_gt_float4_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT8OID, FLOAT4OID);
  cache->int8_gt_float8_oid = OpernameGetOprid(list_make1(makeString(">")),
                                                INT8OID, FLOAT8OID);
  
  /* Populate commutator less than or equal operator OIDs (int X <= numeric/float) */
  cache->int2_le_numeric_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  INT2OID, NUMERICOID);
  cache->int2_le_float4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT2OID, FLOAT4OID);
  cache->int2_le_float8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT2OID, FLOAT8OID);
  cache->int4_le_numeric_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  INT4OID, NUMERICOID);
  cache->int4_le_float4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT4OID, FLOAT4OID);
  cache->int4_le_float8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT4OID, FLOAT8OID);
  cache->int8_le_numeric_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                  INT8OID, NUMERICOID);
  cache->int8_le_float4_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT8OID, FLOAT4OID);
  cache->int8_le_float8_oid = OpernameGetOprid(list_make1(makeString("<=")),
                                                INT8OID, FLOAT8OID);
  
  /* Populate commutator greater than or equal operator OIDs (int X >= numeric/float) */
  cache->int2_ge_numeric_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  INT2OID, NUMERICOID);
  cache->int2_ge_float4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT2OID, FLOAT4OID);
  cache->int2_ge_float8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT2OID, FLOAT8OID);
  cache->int4_ge_numeric_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  INT4OID, NUMERICOID);
  cache->int4_ge_float4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT4OID, FLOAT4OID);
  cache->int4_ge_float8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT4OID, FLOAT8OID);
  cache->int8_ge_numeric_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                  INT8OID, NUMERICOID);
  cache->int8_ge_float4_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT8OID, FLOAT4OID);
  cache->int8_ge_float8_oid = OpernameGetOprid(list_make1(makeString(">=")),
                                                INT8OID, FLOAT8OID);
  
  cache->initialized = true;
}

/*
 * ============================================================================
 * Support Function Helper Types and Functions
 * ============================================================================
 * These helpers are shared between SupportRequestIndexCondition and
 * SupportRequestSimplify handlers to avoid code duplication.
 */

/**
 * @brief Operator type classification
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

/**
 * @brief Result of constant conversion
 */
typedef struct {
  bool valid;          /**< True if conversion succeeded */
  bool has_fraction;   /**< True if constant has fractional part */
  int64 int_val;       /**< Converted integer value (truncated) */
} ConstConversion;

/**
 * @brief Classify an operator OID into OpType
 * @param opno Operator OID
 * @return OpType classification
 */
static OpType
classify_operator(Oid opno) {
  /* Initialize cache if needed */
  init_oid_cache(&oid_cache);
  
  /* Check equality operators */
  if (opno == oid_cache.numeric_eq_int2_oid || opno == oid_cache.numeric_eq_int4_oid ||
      opno == oid_cache.numeric_eq_int8_oid || opno == oid_cache.float4_eq_int2_oid ||
      opno == oid_cache.float4_eq_int4_oid || opno == oid_cache.float4_eq_int8_oid ||
      opno == oid_cache.float8_eq_int2_oid || opno == oid_cache.float8_eq_int4_oid ||
      opno == oid_cache.float8_eq_int8_oid || opno == oid_cache.int2_eq_numeric_oid ||
      opno == oid_cache.int2_eq_float4_oid || opno == oid_cache.int2_eq_float8_oid ||
      opno == oid_cache.int4_eq_numeric_oid || opno == oid_cache.int4_eq_float4_oid ||
      opno == oid_cache.int4_eq_float8_oid || opno == oid_cache.int8_eq_numeric_oid ||
      opno == oid_cache.int8_eq_float4_oid || opno == oid_cache.int8_eq_float8_oid) {
    return OP_TYPE_EQ;
  }
  
  /* Check inequality operators */
  if (opno == oid_cache.numeric_ne_int2_oid || opno == oid_cache.numeric_ne_int4_oid ||
      opno == oid_cache.numeric_ne_int8_oid || opno == oid_cache.float4_ne_int2_oid ||
      opno == oid_cache.float4_ne_int4_oid || opno == oid_cache.float4_ne_int8_oid ||
      opno == oid_cache.float8_ne_int2_oid || opno == oid_cache.float8_ne_int4_oid ||
      opno == oid_cache.float8_ne_int8_oid || opno == oid_cache.int2_ne_numeric_oid ||
      opno == oid_cache.int2_ne_float4_oid || opno == oid_cache.int2_ne_float8_oid ||
      opno == oid_cache.int4_ne_numeric_oid || opno == oid_cache.int4_ne_float4_oid ||
      opno == oid_cache.int4_ne_float8_oid || opno == oid_cache.int8_ne_numeric_oid ||
      opno == oid_cache.int8_ne_float4_oid || opno == oid_cache.int8_ne_float8_oid) {
    return OP_TYPE_NE;
  }
  
  /* Check < operators */
  if (opno == oid_cache.numeric_lt_int2_oid || opno == oid_cache.numeric_lt_int4_oid ||
      opno == oid_cache.numeric_lt_int8_oid || opno == oid_cache.float4_lt_int2_oid ||
      opno == oid_cache.float4_lt_int4_oid || opno == oid_cache.float4_lt_int8_oid ||
      opno == oid_cache.float8_lt_int2_oid || opno == oid_cache.float8_lt_int4_oid ||
      opno == oid_cache.float8_lt_int8_oid || opno == oid_cache.int2_lt_numeric_oid ||
      opno == oid_cache.int2_lt_float4_oid || opno == oid_cache.int2_lt_float8_oid ||
      opno == oid_cache.int4_lt_numeric_oid || opno == oid_cache.int4_lt_float4_oid ||
      opno == oid_cache.int4_lt_float8_oid || opno == oid_cache.int8_lt_numeric_oid ||
      opno == oid_cache.int8_lt_float4_oid || opno == oid_cache.int8_lt_float8_oid) {
    return OP_TYPE_LT;
  }
  
  /* Check > operators */
  if (opno == oid_cache.numeric_gt_int2_oid || opno == oid_cache.numeric_gt_int4_oid ||
      opno == oid_cache.numeric_gt_int8_oid || opno == oid_cache.float4_gt_int2_oid ||
      opno == oid_cache.float4_gt_int4_oid || opno == oid_cache.float4_gt_int8_oid ||
      opno == oid_cache.float8_gt_int2_oid || opno == oid_cache.float8_gt_int4_oid ||
      opno == oid_cache.float8_gt_int8_oid || opno == oid_cache.int2_gt_numeric_oid ||
      opno == oid_cache.int2_gt_float4_oid || opno == oid_cache.int2_gt_float8_oid ||
      opno == oid_cache.int4_gt_numeric_oid || opno == oid_cache.int4_gt_float4_oid ||
      opno == oid_cache.int4_gt_float8_oid || opno == oid_cache.int8_gt_numeric_oid ||
      opno == oid_cache.int8_gt_float4_oid || opno == oid_cache.int8_gt_float8_oid) {
    return OP_TYPE_GT;
  }
  
  /* Check <= operators */
  if (opno == oid_cache.numeric_le_int2_oid || opno == oid_cache.numeric_le_int4_oid ||
      opno == oid_cache.numeric_le_int8_oid || opno == oid_cache.float4_le_int2_oid ||
      opno == oid_cache.float4_le_int4_oid || opno == oid_cache.float4_le_int8_oid ||
      opno == oid_cache.float8_le_int2_oid || opno == oid_cache.float8_le_int4_oid ||
      opno == oid_cache.float8_le_int8_oid || opno == oid_cache.int2_le_numeric_oid ||
      opno == oid_cache.int2_le_float4_oid || opno == oid_cache.int2_le_float8_oid ||
      opno == oid_cache.int4_le_numeric_oid || opno == oid_cache.int4_le_float4_oid ||
      opno == oid_cache.int4_le_float8_oid || opno == oid_cache.int8_le_numeric_oid ||
      opno == oid_cache.int8_le_float4_oid || opno == oid_cache.int8_le_float8_oid) {
    return OP_TYPE_LE;
  }
  
  /* Check >= operators */
  if (opno == oid_cache.numeric_ge_int2_oid || opno == oid_cache.numeric_ge_int4_oid ||
      opno == oid_cache.numeric_ge_int8_oid || opno == oid_cache.float4_ge_int2_oid ||
      opno == oid_cache.float4_ge_int4_oid || opno == oid_cache.float4_ge_int8_oid ||
      opno == oid_cache.float8_ge_int2_oid || opno == oid_cache.float8_ge_int4_oid ||
      opno == oid_cache.float8_ge_int8_oid || opno == oid_cache.int2_ge_numeric_oid ||
      opno == oid_cache.int2_ge_float4_oid || opno == oid_cache.int2_ge_float8_oid ||
      opno == oid_cache.int4_ge_numeric_oid || opno == oid_cache.int4_ge_float4_oid ||
      opno == oid_cache.int4_ge_float8_oid || opno == oid_cache.int8_ge_numeric_oid ||
      opno == oid_cache.int8_ge_float4_oid || opno == oid_cache.int8_ge_float8_oid) {
    return OP_TYPE_GE;
  }
  
  return OP_TYPE_UNKNOWN;
}

/**
 * @brief Find operator OID from function OID by searching cached operators
 * @param funcid Function OID to match
 * @param op_type Output: operator type classification
 * @return Matching operator OID or InvalidOid
 */
static Oid
find_operator_by_funcid(Oid funcid, OpType *op_type) {
  HeapTuple opertup;
  Form_pg_operator operform;
  
  /* Array of all operators to check, grouped by type */
  struct {
    Oid *ops;
    int count;
    OpType type;
  } op_groups[] = {
    /* Equality operators */
    { (Oid[]){oid_cache.numeric_eq_int2_oid, oid_cache.numeric_eq_int4_oid,
              oid_cache.numeric_eq_int8_oid, oid_cache.float4_eq_int2_oid,
              oid_cache.float4_eq_int4_oid, oid_cache.float4_eq_int8_oid,
              oid_cache.float8_eq_int2_oid, oid_cache.float8_eq_int4_oid,
              oid_cache.float8_eq_int8_oid, oid_cache.int2_eq_numeric_oid,
              oid_cache.int2_eq_float4_oid, oid_cache.int2_eq_float8_oid,
              oid_cache.int4_eq_numeric_oid, oid_cache.int4_eq_float4_oid,
              oid_cache.int4_eq_float8_oid, oid_cache.int8_eq_numeric_oid,
              oid_cache.int8_eq_float4_oid, oid_cache.int8_eq_float8_oid}, 18, OP_TYPE_EQ },
    /* Inequality operators */
    { (Oid[]){oid_cache.numeric_ne_int2_oid, oid_cache.numeric_ne_int4_oid,
              oid_cache.numeric_ne_int8_oid, oid_cache.float4_ne_int2_oid,
              oid_cache.float4_ne_int4_oid, oid_cache.float4_ne_int8_oid,
              oid_cache.float8_ne_int2_oid, oid_cache.float8_ne_int4_oid,
              oid_cache.float8_ne_int8_oid, oid_cache.int2_ne_numeric_oid,
              oid_cache.int2_ne_float4_oid, oid_cache.int2_ne_float8_oid,
              oid_cache.int4_ne_numeric_oid, oid_cache.int4_ne_float4_oid,
              oid_cache.int4_ne_float8_oid, oid_cache.int8_ne_numeric_oid,
              oid_cache.int8_ne_float4_oid, oid_cache.int8_ne_float8_oid}, 18, OP_TYPE_NE },
    /* Less than operators */
    { (Oid[]){oid_cache.numeric_lt_int2_oid, oid_cache.numeric_lt_int4_oid,
              oid_cache.numeric_lt_int8_oid, oid_cache.float4_lt_int2_oid,
              oid_cache.float4_lt_int4_oid, oid_cache.float4_lt_int8_oid,
              oid_cache.float8_lt_int2_oid, oid_cache.float8_lt_int4_oid,
              oid_cache.float8_lt_int8_oid, oid_cache.int2_lt_numeric_oid,
              oid_cache.int2_lt_float4_oid, oid_cache.int2_lt_float8_oid,
              oid_cache.int4_lt_numeric_oid, oid_cache.int4_lt_float4_oid,
              oid_cache.int4_lt_float8_oid, oid_cache.int8_lt_numeric_oid,
              oid_cache.int8_lt_float4_oid, oid_cache.int8_lt_float8_oid}, 18, OP_TYPE_LT },
    /* Greater than operators */
    { (Oid[]){oid_cache.numeric_gt_int2_oid, oid_cache.numeric_gt_int4_oid,
              oid_cache.numeric_gt_int8_oid, oid_cache.float4_gt_int2_oid,
              oid_cache.float4_gt_int4_oid, oid_cache.float4_gt_int8_oid,
              oid_cache.float8_gt_int2_oid, oid_cache.float8_gt_int4_oid,
              oid_cache.float8_gt_int8_oid, oid_cache.int2_gt_numeric_oid,
              oid_cache.int2_gt_float4_oid, oid_cache.int2_gt_float8_oid,
              oid_cache.int4_gt_numeric_oid, oid_cache.int4_gt_float4_oid,
              oid_cache.int4_gt_float8_oid, oid_cache.int8_gt_numeric_oid,
              oid_cache.int8_gt_float4_oid, oid_cache.int8_gt_float8_oid}, 18, OP_TYPE_GT },
    /* Less than or equal operators */
    { (Oid[]){oid_cache.numeric_le_int2_oid, oid_cache.numeric_le_int4_oid,
              oid_cache.numeric_le_int8_oid, oid_cache.float4_le_int2_oid,
              oid_cache.float4_le_int4_oid, oid_cache.float4_le_int8_oid,
              oid_cache.float8_le_int2_oid, oid_cache.float8_le_int4_oid,
              oid_cache.float8_le_int8_oid, oid_cache.int2_le_numeric_oid,
              oid_cache.int2_le_float4_oid, oid_cache.int2_le_float8_oid,
              oid_cache.int4_le_numeric_oid, oid_cache.int4_le_float4_oid,
              oid_cache.int4_le_float8_oid, oid_cache.int8_le_numeric_oid,
              oid_cache.int8_le_float4_oid, oid_cache.int8_le_float8_oid}, 18, OP_TYPE_LE },
    /* Greater than or equal operators */
    { (Oid[]){oid_cache.numeric_ge_int2_oid, oid_cache.numeric_ge_int4_oid,
              oid_cache.numeric_ge_int8_oid, oid_cache.float4_ge_int2_oid,
              oid_cache.float4_ge_int4_oid, oid_cache.float4_ge_int8_oid,
              oid_cache.float8_ge_int2_oid, oid_cache.float8_ge_int4_oid,
              oid_cache.float8_ge_int8_oid, oid_cache.int2_ge_numeric_oid,
              oid_cache.int2_ge_float4_oid, oid_cache.int2_ge_float8_oid,
              oid_cache.int4_ge_numeric_oid, oid_cache.int4_ge_float4_oid,
              oid_cache.int4_ge_float8_oid, oid_cache.int8_ge_numeric_oid,
              oid_cache.int8_ge_float4_oid, oid_cache.int8_ge_float8_oid}, 18, OP_TYPE_GE }
  };
  
  for (int g = 0; g < 6; g++) {
    for (int i = 0; i < op_groups[g].count; i++) {
      Oid op_oid = op_groups[g].ops[i];
      opertup = SearchSysCache1(OPEROID, ObjectIdGetDatum(op_oid));
      if (HeapTupleIsValid(opertup)) {
        operform = (Form_pg_operator) GETSTRUCT(opertup);
        bool match = (operform->oprcode == funcid);
        ReleaseSysCache(opertup);
        if (match) {
          *op_type = op_groups[g].type;
          return op_oid;
        }
      }
    }
  }
  
  *op_type = OP_TYPE_UNKNOWN;
  return InvalidOid;
}

/**
 * @brief Convert a numeric/float constant to integer
 * @param const_node Constant node to convert
 * @param int_type Target integer type OID (INT2OID, INT4OID, INT8OID)
 * @return Conversion result with validity flag, fractional flag, and value
 */
static ConstConversion
convert_const_to_int(Const *const_node, Oid int_type) {
  ConstConversion result = {false, false, 0};
  Oid const_type = const_node->consttype;
  
  if (const_node->constisnull) {
    return result;
  }
  
  if (const_type == NUMERICOID) {
    Numeric num = DatumGetNumeric(const_node->constvalue);
    Numeric trunc_num;
    Numeric floor_num;
    
    if (numeric_is_nan(num) || numeric_is_inf(num)) {
      return result;
    }
    
    /* Check for fractional part using trunc(numeric) (OID 1710) and numeric_eq (OID 1718) */
    trunc_num = DatumGetNumeric(OidFunctionCall1(1710, NumericGetDatum(num)));
    result.has_fraction = !DatumGetBool(OidFunctionCall2(1718,
                                                          NumericGetDatum(num),
                                                          NumericGetDatum(trunc_num)));
    
    /* Use floor() (OID 1712) for consistent rounding toward negative infinity */
    floor_num = DatumGetNumeric(OidFunctionCall1(1712, NumericGetDatum(num)));
    
    /* Convert floor value to integer */
    PG_TRY();
    {
      if (int_type == INT2OID) {
        Datum d = OidFunctionCall1(NUM2INT_CAST_NUMERIC_INT2, NumericGetDatum(floor_num));
        result.int_val = DatumGetInt16(d);
        result.valid = true;
      } else if (int_type == INT4OID) {
        Datum d = OidFunctionCall1(NUM2INT_CAST_NUMERIC_INT4, NumericGetDatum(floor_num));
        result.int_val = DatumGetInt32(d);
        result.valid = true;
      } else if (int_type == INT8OID) {
        Datum d = OidFunctionCall1(NUM2INT_CAST_NUMERIC_INT8, NumericGetDatum(floor_num));
        result.int_val = DatumGetInt64(d);
        result.valid = true;
      }
    }
    PG_CATCH();
    {
      FlushErrorState();
      result.valid = false;
    }
    PG_END_TRY();
  } else if (const_type == FLOAT4OID) {
    float4 fval = DatumGetFloat4(const_node->constvalue);
    
    if (isnan(fval) || isinf(fval)) {
      return result;
    }
    
    result.has_fraction = (fval != floorf(fval));
    
    /* Use floor for consistent rounding toward negative infinity */
    float4 floor_val = floorf(fval);
    if (int_type == INT2OID) {
      if (floor_val >= PG_INT16_MIN && floor_val <= PG_INT16_MAX) {
        result.int_val = (int16) floor_val;
        result.valid = true;
      }
    } else if (int_type == INT4OID) {
      if (floor_val >= PG_INT32_MIN && floor_val <= PG_INT32_MAX) {
        result.int_val = (int32) floor_val;
        result.valid = true;
      }
    } else if (int_type == INT8OID) {
      result.int_val = (int64) floor_val;
      result.valid = true;
    }
  } else if (const_type == FLOAT8OID) {
    float8 dval = DatumGetFloat8(const_node->constvalue);
    
    if (isnan(dval) || isinf(dval)) {
      return result;
    }
    
    result.has_fraction = (dval != floor(dval));
    
    /* Use floor for consistent rounding toward negative infinity */
    float8 floor_val = floor(dval);
    if (int_type == INT2OID) {
      if (floor_val >= PG_INT16_MIN && floor_val <= PG_INT16_MAX) {
        result.int_val = (int16) floor_val;
        result.valid = true;
      }
    } else if (int_type == INT4OID) {
      if (floor_val >= PG_INT32_MIN && floor_val <= PG_INT32_MAX) {
        result.int_val = (int32) floor_val;
        result.valid = true;
      }
    } else if (int_type == INT8OID) {
      result.int_val = (int64) floor_val;
      result.valid = true;
    }
  }
  
  return result;
}

/**
 * @brief Get native integer operator OID for given operator type and integer type
 * @param op_type Operator type classification
 * @param int_type Integer type OID
 * @return Native operator OID for same-type comparison
 */
static Oid
get_native_op_oid(OpType op_type, Oid int_type) {
  switch (op_type) {
    case OP_TYPE_EQ:
      if (int_type == INT2OID) return NUM2INT_INT2EQ_OID;
      if (int_type == INT4OID) return NUM2INT_INT4EQ_OID;
      return NUM2INT_INT8EQ_OID;
    case OP_TYPE_NE:
      if (int_type == INT2OID) return NUM2INT_INT2NE_OID;
      if (int_type == INT4OID) return NUM2INT_INT4NE_OID;
      return NUM2INT_INT8NE_OID;
    case OP_TYPE_LT:
      if (int_type == INT2OID) return NUM2INT_INT2LT_OID;
      if (int_type == INT4OID) return NUM2INT_INT4LT_OID;
      return NUM2INT_INT8LT_OID;
    case OP_TYPE_GT:
      if (int_type == INT2OID) return NUM2INT_INT2GT_OID;
      if (int_type == INT4OID) return NUM2INT_INT4GT_OID;
      return NUM2INT_INT8GT_OID;
    case OP_TYPE_LE:
      if (int_type == INT2OID) return NUM2INT_INT2LE_OID;
      if (int_type == INT4OID) return NUM2INT_INT4LE_OID;
      return NUM2INT_INT8LE_OID;
    case OP_TYPE_GE:
      if (int_type == INT2OID) return NUM2INT_INT2GE_OID;
      if (int_type == INT4OID) return NUM2INT_INT4GE_OID;
      return NUM2INT_INT8GE_OID;
    default:
      return InvalidOid;
  }
}

/**
 * @brief Create an integer constant node
 * @param int_type Integer type OID
 * @param int_val Integer value
 * @return New Const node
 */
static Const *
make_int_const(Oid int_type, int64 int_val) {
  if (int_type == INT2OID) {
    return makeConst(INT2OID, -1, InvalidOid, sizeof(int16),
                     Int16GetDatum((int16) int_val), false, true);
  } else if (int_type == INT4OID) {
    return makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                     Int32GetDatum((int32) int_val), false, true);
  } else {
    return makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
                     Int64GetDatum(int_val), false, true);
  }
}

/**
 * @brief Build transformed OpExpr using native integer operator
 * @param native_op_oid Native operator OID
 * @param var_node Variable node (will be copied)
 * @param int_val Integer constant value
 * @param int_type Integer type OID
 * @param location Source location for new expression
 * @param inputcollid Input collation ID
 * @return New OpExpr node
 */
static OpExpr *
build_native_opexpr(Oid native_op_oid, Var *var_node, int64 int_val,
                    Oid int_type, int location, Oid inputcollid) {
  Var *new_var = (Var *) copyObject(var_node);
  Const *new_const = make_int_const(int_type, int_val);
  
  OpExpr *new_clause = (OpExpr *) make_opclause(native_op_oid,
                                                 BOOLOID,
                                                 false,
                                                 (Expr *) new_var,
                                                 (Expr *) new_const,
                                                 InvalidOid,
                                                 InvalidOid);
  new_clause->inputcollid = inputcollid;
  new_clause->location = location;
  
  return new_clause;
}

/**
 * @brief Compute adjusted value and operator for range predicates with fractions
 * @param op_type Original operator type
 * @param int_type Integer type OID
 * @param int_val Truncated integer value
 * @param has_fraction Whether constant has fractional part
 * @param adjusted_val Output: adjusted integer value
 * @return Native operator OID to use
 */
static Oid
compute_range_transform(OpType op_type, Oid int_type, int64 int_val,
                        bool has_fraction, int64 *adjusted_val) {
  *adjusted_val = int_val;
  
  if (has_fraction) {
    if (op_type == OP_TYPE_GT || op_type == OP_TYPE_GE) {
      /* > 10.5 or >= 10.5 becomes >= 11 */
      *adjusted_val = int_val + 1;
      return get_native_op_oid(OP_TYPE_GE, int_type);
    } else {
      /* < 10.5 or <= 10.5 becomes <= 10 */
      return get_native_op_oid(OP_TYPE_LE, int_type);
    }
  }
  
  /* No fraction - preserve original operator */
  return get_native_op_oid(op_type, int_type);
}

/*
 * ============================================================================
 * Main Support Function
 * ============================================================================
 */

/**
 * @brief Generic support function for query optimization
 * @param fcinfo Function call context
 * @return Node pointer or NULL
 *
 * Handles two request types:
 * - SupportRequestIndexCondition: Transforms predicates for btree index scans
 * - SupportRequestSimplify: Simplifies constant predicates (FR-015, FR-016, FR-017)
 *   - FR-015: Equality with fractional value → FALSE
 *   - FR-016: Equality with exact integer → native operator
 *   - FR-017: Range with fraction → adjusted boundary
 */
PG_FUNCTION_INFO_V1(num2int_support);
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  Node *ret = NULL;
  
  /* Initialize OID cache on first use */
  init_oid_cache(&oid_cache);
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    /*
     * SupportRequestIndexCondition: Transform predicates for btree index scans
     */
    SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;
    req->lossy = false;
    
    if (!is_opclause(req->node)) {
      PG_RETURN_POINTER(NULL);
    }
    
    OpExpr *clause = (OpExpr *) req->node;
    Oid opno = clause->opno;
    
    if (list_length(clause->args) != 2) {
      PG_RETURN_POINTER(NULL);
    }
    
    Node *leftop = (Node *) linitial(clause->args);
    Node *rightop = (Node *) lsecond(clause->args);
    Var *var_node = NULL;
    Const *const_node = NULL;
    
    /* Identify Var and Const positions */
    if (IsA(leftop, Const) && IsA(rightop, Var)) {
      const_node = (Const *) leftop;
      var_node = (Var *) rightop;
    } else if (IsA(leftop, Var) && IsA(rightop, Const)) {
      var_node = (Var *) leftop;
      const_node = (Const *) rightop;
    } else {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Classify the operator */
    OpType op_type = classify_operator(opno);
    if (op_type == OP_TYPE_UNKNOWN) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Convert constant to integer */
    Oid int_type = var_node->vartype;
    ConstConversion conv = convert_const_to_int(const_node, int_type);
    if (!conv.valid) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* For equality with fractional value, index scan won't help - return NULL */
    if (op_type == OP_TYPE_EQ && conv.has_fraction) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Compute the native operator and adjusted value */
    int64 adjusted_val = conv.int_val;
    Oid native_op_oid;
    
    if (op_type == OP_TYPE_EQ || op_type == OP_TYPE_NE) {
      native_op_oid = get_native_op_oid(op_type, int_type);
    } else {
      native_op_oid = compute_range_transform(op_type, int_type, conv.int_val,
                                               conv.has_fraction, &adjusted_val);
    }
    
    /* Build transformed expression */
    OpExpr *new_clause = build_native_opexpr(native_op_oid, var_node, adjusted_val,
                                              int_type, clause->location,
                                              clause->inputcollid);
    
    ret = (Node *) list_make1(new_clause);
  }
  else if (IsA(rawreq, SupportRequestSimplify)) {
    /*
     * SupportRequestSimplify: Transform constant predicates at simplification time
     * 
     * - FR-015: Equality with fractional value → FALSE
     * - FR-016: Equality with exact integer → native operator
     * - FR-017: Range with fraction → adjusted boundary
     */
    SupportRequestSimplify *req = (SupportRequestSimplify *) rawreq;
    FuncExpr *func = req->fcall;
    
    if (list_length(func->args) != 2) {
      PG_RETURN_POINTER(NULL);
    }
    
    Node *leftop = (Node *) linitial(func->args);
    Node *rightop = (Node *) lsecond(func->args);
    Var *var_node = NULL;
    Const *const_node = NULL;
    
    /* Identify Var and Const positions */
    if (IsA(leftop, Const) && IsA(rightop, Var)) {
      const_node = (Const *) leftop;
      var_node = (Var *) rightop;
    } else if (IsA(leftop, Var) && IsA(rightop, Const)) {
      var_node = (Var *) leftop;
      const_node = (Const *) rightop;
    } else {
      PG_RETURN_POINTER(NULL);
    }
    
    if (const_node->constisnull) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Find the operator that uses this function */
    OpType op_type;
    Oid opno = find_operator_by_funcid(func->funcid, &op_type);
    if (opno == InvalidOid) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Convert constant to integer */
    Oid int_type = var_node->vartype;
    ConstConversion conv = convert_const_to_int(const_node, int_type);
    if (!conv.valid) {
      PG_RETURN_POINTER(NULL);
    }
    
    /* Apply transformation based on operator type and fractional status */
    if (op_type == OP_TYPE_EQ) {
      if (conv.has_fraction) {
        /* FR-015: Equality with fractional value is always FALSE */
        ret = (Node *) makeBoolConst(false, false);
      } else {
        /* FR-016: Equality with exact integer - transform to native operator */
        Oid native_op_oid = get_native_op_oid(OP_TYPE_EQ, int_type);
        ret = (Node *) build_native_opexpr(native_op_oid, var_node, conv.int_val,
                                            int_type, func->location, InvalidOid);
      }
    } else if (op_type == OP_TYPE_NE) {
      if (conv.has_fraction) {
        /* Inequality with fractional value is always TRUE */
        ret = (Node *) makeBoolConst(true, false);
      } else {
        /* Transform to native inequality operator */
        Oid native_op_oid = get_native_op_oid(OP_TYPE_NE, int_type);
        ret = (Node *) build_native_opexpr(native_op_oid, var_node, conv.int_val,
                                            int_type, func->location, InvalidOid);
      }
    } else {
      /* FR-017: Range boundary transformation */
      int64 adjusted_val;
      Oid native_op_oid = compute_range_transform(op_type, int_type, conv.int_val,
                                                   conv.has_fraction, &adjusted_val);
      ret = (Node *) build_native_opexpr(native_op_oid, var_node, adjusted_val,
                                          int_type, func->location, InvalidOid);
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
 * @brief Compare numeric value with int2 value
 * @param num Numeric value (left operand)
 * @param val int2 value (right operand)
 * @return -1 if num < val, 0 if num == val, 1 if num > val
 */
int
numeric_cmp_int2_internal(Numeric num, int16 val) {
  Numeric val_as_numeric = int64_to_numeric((int64)val);
  /* Use OID 1769 for numeric_cmp */
  Datum result = OidFunctionCall2Coll(1769, InvalidOid,
        NumericGetDatum(num), NumericGetDatum(val_as_numeric));
  return DatumGetInt32(result);
}

/**
 * @brief Compare numeric value with int4 value
 * @param num Numeric value (left operand)
 * @param val int4 value (right operand)
 * @return -1 if num < val, 0 if num == val, 1 if num > val
 */
int
numeric_cmp_int4_internal(Numeric num, int32 val) {
  Numeric val_as_numeric = int64_to_numeric((int64)val);
  Datum result = OidFunctionCall2Coll(1769, InvalidOid,
        NumericGetDatum(num), NumericGetDatum(val_as_numeric));
  return DatumGetInt32(result);
}

/**
 * @brief Compare numeric value with int8 value
 */
int
numeric_cmp_int8_internal(Numeric num, int64 val) {
  Numeric val_as_numeric = int64_to_numeric(val);
  Datum result = OidFunctionCall2Coll(1769, InvalidOid,
        NumericGetDatum(num), NumericGetDatum(val_as_numeric));
  return DatumGetInt32(result);
}

/**
 * @brief Compare float4 value with int2 value
 */
int
float4_cmp_int2_internal(float4 fval, int16 ival) {
  float4 ival_as_float4;
  
  /* Handle NaN - NaN never equals anything */
  if (isnan(fval)) {
    return 1;  /* Treat NaN as greater for consistent ordering */
  }
  
  /* Handle infinity */
  if (isinf(fval)) {
    return (fval > 0) ? 1 : -1;
  }
  
  /* Convert integer to float4 and compare */
  ival_as_float4 = (float4)ival;
  
  /* Check if representation lost precision */
  if ((int16)ival_as_float4 != ival) {
    /* This shouldn't happen for int2 values, but handle it */
    return (fval < ival) ? -1 : 1;
  }
  
  if (fval < ival_as_float4) return -1;
  if (fval > ival_as_float4) return 1;
  return 0;
}

/**
 * @brief Compare float4 value with int4 value
 */
int
float4_cmp_int4_internal(float4 fval, int32 ival) {
  float4 ival_as_float4;
  
  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;
  
  ival_as_float4 = (float4)ival;
  
  /* Check if integer exceeds float4 precision (>2^24) */
  if (labs((long)ival) > 16777216L) {
    /* Check if round-trip conversion preserves value */
    if ((int32)ival_as_float4 != ival) {
      /* Precision lost - can't be equal */
      /* Compare based on which direction the rounding went */
      if (fval < ival_as_float4) return -1;
      if (fval > ival_as_float4) return 1;
      /* If fval == ival_as_float4, they still can't be truly equal since ival_as_float4 != ival */
      return (ival_as_float4 < ival) ? -1 : 1;
    }
  }
  
  if (fval < ival_as_float4) return -1;
  if (fval > ival_as_float4) return 1;
  return 0;
}

/**
 * @brief Compare float4 value with int8 value
 */
int
float4_cmp_int8_internal(float4 fval, int64 ival) {
  float4 ival_as_float4;
  
  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;
  
  ival_as_float4 = (float4)ival;
  
  /* Check if integer exceeds float4 precision */
  if (llabs(ival) > 16777216LL) {
    if ((int64)ival_as_float4 != ival) {
      if (fval < ival_as_float4) return -1;
      if (fval > ival_as_float4) return 1;
      return (ival_as_float4 < ival) ? -1 : 1;
    }
  }
  
  if (fval < ival_as_float4) return -1;
  if (fval > ival_as_float4) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int2 value
 */
int
float8_cmp_int2_internal(float8 fval, int16 ival) {
  float8 ival_as_float8;
  
  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;
  
  ival_as_float8 = (float8)ival;
  
  if (fval < ival_as_float8) return -1;
  if (fval > ival_as_float8) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int4 value
 */
int
float8_cmp_int4_internal(float8 fval, int32 ival) {
  float8 ival_as_float8;
  
  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;
  
  ival_as_float8 = (float8)ival;
  
  if (fval < ival_as_float8) return -1;
  if (fval > ival_as_float8) return 1;
  return 0;
}

/**
 * @brief Compare float8 value with int8 value
 */
int
float8_cmp_int8_internal(float8 fval, int64 ival) {
  float8 ival_as_float8;
  float8 fval_rounded;
  
  if (isnan(fval)) return 1;
  if (isinf(fval)) return (fval > 0) ? 1 : -1;
  
  /* Check if float has fractional part - never equals integer */
  fval_rounded = trunc(fval);
  if (fval != fval_rounded) {
    return (fval < ival) ? -1 : 1;
  }
  
  ival_as_float8 = (float8)ival;
  
  /* float8 has 53-bit mantissa, check precision beyond 2^53 */
  if (llabs(ival) > 9007199254740992LL) {
    /* Integer is beyond exact float8 representation */
    if ((int64)ival_as_float8 != ival) {
      /* Integer cannot be exactly represented as float8 */
      /* For equality, this means they can never be equal */
      if (fval < ival_as_float8) return -1;
      if (fval > ival_as_float8) return 1;
      /* fval == ival_as_float8, but ival_as_float8 != ival due to rounding */
      /* Therefore fval != ival (precision loss detected) */
      return (ival_as_float8 < ival) ? -1 : 1;
    }
  }
  
  if (fval < ival_as_float8) return -1;
  if (fval > ival_as_float8) return 1;
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
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(numeric_eq_int4);
Datum
numeric_eq_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
}

PG_FUNCTION_INFO_V1(numeric_eq_int8);
Datum
numeric_eq_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
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
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(numeric_ne_int4);
Datum
numeric_ne_int4(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int32 val = PG_GETARG_INT32(1);
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
}

PG_FUNCTION_INFO_V1(numeric_ne_int8);
Datum
numeric_ne_int8(PG_FUNCTION_ARGS) {
  Numeric num = PG_GETARG_NUMERIC(0);
  int64 val = PG_GETARG_INT64(1);
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
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
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
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
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
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
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp == 0);
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
  int cmp = numeric_cmp_int2_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
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
  int cmp = numeric_cmp_int4_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
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
  int cmp = numeric_cmp_int8_internal(num, val);
  PG_RETURN_BOOL(cmp != 0);
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
 * Strategy: Cast integers to the higher-precision type (numeric/float) and
 * use the existing hash functions. This ensures:
 *   hash_int2_as_numeric(10) = hash_numeric(10.0)
 *   hash_int4_as_float8(10) = hashfloat8(10.0)
 */

/* Numeric hash wrappers - cast int to numeric, then hash */
PG_FUNCTION_INFO_V1(hash_int2_as_numeric);
Datum
hash_int2_as_numeric(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
}

PG_FUNCTION_INFO_V1(hash_int2_as_numeric_extended);
Datum
hash_int2_as_numeric_extended(PG_FUNCTION_ARGS) {
  int16 ival = PG_GETARG_INT16(0);
  int64 seed = PG_GETARG_INT64(1);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall2(hash_numeric_extended, NumericGetDatum(num), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int4_as_numeric);
Datum
hash_int4_as_numeric(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
}

PG_FUNCTION_INFO_V1(hash_int4_as_numeric_extended);
Datum
hash_int4_as_numeric_extended(PG_FUNCTION_ARGS) {
  int32 ival = PG_GETARG_INT32(0);
  int64 seed = PG_GETARG_INT64(1);
  Numeric num = int64_to_numeric((int64) ival);
  return DirectFunctionCall2(hash_numeric_extended, NumericGetDatum(num), Int64GetDatum(seed));
}

PG_FUNCTION_INFO_V1(hash_int8_as_numeric);
Datum
hash_int8_as_numeric(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  Numeric num = int64_to_numeric(ival);
  return DirectFunctionCall1(hash_numeric, NumericGetDatum(num));
}

PG_FUNCTION_INFO_V1(hash_int8_as_numeric_extended);
Datum
hash_int8_as_numeric_extended(PG_FUNCTION_ARGS) {
  int64 ival = PG_GETARG_INT64(0);
  int64 seed = PG_GETARG_INT64(1);
  Numeric num = int64_to_numeric(ival);
  return DirectFunctionCall2(hash_numeric_extended, NumericGetDatum(num), Int64GetDatum(seed));
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

