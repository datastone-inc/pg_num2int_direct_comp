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
  
  cache->initialized = true;
}

/**
 * @brief Generic support function for index optimization
 * @param fcinfo Function call context
 * @return Node pointer or NULL
 *
 * Implements SupportRequestIndexCondition to enable btree index usage
 * for exact comparison predicates. Inspects operator OID, validates
 * constant operand is within range, and transforms predicate for index scan.
 */
PG_FUNCTION_INFO_V1(num2int_support);
Datum
num2int_support(PG_FUNCTION_ARGS) {
  Node *rawreq = (Node *) PG_GETARG_POINTER(0);
  Node *ret = NULL;
  
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    SupportRequestIndexCondition *req = (SupportRequestIndexCondition *) rawreq;
    
    /* Initialize output fields */
    req->lossy = false;  /* Our transformation is exact, not lossy */
    
    /* Initialize OID cache on first use */
    init_oid_cache(&oid_cache);
    
    /* Check if this is an OpExpr (operator clause) */
    if (is_opclause(req->node)) {
      OpExpr *clause = (OpExpr *) req->node;
      Oid opno = clause->opno;
      Node *leftop;
      Node *rightop;
      Var *var_node = NULL;
      Const *const_node = NULL;
      
      /* Extract left and right operands */
      if (list_length(clause->args) != 2) {
        PG_RETURN_POINTER(NULL);
      }
      
      leftop = (Node *) linitial(clause->args);
      rightop = (Node *) lsecond(clause->args);
      
      /* Identify Var and Const positions */
      if (IsA(leftop, Const) && IsA(rightop, Var)) {
        const_node = (Const *) leftop;
        var_node = (Var *) rightop;
      } else if (IsA(leftop, Var) && IsA(rightop, Const)) {
        var_node = (Var *) leftop;
        const_node = (Const *) rightop;
      } else {
        /* Not Var + Const pattern */
        PG_RETURN_POINTER(NULL);
      }
      
      /* Check if this is one of our equality operators */
      if (opno == oid_cache.numeric_eq_int2_oid ||
          opno == oid_cache.numeric_eq_int4_oid ||
          opno == oid_cache.numeric_eq_int8_oid ||
          opno == oid_cache.float4_eq_int2_oid ||
          opno == oid_cache.float4_eq_int4_oid ||
          opno == oid_cache.float4_eq_int8_oid ||
          opno == oid_cache.float8_eq_int2_oid ||
          opno == oid_cache.float8_eq_int4_oid ||
          opno == oid_cache.float8_eq_int8_oid ||
          opno == oid_cache.int2_eq_numeric_oid ||
          opno == oid_cache.int2_eq_float4_oid ||
          opno == oid_cache.int2_eq_float8_oid ||
          opno == oid_cache.int4_eq_numeric_oid ||
          opno == oid_cache.int4_eq_float4_oid ||
          opno == oid_cache.int4_eq_float8_oid ||
          opno == oid_cache.int8_eq_numeric_oid ||
          opno == oid_cache.int8_eq_float4_oid ||
          opno == oid_cache.int8_eq_float8_oid) {
        
        /* Transform: convert numeric/float const to int const */
        /* Create new OpExpr using standard int = int operator */
        Const *new_const = NULL;
        OpExpr *new_clause = NULL;
        Var *new_var;
        Oid standard_eq_oid;
        Oid int_type = var_node->vartype;
        int64 int_val;
        bool is_valid = false;
        
        /* Convert const to integer value based on type */
        if (const_node->constisnull) {
          PG_RETURN_POINTER(NULL);
        }
        
        if (const_node->consttype == NUMERICOID) {
          Numeric num = DatumGetNumeric(const_node->constvalue);
          
          /* Skip NaN/Inf */
          if (numeric_is_nan(num) || numeric_is_inf(num)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Convert using PostgreSQL's built-in cast functions: int2(numeric), int4(numeric), int8(numeric) */
          PG_TRY();
          {
            if (int_type == INT2OID) {
              /* Use int2(numeric) cast - OID 1783 */
              Datum result = OidFunctionCall1(1783, NumericGetDatum(num));
              int_val = DatumGetInt16(result);
              is_valid = true;
            } else if (int_type == INT4OID) {
              /* Use int4(numeric) cast - OID 1744 */
              Datum result = OidFunctionCall1(1744, NumericGetDatum(num));
              int_val = DatumGetInt32(result);
              is_valid = true;
            } else if (int_type == INT8OID) {
              /* Use int8(numeric) cast - OID 1779 */
              Datum result = OidFunctionCall1(1779, NumericGetDatum(num));
              int_val = DatumGetInt64(result);
              is_valid = true;
            }
          }
          PG_CATCH();
          {
            /* Conversion failed - out of range */
            FlushErrorState();
            is_valid = false;
          }
          PG_END_TRY();
          
          if (!is_valid) {
            /* Conversion failed - out of range */
            PG_RETURN_POINTER(NULL);
          }
        } else if (const_node->consttype == FLOAT4OID) {
          float4 fval = DatumGetFloat4(const_node->constvalue);
          
          if (isnan(fval) || isinf(fval)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Check range and convert */
          if (int_type == INT2OID) {
            if (fval < PG_INT16_MIN || fval > PG_INT16_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int16) fval;
            is_valid = ((float4) int_val == fval);
          } else if (int_type == INT4OID) {
            if (fval < PG_INT32_MIN || fval > PG_INT32_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int32) fval;
            is_valid = ((float4) int_val == fval);
          } else if (int_type == INT8OID) {
            int_val = (int64) fval;
            is_valid = ((float4) int_val == fval);
          }
          
          if (!is_valid) {
            /* Has fractional part or precision loss */
            PG_RETURN_POINTER(NULL);
          }
        } else if (const_node->consttype == FLOAT8OID) {
          float8 dval = DatumGetFloat8(const_node->constvalue);
          
          if (isnan(dval) || isinf(dval)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Check range and convert */
          if (int_type == INT2OID) {
            if (dval < PG_INT16_MIN || dval > PG_INT16_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int16) dval;
            is_valid = ((float8) int_val == dval);
          } else if (int_type == INT4OID) {
            if (dval < PG_INT32_MIN || dval > PG_INT32_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int32) dval;
            is_valid = ((float8) int_val == dval);
          } else if (int_type == INT8OID) {
            int_val = (int64) dval;
            is_valid = ((float8) int_val == dval);
          }
          
          if (!is_valid) {
            /* Has fractional part or precision loss */
            PG_RETURN_POINTER(NULL);
          }
        } else {
          /* Unknown type */
          PG_RETURN_POINTER(NULL);
        }
        
        /* Create new integer Const node */
        if (int_type == INT2OID) {
          new_const = makeConst(INT2OID, -1, InvalidOid, sizeof(int16),
                               Int16GetDatum((int16) int_val), false, true);
          standard_eq_oid = NUM2INT_INT2EQ_OID;
        } else if (int_type == INT4OID) {
          new_const = makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                               Int32GetDatum((int32) int_val), false, true);
          standard_eq_oid = NUM2INT_INT4EQ_OID;
        } else if (int_type == INT8OID) {
          new_const = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
                               Int64GetDatum(int_val), false, true);
          standard_eq_oid = NUM2INT_INT8EQ_OID;
        } else {
          PG_RETURN_POINTER(NULL);
        }
        
        /* Build new OpExpr: ALWAYS put Var on left, Const on right for btree index */
        /* Copy the Var node to avoid sharing pointers with original clause */
        new_var = (Var *) copyObject(var_node);
        
        new_clause = (OpExpr *) make_opclause(standard_eq_oid,
                                               BOOLOID,
                                               false,
                                               (Expr *) new_var,
                                               (Expr *) new_const,
                                               InvalidOid,
                                               InvalidOid);
        
        /* Set collation and location from original */
        new_clause->inputcollid = clause->inputcollid;
        new_clause->location = clause->location;
        
        /* Mark this transformation as NOT lossy - it's exact */
        req->lossy = false;
        
        /* Return a LIST of conditions (even though we only have one) */
        ret = (Node *) list_make1(new_clause);
      }
      /* Check if this is one of our range comparison operators (<, >, <=, >=) */
      else if (opno == oid_cache.numeric_lt_int2_oid ||
               opno == oid_cache.numeric_lt_int4_oid ||
               opno == oid_cache.numeric_lt_int8_oid ||
               opno == oid_cache.float4_lt_int2_oid ||
               opno == oid_cache.float4_lt_int4_oid ||
               opno == oid_cache.float4_lt_int8_oid ||
               opno == oid_cache.float8_lt_int2_oid ||
               opno == oid_cache.float8_lt_int4_oid ||
               opno == oid_cache.float8_lt_int8_oid ||
               opno == oid_cache.numeric_gt_int2_oid ||
               opno == oid_cache.numeric_gt_int4_oid ||
               opno == oid_cache.numeric_gt_int8_oid ||
               opno == oid_cache.float4_gt_int2_oid ||
               opno == oid_cache.float4_gt_int4_oid ||
               opno == oid_cache.float4_gt_int8_oid ||
               opno == oid_cache.float8_gt_int2_oid ||
               opno == oid_cache.float8_gt_int4_oid ||
               opno == oid_cache.float8_gt_int8_oid ||
               opno == oid_cache.numeric_le_int2_oid ||
               opno == oid_cache.numeric_le_int4_oid ||
               opno == oid_cache.numeric_le_int8_oid ||
               opno == oid_cache.float4_le_int2_oid ||
               opno == oid_cache.float4_le_int4_oid ||
               opno == oid_cache.float4_le_int8_oid ||
               opno == oid_cache.float8_le_int2_oid ||
               opno == oid_cache.float8_le_int4_oid ||
               opno == oid_cache.float8_le_int8_oid ||
               opno == oid_cache.numeric_ge_int2_oid ||
               opno == oid_cache.numeric_ge_int4_oid ||
               opno == oid_cache.numeric_ge_int8_oid ||
               opno == oid_cache.float4_ge_int2_oid ||
               opno == oid_cache.float4_ge_int4_oid ||
               opno == oid_cache.float4_ge_int8_oid ||
               opno == oid_cache.float8_ge_int2_oid ||
               opno == oid_cache.float8_ge_int4_oid ||
               opno == oid_cache.float8_ge_int8_oid) {
        
        /* Transform range operators with intelligent rounding
         * Examples:
         *   intcol > 10.5::numeric  =>  intcol >= 11   (round up)
         *   intcol < 10.5::numeric  =>  intcol <= 10   (round down)
         *   intcol >= 10.5::numeric =>  intcol >= 11   (round up)
         *   intcol <= 10.5::numeric =>  intcol <= 10   (round down)
         */
        Const *new_const = NULL;
        OpExpr *new_clause = NULL;
        Oid standard_op_oid;
        Oid int_type = var_node->vartype;
        int64 int_val;
        bool has_fraction = false;
        bool is_lt, is_gt, is_le, is_ge;
        Var *new_var;
        
        if (const_node->constisnull) {
          PG_RETURN_POINTER(NULL);
        }
        
        /* Determine which operator we're dealing with */
        is_lt = (opno == oid_cache.numeric_lt_int2_oid ||
                 opno == oid_cache.numeric_lt_int4_oid ||
                 opno == oid_cache.numeric_lt_int8_oid ||
                 opno == oid_cache.float4_lt_int2_oid ||
                 opno == oid_cache.float4_lt_int4_oid ||
                 opno == oid_cache.float4_lt_int8_oid ||
                 opno == oid_cache.float8_lt_int2_oid ||
                 opno == oid_cache.float8_lt_int4_oid ||
                 opno == oid_cache.float8_lt_int8_oid);
        
        is_gt = (opno == oid_cache.numeric_gt_int2_oid ||
                 opno == oid_cache.numeric_gt_int4_oid ||
                 opno == oid_cache.numeric_gt_int8_oid ||
                 opno == oid_cache.float4_gt_int2_oid ||
                 opno == oid_cache.float4_gt_int4_oid ||
                 opno == oid_cache.float4_gt_int8_oid ||
                 opno == oid_cache.float8_gt_int2_oid ||
                 opno == oid_cache.float8_gt_int4_oid ||
                 opno == oid_cache.float8_gt_int8_oid);
        
        is_le = (opno == oid_cache.numeric_le_int2_oid ||
                 opno == oid_cache.numeric_le_int4_oid ||
                 opno == oid_cache.numeric_le_int8_oid ||
                 opno == oid_cache.float4_le_int2_oid ||
                 opno == oid_cache.float4_le_int4_oid ||
                 opno == oid_cache.float4_le_int8_oid ||
                 opno == oid_cache.float8_le_int2_oid ||
                 opno == oid_cache.float8_le_int4_oid ||
                 opno == oid_cache.float8_le_int8_oid);
        
        is_ge = (opno == oid_cache.numeric_ge_int2_oid ||
                 opno == oid_cache.numeric_ge_int4_oid ||
                 opno == oid_cache.numeric_ge_int8_oid ||
                 opno == oid_cache.float4_ge_int2_oid ||
                 opno == oid_cache.float4_ge_int4_oid ||
                 opno == oid_cache.float4_ge_int8_oid ||
                 opno == oid_cache.float8_ge_int2_oid ||
                 opno == oid_cache.float8_ge_int4_oid ||
                 opno == oid_cache.float8_ge_int8_oid);
        
        /* Convert const to integer value and detect fractional part */
        if (const_node->consttype == NUMERICOID) {
          Numeric num = DatumGetNumeric(const_node->constvalue);
          Numeric trunc_num;
          
          if (numeric_is_nan(num) || numeric_is_inf(num)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Check for fractional part: trunc(num) returns integer part */
          trunc_num = DatumGetNumeric(OidFunctionCall1(1916 /* numeric_trunc */, NumericGetDatum(num)));
          has_fraction = !DatumGetBool(OidFunctionCall2(1752 /* numeric_eq */,
                                                         NumericGetDatum(num),
                                                         NumericGetDatum(trunc_num)));
          
          /* Convert to integer using appropriate cast */
          PG_TRY();
          {
            if (int_type == INT2OID) {
              Datum result = OidFunctionCall1(1783, NumericGetDatum(num));
              int_val = DatumGetInt16(result);
            } else if (int_type == INT4OID) {
              Datum result = OidFunctionCall1(1744, NumericGetDatum(num));
              int_val = DatumGetInt32(result);
            } else if (int_type == INT8OID) {
              Datum result = OidFunctionCall1(1779, NumericGetDatum(num));
              int_val = DatumGetInt64(result);
            } else {
              PG_RETURN_POINTER(NULL);
            }
          }
          PG_CATCH();
          {
            FlushErrorState();
            PG_RETURN_POINTER(NULL);
          }
          PG_END_TRY();
          
        } else if (const_node->consttype == FLOAT4OID) {
          float4 fval = DatumGetFloat4(const_node->constvalue);
          float4 floor_val;
          
          if (isnan(fval) || isinf(fval)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Check for fractional part */
          floor_val = floorf(fval);
          has_fraction = (fval != floor_val);
          
          /* Convert to integer */
          if (int_type == INT2OID) {
            if (fval < PG_INT16_MIN || fval > PG_INT16_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int16) fval;
          } else if (int_type == INT4OID) {
            if (fval < PG_INT32_MIN || fval > PG_INT32_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int32) fval;
          } else if (int_type == INT8OID) {
            int_val = (int64) fval;
          } else {
            PG_RETURN_POINTER(NULL);
          }
          
        } else if (const_node->consttype == FLOAT8OID) {
          float8 dval = DatumGetFloat8(const_node->constvalue);
          float8 floor_val;
          
          if (isnan(dval) || isinf(dval)) {
            PG_RETURN_POINTER(NULL);
          }
          
          /* Check for fractional part */
          floor_val = floor(dval);
          has_fraction = (dval != floor_val);
          
          /* Convert to integer */
          if (int_type == INT2OID) {
            if (dval < PG_INT16_MIN || dval > PG_INT16_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int16) dval;
          } else if (int_type == INT4OID) {
            if (dval < PG_INT32_MIN || dval > PG_INT32_MAX) {
              PG_RETURN_POINTER(NULL);
            }
            int_val = (int32) dval;
          } else if (int_type == INT8OID) {
            int_val = (int64) dval;
          } else {
            PG_RETURN_POINTER(NULL);
          }
        } else {
          PG_RETURN_POINTER(NULL);
        }
        
        /* Apply rounding logic based on operator and fractional part */
        if (has_fraction) {
          if (is_gt || is_ge) {
            /* For > or >=, round up (add 1 to truncated value) */
            int_val++;
            /* Use >= operator for both > and >= */
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2GE_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4GE_OID;
            } else {
              standard_op_oid = NUM2INT_INT8GE_OID;
            }
          } else if (is_lt || is_le) {
            /* For < or <=, use truncated value with <= operator */
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2LE_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4LE_OID;
            } else {
              standard_op_oid = NUM2INT_INT8LE_OID;
            }
          } else {
            PG_RETURN_POINTER(NULL);
          }
        } else {
          /* No fractional part - use the original operator */
          if (is_lt) {
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2LT_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4LT_OID;
            } else {
              standard_op_oid = NUM2INT_INT8LT_OID;
            }
          } else if (is_gt) {
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2GT_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4GT_OID;
            } else {
              standard_op_oid = NUM2INT_INT8GT_OID;
            }
          } else if (is_le) {
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2LE_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4LE_OID;
            } else {
              standard_op_oid = NUM2INT_INT8LE_OID;
            }
          } else if (is_ge) {
            if (int_type == INT2OID) {
              standard_op_oid = NUM2INT_INT2GE_OID;
            } else if (int_type == INT4OID) {
              standard_op_oid = NUM2INT_INT4GE_OID;
            } else {
              standard_op_oid = NUM2INT_INT8GE_OID;
            }
          } else {
            PG_RETURN_POINTER(NULL);
          }
        }
        
        /* Create new integer Const node */
        if (int_type == INT2OID) {
          new_const = makeConst(INT2OID, -1, InvalidOid, sizeof(int16),
                               Int16GetDatum((int16) int_val), false, true);
        } else if (int_type == INT4OID) {
          new_const = makeConst(INT4OID, -1, InvalidOid, sizeof(int32),
                               Int32GetDatum((int32) int_val), false, true);
        } else if (int_type == INT8OID) {
          new_const = makeConst(INT8OID, -1, InvalidOid, sizeof(int64),
                               Int64GetDatum(int_val), false, true);
        } else {
          PG_RETURN_POINTER(NULL);
        }
        
        /* Build new OpExpr: ALWAYS put Var on left, Const on right for btree index */
        new_var = (Var *) copyObject(var_node);
        
        new_clause = (OpExpr *) make_opclause(standard_op_oid,
                                               BOOLOID,
                                               false,
                                               (Expr *) new_var,
                                               (Expr *) new_const,
                                               InvalidOid,
                                               InvalidOid);
        
        new_clause->inputcollid = clause->inputcollid;
        new_clause->location = clause->location;
        
        /* Mark as NOT lossy - transformation is exact */
        req->lossy = false;
        
        ret = (Node *) list_make1(new_clause);
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
 * They will be implemented in Phase 3 (User Story 1)
 */

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

