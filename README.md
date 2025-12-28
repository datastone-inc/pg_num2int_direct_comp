<div align="center">

<img src="assets/datastone-logo.svg" alt="dataStone Logo" width="400"/>

# pg_num2int_direct_comp

[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-12%2B-blue.svg)](https://www.postgresql.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Language](https://img.shields.io/badge/Language-C-orange.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Extension](https://img.shields.io/badge/Type-PostgreSQL%20Extension-purple.svg)](https://www.postgresql.org/docs/current/extend-extensions.html)

Exact numeric-to-integer comparison operators for PostgreSQL

</div>

---

## Table of Contents

- [Overview](#overview)
  - [Key Features](#key-features)
  - [Supported Types](#supported-types)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Build and Install](#build-and-install)
  - [Enable in Database](#enable-in-database)
- [Quick Start](#quick-start)
  - [Example 1: Detecting Float Precision Loss](#example-1-detecting-float-precision-loss)
  - [Example 2: Transitive Equality Correctness](#example-2-transitive-equality-correctness)
  - [Example 3: Index-Optimized Queries](#example-3-index-optimized-queries)
  - [Example 4: Fractional Comparisons](#example-4-fractional-comparisons)
  - [Example 5: Range Queries](#example-5-range-queries)
  - [Example 6: Hash Joins for Large Tables](#example-6-hash-joins-for-large-tables)
  - [Example 7: Type Aliases](#example-7-type-aliases)
- [Limitations](#limitations)
  - [Merge Joins Not Supported](#merge-joins-not-supported)
  - [Join Strategy Selection](#join-strategy-selection)
- [Documentation](#documentation)
- [Technical Details](#technical-details)
- [Compatibility](#compatibility)
- [Performance](#performance)
- [Support](#support)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

PostgreSQL database schema drift and software evolution means that cross-type comparisons between numeric types (numeric, float4, float8) and integer types (int2, int4, int8) will occur. PostgreSQL's default behavior injects implicit casts that cause two critical problems: **incorrect comparison results** and **poor query performance**.

### The Problem: Implicit Casting Produces Wrong Results

When no direct cross-type comparison operator exists, PostgreSQL implicitly casts operands to a common type before comparing. The resolution depends on which types are involved:

- **Float vs. integer** (e.g., `float4 = int4`): Both operands are implicitly cast to `float8`, since it's the *preferred type* when floats are involved
- **Numeric vs. integer** (e.g., `numeric = int8`): The integer is implicitly cast to `numeric`

When integers are cast to floating-point types, precision can be lost.

#### Why Precision Loss Occurs

IEEE 754 floating-point formats used by PostgreSQL have limited mantissa bits:

- **float4 (32-bit)**: 23-bit mantissa → can exactly represent integers only up to 2²⁴ (16,777,216)
- **float8 (64-bit)**: 52-bit mantissa → can exactly represent integers only up to 2⁵³ (9,007,199,254,740,992)

Integers beyond these thresholds get **rounding errors** when cast to float. The database silently loses precision, and comparisons produce mathematically incorrect results:

```sql
-- PostgreSQL's DEFAULT behavior (WRONG):
SELECT 16777217::int4 = 16777217::float4;  -- Returns FALSE (!)

-- What actually happens:
-- 1. 16777217::float4 cannot be exactly represented, rounds to 16777216.0
-- 2. PostgreSQL casts both operands to float8 for comparison
-- 3. Compares 16777217.0 = 16777216.0 → FALSE

-- The same integer literal on both sides returns FALSE, i.e., there are many integer values for which there are no equal float4 (or float8) representation. If you rely on cross-type comparisons, this leads to silent incorrect results.
```

This affects real-world scenarios, for myself, this is why I originally created this extension, a customer application was passing user IDs (and all numeric parametes) as float4 values, leading to silent data integrity issues:

```sql
-- User lookup returns the WRONG person
CREATE TABLE users(id int4 PRIMARY KEY, name text);
INSERT INTO users VALUES (16777216, 'Alice'), (16777217, 'Bob');

-- Application passes user ID as float4 parameter (e.g., from JSON, ORM, or API)
PREPARE find_user(float4) AS SELECT * FROM users WHERE id = $1;
EXECUTE find_user(16777217);  -- Looking for Bob...
--    id    | name  
-- ---------+-------
--  16777216 | Alice   ← WRONG! We asked for Bob. Bob cannot be found.
```

#### Transitive Equality Violations

PostgreSQL's implicit casting also violates transitivity, normally a fundamental property of equality:

```sql
-- If a = b and b = c, then a = c should always hold
-- But with implicit casts:
SELECT 16777216::int4 = 16777217::float4;  -- TRUE (after cast)
SELECT 16777217::float4 = 16777217::int4;  -- TRUE (after cast)  
SELECT 16777216::int4 = 16777217::int4;    -- FALSE

-- Transitivity violated: a=b, b=c, but a≠c
```

> **This extension guarantees mathematical correctness.** All comparison operators added by this extension satisfy reflexivity, symmetry, and transitivity for equality, and irreflexivity, transitivity, and trichotomy for ordering. Cross-type comparisons work correctly without precision loss.

PostgreSQL's query planner can leverage transitivity (inferring `a=c` from `a=b` and `b=c`) only when operators are defined with appropriate btree [operator class](https://www.postgresql.org/docs/current/btree.html#BTREE-SUPPORT-FUNCS) strategies. Stock PostgreSQL's implicit-cast-based comparisons don't provide this, leading to missed optimizations. This extension defines proper operator classes for all cross-type comparisons.

### The Problem: Implicit Casting Destroys Performance

Beyond correctness, implicit casts prevent the query optimizer from using indexes efficiently.

#### Index Scans Become Sequential Scans

When you write `WHERE intcol = 10.0::numeric`, PostgreSQL's default behavior:

1. Sees a type mismatch (int4 column vs. numeric literal)
2. Casts `intcol` to numeric for every row in the table
3. Cannot use a btree index on `intcol` because the indexed expression is `intcol`, not `intcol::numeric`
4. Falls back to a **sequential scan** of the entire table

```sql
-- Without this extension:
EXPLAIN SELECT * FROM million_row_table WHERE intcol = 10.0::numeric;
--  Seq Scan on million_row_table  (cost=0.00..19425.00 rows=5000 width=8)
--    Filter: ((intcol)::numeric = 10.0)
--  Execution time: 850ms

-- With this extension:
EXPLAIN SELECT * FROM million_row_table WHERE intcol = 10.0::numeric;
--  Index Scan using idx_intcol on million_row_table  (cost=0.42..8.44 rows=1 width=8)
--    Index Cond: (intcol = 10)
--  Execution time: 0.1ms
```

#### Hash Joins Disabled for Cross-Type Equijoins

PostgreSQL cannot use hash joins for cross-type equality conditions because different types have different hash functions. A hash join requires `hash(a) = hash(b)` whenever `a = b`, but:

```sql
-- These hash differently by default, so no hash join possible:
hash(10::int4) ≠ hash(10.0::numeric)
```

Without hash join support, large table joins default to nested loops without index optimization—potentially orders of magnitude slower.

### The Solution: Exact Cross-Type Comparison

This extension provides 108 comparison operators that compare numeric and integer types **directly**, without implicit casting. The comparison algorithm ensures mathematical correctness:

#### How Exact Comparison Works

For `numeric_value = integer_value`:

1. **Bounds check**: If `numeric_value` is outside the integer type's range (e.g., > 2³¹-1 for int4), return the appropriate ordering result immediately
2. **Truncate to integer part**: Extract the integer portion of the numeric value
3. **Compare integer parts**: If truncated value ≠ integer, they're not equal
4. **Check for fractional part**: If numeric has any fractional component, equality is false; for ordering, the fractional part determines if numeric is slightly greater than the truncated integer

```c
// Pseudocode C comparison function
int compare_numeric_to_int(numeric num, int32 i) {
    // Handle special values (NaN, ±Infinity)
    if (is_nan(num)) return NOT_EQUAL;  // NaN ≠ everything
    if (is_inf(num)) return (positive ? GREATER : LESS);
    
    // Check if num is outside int32 range
    if (num > INT32_MAX) return GREATER;
    if (num < INT32_MIN) return LESS;
    
    // Truncate numeric to integer part
    int32 num_truncated = trunc(num);
    
    // Compare integer values
    if (num_truncated > i) return GREATER;
    if (num_truncated < i) return LESS;
    
    // Integer parts equal, check for nonzero fractional part
    if (has_fractional_part(num)) return (i > 0) ? GREATER : LESS;
    
    // exact EQUALITY
    return EQUAL;
}
```

This approach is **mathematically correct** because:

- No precision is lost—we never cast integers to floats
- Fractional parts are preserved and compared exactly
- Values outside representable ranges are handled correctly
- IEEE 754 special values (NaN, ±Infinity) follow standard semantics

#### Index Optimization via Support Functions

The extension uses PostgreSQL's `SupportRequestIndexCondition` mechanism to enable index usage for all comparison operators (`=`, `<`, `<=`, `>`, `>=`, `<>`). When the query contains a cross-type comparison like `intcol >= 10.0::numeric` or `intcol = $1` (where `$1` is a numeric parameter):

1. The support function intercepts the comparison
2. It checks if the numeric value is exactly representable as the integer type
3. If yes: transforms to `intcol >= 10` (pure integer comparison), enabling index scan
4. If no: adjusts the comparison appropriately (e.g., `intcol > 10.5` becomes `intcol >= 11`)

```sql
-- Real-world example: API passes product ID as numeric parameter
CREATE TABLE products(id int4 PRIMARY KEY, name text, price numeric);
PREPARE find_product(numeric) AS SELECT * FROM products WHERE id = $1;

-- Without this extension: sequential scan (casts indexed column)
EXPLAIN (COSTS OFF) EXECUTE find_product(42);
--  Seq Scan on products
--    Filter: ((id)::numeric = '42'::numeric)   ← full table scan!

-- With this extension: index scan (transforms to integer comparison)  
EXPLAIN (COSTS OFF) EXECUTE find_product(42);
--  Index Scan using products_pkey on products
--    Index Cond: (id = 42)                     ← uses primary key index
```

<!-- TODO: Add transitivity example when merge join support is added in next phase.
     When cross-type operators are in the same btree operator family as same-type operators,
     PostgreSQL can infer A.id = $1 from A.id = B.val AND B.val = $1, enabling index nested 
     loop joins with index lookups on both sides. Example:
     
     EXPLAIN SELECT * FROM A, B WHERE A.id = B.val AND B.val = $1;
     --  Nested Loop
     --    -> Index Scan on b_val_idx on B
     --    -> Index Scan on a_pkey on A   ← uses A.id = $1 (inferred)
-->

#### Hash Support for Joins, GROUP BY, and DISTINCT

The extension provides cross-type hash functions that ensure equal values hash identically regardless of type. This enables PostgreSQL to use hash-based operations across type boundaries:

**How it works:**
- Cross-type equality operators are added to the `numeric_ops` hash operator family (for numeric × integer comparisons)
- Cross-type equality operators are added to the `float_ops` hash operator family (for float × integer comparisons)
- When hashing an integer for cross-type comparison, it computes the hash of the equivalent higher-precision value
- This guarantees `a = b → hash(a) = hash(b)` across types, enabling hash joins

**Why only `numeric_ops` fully supports GROUP BY and DISTINCT:**

The `numeric_ops` family fully supports cross-type hash aggregation because numeric can exactly represent all integer values. However, `float_ops` is limited to hash **joins** only—we intentionally do not add the additional operators that would enable cross-type hash GROUP BY or DISTINCT with floats.

The reason: enabling hash aggregation across float/integer types would require PostgreSQL to treat integers as members of the float type family. This would cause the query planner to make transitive inferences that violate mathematical correctness (since floats cannot exactly represent all integers). For example, the planner might incorrectly group `16777217::int4` with `16777216::float4` because they would hash identically after float conversion.

**Operations enabled:**

```sql
-- Hash joins across int4 and numeric columns
EXPLAIN (COSTS OFF) SELECT c.name, SUM(o.amount) 
FROM customers c                              -- c.id is int4
JOIN orders o ON c.id = o.customer_id         -- o.customer_id is numeric
GROUP BY c.name;

--  HashAggregate
--    Group Key: c.name
--    ->  Hash Join                           ← hash join across types!
--          Hash Cond: (o.customer_id = c.id)
--          ->  Seq Scan on orders o
--          ->  Hash
--                ->  Seq Scan on customers c

-- Without this extension, PostgreSQL cannot hash-join across types
-- and falls back to slower nested loop joins
```

GROUP BY and DISTINCT also benefit when mixing types, as PostgreSQL can use HashAggregate instead of Sort-based approaches.

#### Performance: Correctness Enables Speed

A natural question: isn't this per-comparison logic slower than PostgreSQL's simple float cast-and-compare?

**Per-operation**: Yes, slightly. The fractional-part check adds a few CPU cycles compared to a raw float comparison. However, the code is written in C and the overhead is minimal.

**Overall query performance**: This extension is typically **much faster** than stock PostgreSQL for cross-type comparisons because:

1. **Index scans instead of sequential scans**: Stock PostgreSQL casts the indexed column, forcing sequential scans. This extension enables index usage—scanning a handful of rows instead of the entire table.

2. **Hash joins instead of nested loops**: Stock PostgreSQL cannot hash-join across types. This extension enables hash joins—a single pass through each table instead of repeatedly scanning one table for every row in the other.

The irony is that PostgreSQL's "fast" implicit cast approach is actually *slow* for real queries because it defeats the optimizer. **Correctness and performance are not at odds here—the correct approach enables the fast query plans.**

### Key Features

- **Exact Precision**: `16777216::int4 = 16777217::float4` correctly returns `false` (detects float4 precision loss)
- **Index Optimization**: Queries like `WHERE intkey = 10.0::numeric` use btree indexes via indexed nested loop joins
- **Hash Join Support**: Large table equijoins use hash joins when appropriate
- **Complete Coverage**: 108 operators (6 comparison types × 9 type pairs × 2 directions) covering all combinations of (numeric, float4, float8) with (int2, int4, int8) in both directions
- **Automatic Type Compatibility**: Works seamlessly with PostgreSQL type aliases (serial/smallserial/bigserial are int4/int2/int8; decimal is numeric)

### Supported Types

**Numeric Types**: `numeric`, `decimal` (alias for numeric), `float4` (real), `float8` (double precision)  
**Integer Types**: `int2` (smallint), `int4` (integer), `int8` (bigint), `serial`, `smallserial`, `bigserial`

Note: Serial types are automatically supported because PostgreSQL treats them as their underlying integer types (serial=int4, bigserial=int8, smallserial=int2).

## Installation

### Prerequisites

- PostgreSQL 12 or later
- Development headers (`postgresql-server-dev` or equivalent)
- PGXS build environment

### Build and Install

```bash
make
sudo make install
```

### Enable in Database

```sql
CREATE EXTENSION pg_num2int_direct_comp;
```

## Quick Start

### Example 1: Detecting Float Precision Loss

```sql
-- Default PostgreSQL behavior (precision loss)
SELECT 16777216::int4 = 16777217::float4;  -- true (PostgreSQL casts int4→float4, losing precision)

-- With pg_num2int_direct_comp (exact comparison)
CREATE EXTENSION pg_num2int_direct_comp;
SELECT 16777216::int4 = 16777217::float4;  -- false (correct - detects precision mismatch)
```

### Example 2: Transitive Equality Correctness

```sql
-- Extension maintains mathematical correctness
CREATE EXTENSION pg_num2int_direct_comp;

-- These are NOT transitively equal (10.5 has fractional part)
SELECT 10::int4 = 10::int4;        -- true
SELECT 10::int4 = 10.5::numeric;   -- false (fractional part)
SELECT 10::int4 = 10.5::numeric;   -- false (correctly prevents invalid transitive inference)

-- These ARE transitively equal (exact match)
SELECT 10::int4 = 10.0::numeric;   -- true
SELECT 10.0::numeric = 10::int8;   -- true
-- Therefore: 10::int4 = 10::int8  (valid transitive equality)
```

### Example 3: Index-Optimized Queries

```sql
CREATE TABLE measurements (id SERIAL, value INT4);
CREATE INDEX ON measurements(value);
INSERT INTO measurements (value) SELECT generate_series(1, 1000000);

-- Uses index efficiently (no cast on indexed column)
EXPLAIN SELECT * FROM measurements WHERE value = 500.0::numeric;
-- Result: Index Scan using measurements_value_idx
```

### Example 4: Fractional Comparisons

```sql
-- Fractional values never equal integers
SELECT 10::int4 = 10.5::numeric;  -- false
SELECT 10::int4 < 10.5::numeric;  -- true
```

### Example 5: Range Queries

```sql
CREATE TABLE inventory (item_id INT4, quantity INT4);
INSERT INTO inventory VALUES (1, 10), (2, 11), (3, 12);

-- Exact boundary handling
SELECT * FROM inventory WHERE quantity > 10.5::float8;
-- Returns: (2, 11), (3, 12)
```

### Example 6: Hash Joins for Large Tables

```sql
-- Hash joins work automatically for large table joins
CREATE TABLE sales (id SERIAL, amount NUMERIC(10,2));
CREATE TABLE targets (id SERIAL, threshold INT4);
INSERT INTO sales SELECT generate_series(1, 1000000), random() * 1000;
INSERT INTO targets SELECT generate_series(1, 1000000), (random() * 1000)::int4;

-- Planner chooses hash join for large equijoin
EXPLAIN SELECT COUNT(*) FROM sales s JOIN targets t ON s.amount = t.threshold;
-- Result: Hash Join (when tables are large)
```

### Example 7: Type Aliases

```sql
-- Serial types work automatically
CREATE TABLE users (id SERIAL, score INT4);
SELECT * FROM users WHERE id = 100.0::numeric;  -- Uses exact comparison

-- Decimal type works automatically
SELECT 10::int4 = 10.0::decimal;  -- true (decimal is alias for numeric)
```

## Limitations

### Merge Joins (Coming Soon)

**What**: Merge join optimization is planned for v1.0.0. This will require adding operators to the `integer_ops` family with safeguards to prevent invalid transitive inference (see research.md section 4 for details).

**Current Impact**: Until merge join support lands, queries will use indexed nested loop joins or hash joins. For most real-world queries with selective predicates and indexes, indexed nested loop joins provide equal or better performance than merge joins.

**Details**: See [specs/001-num-int-direct-comp/research.md](specs/001-num-int-direct-comp/research.md#4-3-merge-join-implementation-path) for the implementation plan and rationale.

### Join Strategy Selection

- ✅ **Indexed Nested Loop Joins**: Fully supported and optimized (sub-millisecond on indexed columns)
- ✅ **Hash Joins**: Fully supported for large table equijoins
- ❌ **Merge Joins**: Not possible due to architectural constraints

## Documentation

- [Installation Guide](doc/installation.md) - Detailed setup instructions
- [User Guide](doc/user-guide.md) - Usage patterns and best practices
- [API Reference](doc/api-reference.md) - Complete operator reference
- [Development Guide](doc/development.md) - Contributing and testing
- [Research & Design](specs/001-num-int-direct-comp/research.md) - Architectural decisions and trade-offs

## Technical Details

- **Language**: C (C99)
- **Build System**: PGXS
- **Test Framework**: pg_regress
- **License**: MIT

## Compatibility

- PostgreSQL 12, 13, 14, 15, 16
- Linux, macOS, Windows (via MinGW)

## Performance

- **Index lookups**: Sub-millisecond on 1M+ row tables using indexed nested loop joins
- **Hash joins**: Efficient for large table equijoins
- **Overhead**: <10% vs native integer comparisons
- **Optimized**: Uses PostgreSQL's built-in numeric primitives and hash functions

## Support

- **Issues**: Report bugs via GitHub Issues
- **Documentation**: See `doc/` directory
- **Contributing**: See [DEVELOPMENT.md](doc/development.md)

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

This extension was developed with assistance from AI tools to ensure code quality and completeness.
