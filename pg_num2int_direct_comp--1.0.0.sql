/*
 * Copyright (c) 2025 Dave Sharpe
 * 
 * SPDX-License-Identifier: MIT
 * See LICENSE file for full license text.
 */

/**
 * pg_num2int_direct_comp extension installation script
 * Version 1.0.0
 *
 * This file was developed with assistance from AI tools.
 */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_num2int_direct_comp" to load this file. \quit

-- Support function for index optimization
CREATE OR REPLACE FUNCTION num2int_support(internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'num2int_support'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

COMMENT ON FUNCTION num2int_support(internal) IS
'Support function for index optimization of numeric/integer comparisons';

-- ============================================================================
-- Phase 3: Equality and Inequality Operators (User Story 1 - MVP)
-- ============================================================================

-- Numeric × Integer Equality Functions
CREATE OR REPLACE FUNCTION numeric_eq_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_eq_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_eq_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_eq_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_eq_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_eq_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_eq_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_eq_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Float4 × Integer Equality Functions
CREATE OR REPLACE FUNCTION float4_eq_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_eq_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_eq_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_eq_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_eq_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_eq_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Float8 × Integer Equality Functions
CREATE OR REPLACE FUNCTION float8_eq_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_eq_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_eq_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_eq_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_eq_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_eq_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Numeric × Integer Inequality Functions
CREATE OR REPLACE FUNCTION numeric_ne_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ne_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_ne_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ne_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_ne_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ne_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Float4 × Integer Inequality Functions
CREATE OR REPLACE FUNCTION float4_ne_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ne_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_ne_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ne_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_ne_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ne_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Float8 × Integer Inequality Functions
CREATE OR REPLACE FUNCTION float8_ne_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ne_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_ne_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ne_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_ne_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ne_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- ============================================================================
-- Equality Operators (=)
-- ============================================================================

-- Numeric × Integer Equality Operators
CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_eq_int2,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

-- Float4 × Integer Equality Operators
CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_eq_int2,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

-- Float8 × Integer Equality Operators
CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_eq_int2,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

-- ============================================================================
-- Inequality Operators (<>)
-- ============================================================================

-- Numeric × Integer Inequality Operators
CREATE OPERATOR <> (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_ne_int2,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_ne_int4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_ne_int8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

-- Float4 × Integer Inequality Operators
CREATE OPERATOR <> (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_ne_int2,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_ne_int4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_ne_int8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

-- Float8 × Integer Inequality Operators
CREATE OPERATOR <> (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_ne_int2,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_ne_int4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_ne_int8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

-- ============================================================================
-- Commutator Operators (Integer × Numeric/Float reverse direction)
-- ============================================================================

-- Commutator Equality Functions
CREATE OR REPLACE FUNCTION int2_eq_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_eq_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_eq_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_eq_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_eq_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_eq_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_eq_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_eq_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_eq_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_eq_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_eq_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_eq_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_eq_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_eq_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_eq_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_eq_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Commutator Inequality Functions
CREATE OR REPLACE FUNCTION int2_ne_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ne_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_ne_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ne_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_ne_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ne_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ne_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ne_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ne_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ne_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ne_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ne_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ne_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ne_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ne_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ne_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ne_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ne_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Commutator Equality Operators
CREATE OPERATOR = (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);

-- Commutator Inequality Operators
CREATE OPERATOR <> (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_ne_numeric,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_ne_float4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_ne_float8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_ne_numeric,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_ne_float4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_ne_float8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_ne_numeric,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_ne_float4,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);

CREATE OPERATOR <> (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_ne_float8,
  COMMUTATOR = <>,
  NEGATOR = =,
  RESTRICT = neqsel,
  JOIN = neqjoinsel
);


-- ============================================================================
-- Phase 5: Range Comparison Operators (User Story 2)
-- ============================================================================

-- Less Than (<) Functions
CREATE OR REPLACE FUNCTION numeric_lt_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_lt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_lt_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_lt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_lt_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_lt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_lt_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_lt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_lt_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_lt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_lt_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_lt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_lt_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_lt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_lt_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_lt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_lt_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_lt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Greater Than (>) Functions
CREATE OR REPLACE FUNCTION numeric_gt_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_gt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_gt_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_gt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_gt_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_gt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_gt_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_gt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_gt_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_gt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_gt_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_gt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_gt_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_gt_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_gt_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_gt_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_gt_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_gt_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Less Than or Equal (<=) Functions
CREATE OR REPLACE FUNCTION numeric_le_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_le_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_le_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_le_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_le_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_le_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_le_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_le_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_le_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_le_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_le_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_le_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_le_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_le_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_le_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_le_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_le_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_le_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Greater Than or Equal (>=) Functions
CREATE OR REPLACE FUNCTION numeric_ge_int2(numeric, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ge_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_ge_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ge_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION numeric_ge_int8(numeric, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_ge_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_ge_int2(float4, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ge_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_ge_int4(float4, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ge_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float4_ge_int8(float4, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float4_ge_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_ge_int2(float8, int2)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ge_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_ge_int4(float8, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ge_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION float8_ge_int8(float8, int8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'float8_ge_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Less Than (<) Operators
CREATE OPERATOR < (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_lt_int2,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_lt_int4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_lt_int8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_lt_int2,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_lt_int4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_lt_int8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_lt_int2,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_lt_int4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_lt_int8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

-- Greater Than (>) Operators
CREATE OPERATOR > (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_gt_int2,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_gt_int4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_gt_int8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_gt_int2,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_gt_int4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_gt_int8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_gt_int2,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_gt_int4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_gt_int8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

-- Less Than or Equal (<=) Operators
CREATE OPERATOR <= (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_le_int2,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_le_int4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_le_int8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_le_int2,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_le_int4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_le_int8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_le_int2,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_le_int4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_le_int8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

-- Greater Than or Equal (>=) Operators
CREATE OPERATOR >= (
  LEFTARG = numeric,
  RIGHTARG = int2,
  FUNCTION = numeric_ge_int2,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_ge_int4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_ge_int8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_ge_int2,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_ge_int4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_ge_int8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_ge_int2,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_ge_int4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_ge_int8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);


-- ============================================================================
-- Phase 5: Commutator Range Operators (int X op numeric/float)
-- ============================================================================

-- Less Than (<) Commutator Functions
CREATE OR REPLACE FUNCTION int2_lt_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_lt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_lt_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_lt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_lt_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_lt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_lt_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_lt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_lt_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_lt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_lt_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_lt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_lt_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_lt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_lt_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_lt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_lt_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_lt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Greater Than (>) Commutator Functions
CREATE OR REPLACE FUNCTION int2_gt_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_gt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_gt_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_gt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_gt_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_gt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_gt_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_gt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_gt_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_gt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_gt_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_gt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_gt_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_gt_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_gt_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_gt_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_gt_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_gt_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Less Than or Equal (<=) Commutator Functions
CREATE OR REPLACE FUNCTION int2_le_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_le_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_le_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_le_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_le_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_le_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_le_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_le_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_le_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_le_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_le_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_le_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_le_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_le_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_le_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_le_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_le_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_le_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Greater Than or Equal (>=) Commutator Functions
CREATE OR REPLACE FUNCTION int2_ge_numeric(int2, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ge_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_ge_float4(int2, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ge_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int2_ge_float8(int2, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int2_ge_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ge_numeric(int4, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ge_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ge_float4(int4, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ge_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int4_ge_float8(int4, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int4_ge_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ge_numeric(int8, numeric)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ge_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ge_float4(int8, float4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ge_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

CREATE OR REPLACE FUNCTION int8_ge_float8(int8, float8)
RETURNS boolean
AS 'MODULE_PATHNAME', 'int8_ge_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE
SUPPORT num2int_support;

-- Less Than (<) Commutator Operators
CREATE OPERATOR < (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_lt_numeric,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_lt_float4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_lt_float8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_lt_numeric,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_lt_float4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_lt_float8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_lt_numeric,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_lt_float4,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

CREATE OPERATOR < (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_lt_float8,
  COMMUTATOR = >,
  NEGATOR = >=,
  RESTRICT = scalarltsel,
  JOIN = scalarltjoinsel
);

-- Greater Than (>) Commutator Operators
CREATE OPERATOR > (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_gt_numeric,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_gt_float4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_gt_float8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_gt_numeric,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_gt_float4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_gt_float8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_gt_numeric,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_gt_float4,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

CREATE OPERATOR > (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_gt_float8,
  COMMUTATOR = <,
  NEGATOR = <=,
  RESTRICT = scalargtsel,
  JOIN = scalargtjoinsel
);

-- Less Than or Equal (<=) Commutator Operators
CREATE OPERATOR <= (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_le_numeric,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_le_float4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_le_float8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_le_numeric,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_le_float4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_le_float8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_le_numeric,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_le_float4,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

CREATE OPERATOR <= (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_le_float8,
  COMMUTATOR = >=,
  NEGATOR = >,
  RESTRICT = scalarlesel,
  JOIN = scalarlejoinsel
);

-- Greater Than or Equal (>=) Commutator Operators
CREATE OPERATOR >= (
  LEFTARG = int2,
  RIGHTARG = numeric,
  FUNCTION = int2_ge_numeric,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_ge_float4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_ge_float8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_ge_numeric,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_ge_float4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_ge_float8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_ge_numeric,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_ge_float4,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

CREATE OPERATOR >= (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_ge_float8,
  COMMUTATOR = <=,
  NEGATOR = <,
  RESTRICT = scalargesel,
  JOIN = scalargejoinsel
);

