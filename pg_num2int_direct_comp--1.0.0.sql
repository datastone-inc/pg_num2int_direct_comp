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
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int4,
  FUNCTION = numeric_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

CREATE OPERATOR = (
  LEFTARG = numeric,
  RIGHTARG = int8,
  FUNCTION = numeric_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

-- Float4 × Integer Equality Operators
CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int2,
  FUNCTION = float4_eq_int2,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int4,
  FUNCTION = float4_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = float4,
  RIGHTARG = int8,
  FUNCTION = float4_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

-- Float8 × Integer Equality Operators
CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int2,
  FUNCTION = float8_eq_int2,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int4,
  FUNCTION = float8_eq_int4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = float8,
  RIGHTARG = int8,
  FUNCTION = float8_eq_int8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
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
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

CREATE OPERATOR = (
  LEFTARG = int2,
  RIGHTARG = float4,
  FUNCTION = int2_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = int2,
  RIGHTARG = float8,
  FUNCTION = int2_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = float4,
  FUNCTION = int4_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = float8,
  FUNCTION = int4_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = numeric,
  FUNCTION = int8_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES,
  MERGES
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = float4,
  FUNCTION = int8_eq_float4,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
);

CREATE OPERATOR = (
  LEFTARG = int8,
  RIGHTARG = float8,
  FUNCTION = int8_eq_float8,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel,
  HASHES
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

-- ============================================================================
-- Btree Operator Family Registration for Merge Join Support
-- ============================================================================

-- First, create SQL-callable wrappers for btree comparison functions
-- These are needed as support functions in btree operator families

CREATE OR REPLACE FUNCTION numeric_cmp_int2(numeric, int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'numeric_cmp_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION numeric_cmp_int4(numeric, int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'numeric_cmp_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION numeric_cmp_int8(numeric, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'numeric_cmp_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Reverse comparison functions for integer_ops btree family
-- These are called with (int, numeric) argument order
CREATE OR REPLACE FUNCTION int2_cmp_numeric(int2, numeric)
RETURNS int4
AS 'MODULE_PATHNAME', 'int2_cmp_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION int4_cmp_numeric(int4, numeric)
RETURNS int4
AS 'MODULE_PATHNAME', 'int4_cmp_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION int8_cmp_numeric(int8, numeric)
RETURNS int4
AS 'MODULE_PATHNAME', 'int8_cmp_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float4_cmp_int2(float4, int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'float4_cmp_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float4_cmp_int4(float4, int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'float4_cmp_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float4_cmp_int8(float4, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'float4_cmp_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float8_cmp_int2(float8, int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'float8_cmp_int2'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float8_cmp_int4(float8, int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'float8_cmp_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION float8_cmp_int8(float8, int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'float8_cmp_int8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- ============================================================================
-- Hash Support Functions (For Hash Joins)
-- ============================================================================
-- These functions ensure consistent hashing across types by casting integers
-- to the higher-precision type (numeric/float) before hashing.
-- This guarantees: hash(10::int4) = hash(10.0::numeric)

-- Hash int as numeric (for numeric_ops hash family)
CREATE OR REPLACE FUNCTION hash_int2_as_numeric(int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int2_as_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int2_as_numeric_extended(int2, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int2_as_numeric_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_numeric(int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int4_as_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_numeric_extended(int4, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int4_as_numeric_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_numeric(int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int8_as_numeric'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_numeric_extended(int8, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int8_as_numeric_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Hash int as float4 (for float_ops hash family)
CREATE OR REPLACE FUNCTION hash_int2_as_float4(int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int2_as_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int2_as_float4_extended(int2, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int2_as_float4_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_float4(int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int4_as_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_float4_extended(int4, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int4_as_float4_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_float4(int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int8_as_float4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_float4_extended(int8, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int8_as_float4_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Hash int as float8 (for float_ops hash family)
CREATE OR REPLACE FUNCTION hash_int2_as_float8(int2)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int2_as_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int2_as_float8_extended(int2, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int2_as_float8_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_float8(int4)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int4_as_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int4_as_float8_extended(int4, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int4_as_float8_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_float8(int8)
RETURNS int4
AS 'MODULE_PATHNAME', 'hash_int8_as_float8'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION hash_int8_as_float8_extended(int8, int8)
RETURNS int8
AS 'MODULE_PATHNAME', 'hash_int8_as_float8_extended'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Add numeric × int operators to numeric_ops btree family
-- This enables merge join optimization and transitive inference
--
-- CRITICAL: We must also add integer comparison operators so PostgreSQL can
-- compare integer values within numeric_ops context (e.g., during merge join sort).
ALTER OPERATOR FAMILY numeric_ops USING btree ADD
  -- Same-type integer comparisons (required for merge join sorting)
  -- int2 comparisons
  FUNCTION 1 (int2, int2) btint2cmp(int2, int2),
  OPERATOR 1 < (int2, int2),
  OPERATOR 2 <= (int2, int2),
  OPERATOR 3 = (int2, int2),
  OPERATOR 4 >= (int2, int2),
  OPERATOR 5 > (int2, int2),
  
  -- int4 comparisons
  FUNCTION 1 (int4, int4) btint4cmp(int4, int4),
  OPERATOR 1 < (int4, int4),
  OPERATOR 2 <= (int4, int4),
  OPERATOR 3 = (int4, int4),
  OPERATOR 4 >= (int4, int4),
  OPERATOR 5 > (int4, int4),
  
  -- int8 comparisons
  FUNCTION 1 (int8, int8) btint8cmp(int8, int8),
  OPERATOR 1 < (int8, int8),
  OPERATOR 2 <= (int8, int8),
  OPERATOR 3 = (int8, int8),
  OPERATOR 4 >= (int8, int8),
  OPERATOR 5 > (int8, int8),
  
  -- Cross-integer comparisons (int2 vs int4, int2 vs int8, int4 vs int8)
  FUNCTION 1 (int2, int4) btint24cmp(int2, int4),
  OPERATOR 1 < (int2, int4),
  OPERATOR 2 <= (int2, int4),
  OPERATOR 3 = (int2, int4),
  OPERATOR 4 >= (int2, int4),
  OPERATOR 5 > (int2, int4),
  
  FUNCTION 1 (int4, int2) btint42cmp(int4, int2),
  OPERATOR 1 < (int4, int2),
  OPERATOR 2 <= (int4, int2),
  OPERATOR 3 = (int4, int2),
  OPERATOR 4 >= (int4, int2),
  OPERATOR 5 > (int4, int2),
  
  FUNCTION 1 (int2, int8) btint28cmp(int2, int8),
  OPERATOR 1 < (int2, int8),
  OPERATOR 2 <= (int2, int8),
  OPERATOR 3 = (int2, int8),
  OPERATOR 4 >= (int2, int8),
  OPERATOR 5 > (int2, int8),
  
  FUNCTION 1 (int8, int2) btint82cmp(int8, int2),
  OPERATOR 1 < (int8, int2),
  OPERATOR 2 <= (int8, int2),
  OPERATOR 3 = (int8, int2),
  OPERATOR 4 >= (int8, int2),
  OPERATOR 5 > (int8, int2),
  
  FUNCTION 1 (int4, int8) btint48cmp(int4, int8),
  OPERATOR 1 < (int4, int8),
  OPERATOR 2 <= (int4, int8),
  OPERATOR 3 = (int4, int8),
  OPERATOR 4 >= (int4, int8),
  OPERATOR 5 > (int4, int8),
  
  FUNCTION 1 (int8, int4) btint84cmp(int8, int4),
  OPERATOR 1 < (int8, int4),
  OPERATOR 2 <= (int8, int4),
  OPERATOR 3 = (int8, int4),
  OPERATOR 4 >= (int8, int4),
  OPERATOR 5 > (int8, int4),
  
  -- numeric <op> int2 with support function
  FUNCTION 1 (numeric, int2) numeric_cmp_int2(numeric, int2),
  OPERATOR 1 < (numeric, int2),
  OPERATOR 2 <= (numeric, int2),
  OPERATOR 3 = (numeric, int2),
  OPERATOR 4 >= (numeric, int2),
  OPERATOR 5 > (numeric, int2),
  
  -- numeric <op> int4 with support function
  FUNCTION 1 (numeric, int4) numeric_cmp_int4(numeric, int4),
  OPERATOR 1 < (numeric, int4),
  OPERATOR 2 <= (numeric, int4),
  OPERATOR 3 = (numeric, int4),
  OPERATOR 4 >= (numeric, int4),
  OPERATOR 5 > (numeric, int4),
  
  -- numeric <op> int8 with support function
  FUNCTION 1 (numeric, int8) numeric_cmp_int8(numeric, int8),
  OPERATOR 1 < (numeric, int8),
  OPERATOR 2 <= (numeric, int8),
  OPERATOR 3 = (numeric, int8),
  OPERATOR 4 >= (numeric, int8),
  OPERATOR 5 > (numeric, int8),
  
  -- int2 <op> numeric (commutator direction) - uses reverse comparison function
  FUNCTION 1 (int2, numeric) int2_cmp_numeric(int2, numeric),
  OPERATOR 1 < (int2, numeric),
  OPERATOR 2 <= (int2, numeric),
  OPERATOR 3 = (int2, numeric),
  OPERATOR 4 >= (int2, numeric),
  OPERATOR 5 > (int2, numeric),
  
  -- int4 <op> numeric (commutator direction) - uses reverse comparison function
  FUNCTION 1 (int4, numeric) int4_cmp_numeric(int4, numeric),
  OPERATOR 1 < (int4, numeric),
  OPERATOR 2 <= (int4, numeric),
  OPERATOR 3 = (int4, numeric),
  OPERATOR 4 >= (int4, numeric),
  OPERATOR 5 > (int4, numeric),
  
  -- int8 <op> numeric (commutator direction) - uses reverse comparison function
  FUNCTION 1 (int8, numeric) int8_cmp_numeric(int8, numeric),
  OPERATOR 1 < (int8, numeric),
  OPERATOR 2 <= (int8, numeric),
  OPERATOR 3 = (int8, numeric),
  OPERATOR 4 >= (int8, numeric),
  OPERATOR 5 > (int8, numeric);

-- Add int × numeric operators to integer_ops btree family
-- This enables merge joins and index access from the integer side.
-- Per spec US4: int×numeric operators must be in BOTH integer_ops AND numeric_ops.
--
-- CRITICAL: We must also add numeric's own operators and comparison function
-- so PostgreSQL can compare two numeric values within integer_ops context
-- (e.g., during merge join sort). We reference PostgreSQL's built-in operators.
ALTER OPERATOR FAMILY integer_ops USING btree ADD
  -- Same-type numeric comparison (required for merge join sorting)
  FUNCTION 1 (numeric, numeric) numeric_cmp(numeric, numeric),
  OPERATOR 1 < (numeric, numeric),
  OPERATOR 2 <= (numeric, numeric),
  OPERATOR 3 = (numeric, numeric),
  OPERATOR 4 >= (numeric, numeric),
  OPERATOR 5 > (numeric, numeric),
  
  -- numeric <op> int2 with support function
  FUNCTION 1 (numeric, int2) numeric_cmp_int2(numeric, int2),
  OPERATOR 1 < (numeric, int2),
  OPERATOR 2 <= (numeric, int2),
  OPERATOR 3 = (numeric, int2),
  OPERATOR 4 >= (numeric, int2),
  OPERATOR 5 > (numeric, int2),
  
  -- numeric <op> int4 with support function
  FUNCTION 1 (numeric, int4) numeric_cmp_int4(numeric, int4),
  OPERATOR 1 < (numeric, int4),
  OPERATOR 2 <= (numeric, int4),
  OPERATOR 3 = (numeric, int4),
  OPERATOR 4 >= (numeric, int4),
  OPERATOR 5 > (numeric, int4),
  
  -- numeric <op> int8 with support function
  FUNCTION 1 (numeric, int8) numeric_cmp_int8(numeric, int8),
  OPERATOR 1 < (numeric, int8),
  OPERATOR 2 <= (numeric, int8),
  OPERATOR 3 = (numeric, int8),
  OPERATOR 4 >= (numeric, int8),
  OPERATOR 5 > (numeric, int8),
  
  -- int2 <op> numeric - uses reverse comparison function for correct argument order
  FUNCTION 1 (int2, numeric) int2_cmp_numeric(int2, numeric),
  OPERATOR 1 < (int2, numeric),
  OPERATOR 2 <= (int2, numeric),
  OPERATOR 3 = (int2, numeric),
  OPERATOR 4 >= (int2, numeric),
  OPERATOR 5 > (int2, numeric),
  
  -- int4 <op> numeric - uses reverse comparison function
  FUNCTION 1 (int4, numeric) int4_cmp_numeric(int4, numeric),
  OPERATOR 1 < (int4, numeric),
  OPERATOR 2 <= (int4, numeric),
  OPERATOR 3 = (int4, numeric),
  OPERATOR 4 >= (int4, numeric),
  OPERATOR 5 > (int4, numeric),
  
  -- int8 <op> numeric - uses reverse comparison function
  FUNCTION 1 (int8, numeric) int8_cmp_numeric(int8, numeric),
  OPERATOR 1 < (int8, numeric),
  OPERATOR 2 <= (int8, numeric),
  OPERATOR 3 = (int8, numeric),
  OPERATOR 4 >= (int8, numeric),
  OPERATOR 5 > (int8, numeric);

-- ============================================================================
-- NOTE: int×float operators are NOT added to float_ops btree family in v1.0
-- per spec US5 (deferred to reduce scope). Index optimization is provided via
-- support functions. Future versions may add btree family integration.
-- ============================================================================

-- ============================================================================
-- Hash Operator Family Support (Enables Hash Joins)
-- ============================================================================
--
-- Adding our cross-type equality operators to hash operator families enables
-- hash joins for queries like: SELECT ... FROM numeric_table JOIN int_table
-- ON numeric_col = int_col
--
-- Hash families only need OPERATOR entries (no custom functions needed).
-- PostgreSQL will implicitly cast integers to numeric/float and use the
-- existing hash_numeric/hashfloat4/hashfloat8 functions, ensuring:
--   hash_numeric(10::int4) = hash_numeric(10.0::numeric)
--
-- This approach adds operators to higher-precision families (numeric_ops,
-- float_ops) for the same reason as btree: to avoid enabling invalid 
-- transitive inferences in integer_ops.
-- ============================================================================

-- Add numeric = int operators to numeric_ops hash family
ALTER OPERATOR FAMILY numeric_ops USING hash ADD
  -- Hash functions for integer types (cast to numeric before hashing)
  FUNCTION 1 (int2, int2) hash_int2_as_numeric(int2),
  FUNCTION 2 (int2, int2) hash_int2_as_numeric_extended(int2, int8),
  FUNCTION 1 (int4, int4) hash_int4_as_numeric(int4),
  FUNCTION 2 (int4, int4) hash_int4_as_numeric_extended(int4, int8),
  FUNCTION 1 (int8, int8) hash_int8_as_numeric(int8),
  FUNCTION 2 (int8, int8) hash_int8_as_numeric_extended(int8, int8),
  
  -- numeric = int2 (both directions)
  OPERATOR 1 = (numeric, int2),
  OPERATOR 1 = (int2, numeric),
  
  -- numeric = int4 (both directions)
  OPERATOR 1 = (numeric, int4),
  OPERATOR 1 = (int4, numeric),
  
  -- numeric = int8 (both directions)
  OPERATOR 1 = (numeric, int8),
  OPERATOR 1 = (int8, numeric);

-- Add float4 = int operators to float_ops hash family
-- Note: We use float8 hash functions for consistency since PostgreSQL's
-- float_ops family already ensures hashfloat4(x) = hashfloat8(x)
ALTER OPERATOR FAMILY float_ops USING hash ADD
  -- Hash functions for integer types (cast to float8 before hashing for consistency)
  FUNCTION 1 (int2, int2) hash_int2_as_float8(int2),
  FUNCTION 2 (int2, int2) hash_int2_as_float8_extended(int2, int8),
  FUNCTION 1 (int4, int4) hash_int4_as_float8(int4),
  FUNCTION 2 (int4, int4) hash_int4_as_float8_extended(int4, int8),
  FUNCTION 1 (int8, int8) hash_int8_as_float8(int8),
  FUNCTION 2 (int8, int8) hash_int8_as_float8_extended(int8, int8),
  
  -- float4 = int2 (both directions)
  OPERATOR 1 = (float4, int2),
  OPERATOR 1 = (int2, float4),
  
  -- float4 = int4 (both directions)
  OPERATOR 1 = (float4, int4),
  OPERATOR 1 = (int4, float4),
  
  -- float4 = int8 (both directions)
  OPERATOR 1 = (float4, int8),
  OPERATOR 1 = (int8, float4),
  
  -- float8 = int2 (both directions)
  OPERATOR 1 = (float8, int2),
  OPERATOR 1 = (int2, float8),
  
  -- float8 = int4 (both directions)
  OPERATOR 1 = (float8, int4),
  OPERATOR 1 = (int4, float8),
  
  -- float8 = int8 (both directions)
  OPERATOR 1 = (float8, int8),
  OPERATOR 1 = (int8, float8);
