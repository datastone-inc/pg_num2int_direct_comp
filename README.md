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
  - [Example 2: Planner Transitivity Inference](#example-2-planner-transitivity-inference)
  - [Example 3: Index-Optimized Queries](#example-3-index-optimized-queries)
  - [Example 4: Fractional Comparisons](#example-4-fractional-comparisons)
  - [Example 5: Hash Joins for Large Tables](#example-5-hash-joins-for-large-tables)
  - [Example 6: Type Aliases](#example-6-type-aliases)
  - [Example 7: Query Optimization - Impossible Predicate Detection](#example-7-query-optimization---impossible-predicate-detection)
- [Limitations](#limitations)
  - [Merge Joins Not Supported](#merge-joins-not-supported)
  - [Join Strategy Selection](#join-strategy-selection)
- [Documentation](#documentation)
- [Technical Details](#technical-details)
- [Compatibility](#compatibility)
- [Performance Benchmarking](#performance-benchmarking)
- [Support](#support)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

Database schemas drift and SQL expressions evolve such that cross-type comparisons between numeric types (numeric, float4, float8) and integer types (int2, int4, int8) can occur. If this is happening for you, PostgreSQL's default behavior injects implicit casts that cause two critical problems: **mathematically incorrect comparison results** and **poor query performance**. Whereas this extension provides **exact cross-type comparison operators** to address both problems.

### The Problem: Implicit Casting Produces Wrong Results

When no direct cross-type comparison operator exists, PostgreSQL implicitly casts operands to a common type before comparing. The type resolution depends on which types are involved:

- **Float vs. integer** (e.g., `float4 = int4`): Both operands are implicitly cast to `float8`, since it's the *preferred type* when floats are involved
- **Numeric vs. integer** (e.g., `numeric = int8`): The integer is implicitly cast to `numeric`

When integers are cast to floating-point types, precision can be lost.

#### Why Precision Loss Occurs

IEEE 754 floating-point used by PostgreSQL have limited mantissa bits:

- **float4 (32-bit)**: 23-bit mantissa → can exactly represent integers only up to 2²⁴ (16,777,216). Beyond this, adjacent representable values are separated by increasing gaps (2 at 2²⁵, 4 at 2²⁶, ..., 2⁴⁰ at 2⁶³), so values like 2²⁴+1 round to the nearest representable float.
- **float8 (64-bit)**: 52-bit mantissa → can exactly represent integers only up to 2⁵³ (9,007,199,254,740,992). Beyond this, adjacent representable values are separated by increasing gaps (2 at 2⁵⁴, 4 at 2⁵⁵, ..., 2¹¹ at 2⁶³).

Integers get these **rounding errors** when explicitly or implicitly cast to float. The database silently loses precision, and default comparisons produce mathematically incorrect results:

```sql
-- Without extension (stock PostgreSQL):
-- PostgreSQL's DEFAULT behavior (Mathematically WRONG):
SELECT 16777216::int4 = 16777217::float4   -- default returns TRUE (!?)
     , 16777217::int4 = 16777217::float4;  -- default returns FALSE (!?)

-- What actually happens:
-- 1. 16777217::float4 cannot be exactly represented, rounds to 1.6777216e7
-- 2. PostgreSQL casts both operands to float8 for comparison
-- 3. 1.6777216e7 = 1.6777216e7 → TRUE and 1.6777217e7 = 1.6777216e7 → FALSE

-- The same integer literal on both sides returns FALSE. There are many integer
-- values for which there are no equal float4 or float8 representations.
-- If you rely on cross-type comparisons, this leads to silent incorrect results.
```

This affects real-world scenarios, and for myself, this is why I originally created this extension: a customer's applications was passing user IDs (and all numeric parameters) as float4 values, leading to silent data integrity and performance issues, e.g.:

```sql
-- Without extension (stock PostgreSQL):
-- User lookup returns the WRONG person
CREATE TABLE users(id int4 PRIMARY KEY, name text);
INSERT INTO users VALUES (16777216, 'Alice'), (16777217, 'Bob');

-- Application passes user ID as float4 parameter
PREPARE find_user(float4) AS SELECT * FROM users WHERE id = $1;
EXECUTE find_user(16777217);  -- Looking for Bob...
--    id    | name
-- ---------+-------
--  16777216 | Alice   ← WRONG! and SLOW!
--
-- We asked for Bob and got Alice. Bob cannot be found.
-- WRONG! because 16777217::float4 rounds to 16777216.0 before the comparison.
-- SLOW! because it does a sequential scan of users (cannot use index on id).
```

> **Note Well**: This extension cannot fix this wrong result case (float4 parameter rounding error) because the value `16777217` is rounded to `16777216.0` when converted to float4, *before* any comparison happens. The solution is to (a) use same type `int4` or even `numeric` parameters instead of float4 when exact int4 values are needed, or (b) use a range predicate that accounts for the rounding error ... it is hard to construct such a predicate.

#### Transitive Equality Violations

PostgreSQL's default implicit casting also violates transitivity, normally a fundamental property of equality:

```sql
-- Without extension (stock PostgreSQL):
-- If a = b and b = c, then a = c should always hold
-- but when int8 is implicitly cast to float8 there can be rounding errors, e.g., at the float8 2^53 boundary:
SELECT 9007199254740992::int8 = 9007199254740993::float8;  -- a=b TRUE?! (both become same float8 due to rounding error)
SELECT 9007199254740993::float8 = 9007199254740992::int8;  -- b=c TRUE (left float8 rounds to 9007199254740992)
SELECT 9007199254740993::int8 = 9007199254740992::int8;    -- a=c FALSE!
-- Mathematical transitivity violated: a=b, b=c, but a≠c.
-- PostgreSQL doesn't infer transitivity because it knows about the implicit cast rounding error seen in a=b.
```

Implicit casts can creep into user queries in surprising ways, e.g., expressions that promote integers to float8, causing precision loss at the 2^53 boundary:

```sql
-- Without extension (stock PostgreSQL):
-- Why PostgreSQL cannot infer a=c from a=f(b) AND f(b)=c when f() causes float8 promotion
WITH vals(a, b, c, d) AS (
    VALUES (9007199254740993::int8,   -- A: cannot be exactly represented in float8
            9007199254740993::int8,   -- B: same as A
            9007199254740992::int8,   -- C: one less (exact in float8)
            0.0::float8)              -- D: float8 zero
)
SELECT a, b, c,
    a = (b + d) AS a_eq_bplusd,    -- TRUE ?!: b+d promotes to float8, rounds to 9007199254740992.0
    (b + d) = c AS bplusd_eq_c,    -- TRUE: 9007199254740992.0 = 9007199254740992
    a = c AS a_eq_c                -- FALSE: 9007199254740993 ≠ 9007199254740992
FROM vals
WHERE a = (b + d) AND (b + d) = c;
-- Row IS returned! If PostgreSQL incorrectly inferred a=c, this row would be filtered out.
-- With the extension, a_eq_bplusd would be FALSE (no rows), a_eq_c still FALSE.
```

> **This extension guarantees mathematical correctness.** All comparison operators added by this extension satisfy reflexivity, symmetry, and transitivity for equality, and irreflexivity, transitivity, and trichotomy for ordering. Cross-type comparisons work correctly without precision loss. For example, in the above cases, the extension's operators preserve transitivity:

```sql
-- With extension:
-- integral values compared to float values detect precision loss correctly:
SELECT 16777217::int4 = 16777216::float4;                  -- FALSE, preserving transitivity
SELECT 9007199254740992::int8 = 9007199254740993::float8;  -- FALSE, preserving transitivity
```

PostgreSQL's query planner can leverage transitivity (inferring `a=c` from `a=b` and `b=c`) only when operators are defined with appropriate btree [operator class](https://www.postgresql.org/docs/current/btree.html#BTREE-SUPPORT-FUNCS) strategies. Stock PostgreSQL's implicit-cast-based comparisons don't provide this, leading to missed optimizations. This extension defines proper operator classes for it's cross-type comparisons, but only when they are mathematically correct.

### The Problem: Implicit Casting Destroys Performance

Besides correctness, implicit casts prevent the query optimizer from using indexes, hash joins, and merge joins efficiently.

#### Index Scans Become Sequential Scans

When you write `WHERE intkey = 10.0::numeric`, PostgreSQL's default behavior:

1. Sees a type mismatch (int4 column vs. numeric literal)
2. Casts `intkey` to numeric for every row in the table
3. Cannot use a btree index on `intkey` because the indexed expression is `intkey`, not `intkey::numeric`
4. Falls back to a **sequential scan** of the entire table

```sql
-- Without this extension:
EXPLAIN SELECT * FROM million_row_table WHERE intkey = 10.0::numeric;
--  Seq Scan on million_row_table  (cost=0.00..19425.00 rows=5000 width=8)
--    Filter: ((intkey)::numeric = 10.0)
--  Execution time: 850ms

-- With this extension:
EXPLAIN SELECT * FROM million_row_table WHERE intkey = 10.0::numeric;
--  Index Scan using idx_intkey on million_row_table  (cost=0.42..8.44 rows=1 width=8)
--    Index Cond: (intkey = 10)
--  Execution time: 0.1ms
-- Note: constants like 10.0::numeric are simplified to 10 by the extension
-- but non-constants like $1::numeric or join conditions gain index SARGability.
```

#### Hash Joins Disabled for Cross-Type Equijoins

PostgreSQL cannot use hash joins for cross-type equality conditions because different types have different hash functions. A hash join requires `hash(a) = hash(b)` whenever `a = b`, but:

```sql
-- These hash differently by default, so no hash join possible:
hash(10::int4) ≠ hash(10.0::numeric)
```

Without hash join support, large table joins default to nested loops without index optimization. This can be orders of magnitude slower.

### The Solution: Exact Cross-Type Comparison

This extension provides 108 comparison operators that compare numeric and integer types **directly**, without implicit casting. The comparison logic ensures mathematical correctness. This enables PostgreSQL to use indexes, hash joins, and merge joins effectively. This extension also implements [support function callbacks](https://www.postgresql.org/docs/current/xfunc-optimization.html) that transform cross-type comparisons into native integer comparisons when possible.

#### How Exact Comparison Works

For `numeric_value = integer_value`:

1. **Bounds check**: If `numeric_value` is outside the integer type's range (e.g., > 2³¹-1 for int4), return the appropriate ordering result immediately
2. **Truncate to integer part**: Extract the integer portion of the numeric value
3. **Compare integer parts**: If truncated value ≠ integer, they're not equal
4. **Check for nonzero fractional part**: If numeric has any nonzero fractional component, equality is false; for ordering, the nonzero fractional part determines if numeric is greater/less than the truncated integer

```c
// Pseudocode C comparison function
int compare_numeric_to_int(numeric num, int32 i) {
    // Handle special values (NaN, ±Infinity)
    if (is_nan(num)) return GREATER;  // NaN > every integer in PostgreSQL
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

- No precision is lost, so we never cast integers to floats
- Fractional parts are preserved and compared exactly
- Values outside representable ranges are handled correctly
- IEEE 754 special values (NaN, ±Infinity) follow standard PostgreSQL semantics

#### Index Optimization via Support Functions

The extension uses PostgreSQL's [`SupportRequestSimplify` and `SupportRequestIndexCondition`](https://www.postgresql.org/docs/current/xfunc-optimization.html) mechanisms to enable index usage for all comparison operators (`=`, `<`, `<=`, `>`, `>=`, `<>`). When the query contains a cross-type comparison like `intkey >= 10.0::numeric` or `intkey = $1` (where `$1` is a numeric parameter and the statement is prepared with a custom plan), the support function performs these optimizations:

1. The support function intercepts the comparison
2. It checks if the numeric value is exactly representable as the integer type
3. If yes: transforms to `intkey >= 10` (pure integer comparison), enabling index scan
4. If no: adjusts the comparison appropriately (e.g., `intkey > 10.5` becomes `intkey >= 11`)

```sql
-- Real-world example: API passes product ID as numeric parameter
CREATE TABLE products(id int4 PRIMARY KEY, parent int4 REFERENCES products, name text);
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

**More complex example with joins:**

```sql
-- Hierarchical lookup: find parent product name given child ID
CREATE INDEX ON products(parent);
PREPARE find_parent(numeric) AS
  SELECT p.name AS parent_name
  FROM products p, products c
  WHERE p.id = c.parent AND c.id = $1;

-- Without this extension: sequential scan on both tables
EXPLAIN (COSTS OFF) EXECUTE find_parent(42);
--  Nested Loop
--    ->  Seq Scan on products c
--          Filter: ((id)::numeric = '42'::numeric)   ← full table scan!
--    ->  Seq Scan on products p
--          Filter: (id = c.parent)                   ← another full scan!

-- With this extension: index scans on both tables
EXPLAIN (COSTS OFF) EXECUTE find_parent(42);
--  Nested Loop
--    ->  Index Scan using products_pkey on products c
--          Index Cond: (id = 42)                     ← uses primary key
--    ->  Index Scan using products_pkey on products p
--          Index Cond: (id = c.parent)               ← uses primary key
```

#### Hash Support for Joins, GROUP BY, and DISTINCT

The extension provides cross-type hash functions that ensure equal values hash identically regardless of type. This enables PostgreSQL to use hash-based operations across type boundaries:

**How it works:**

- Cross-type equality operators are added to the `numeric_ops` hash operator family (for numeric × integer comparisons)
- Cross-type equality operators are added to the `float_ops` hash operator family (for float × integer comparisons)
- When hashing an integer for cross-type comparison, it computes the hash of the equivalent higher-precision value
- This guarantees `a = b → hash(a) = hash(b)` across types, enabling hash joins

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

**Per-operation**: Yes. The exact comparison involves bounds checking, truncation, and fractional-part detection which is more expensive than a raw float comparison. However, this overhead is in C. It is negligible compared to I/O costs and is usually outweighed by query-level optimizations.

**Overall query performance**: This extension is typically **much faster** than stock PostgreSQL for cross-type comparisons because:

1. **Index scans instead of sequential scans**: Stock PostgreSQL casts the indexed column, forcing sequential scans. This extension enables index usage, reducing I/O and CPU usage.

2. **Hash joins instead of nested loops**: Stock PostgreSQL cannot hash-join across types. This extension enables hash joins for a single pass through each table instead of repeatedly scanning one table for every row in the other.

The irony is that PostgreSQL's "fast" implicit cast approach is actually *slow* for real queries because it defeats the planner. **Correctness and performance are not at odds here. The correct approach enables fast query plans.**

See [Performance Benchmarking](#performance-benchmarking) section for detailed metrics.

### Key Features

- **Exact Precision**: `16777217::int4 = 16777216::float4` correctly returns `false` (detects float4 precision loss)
- **Index Optimization**: Queries like `WHERE intkey = 10.0::numeric` use btree indexes and indexed nested loop joins
- **Join Support**: Large table equijoins use merge joins and hash joins when appropriate
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
-- With extension:
-- float4 cannot represent 16777217 exactly, it rounds to 16777216.0
-- The extension detects when the integer differs from the float representation

CREATE EXTENSION pg_num2int_direct_comp;

-- This is TRUE because 16777217::float4 rounds to 16777216.0, matching 16777216
SELECT 16777216::int4 = 16777217::float4;  -- true (both are 16777216)

-- This is FALSE because 16777217 ≠ 16777216.0 (extension detects the mismatch!)
SELECT 16777217::int4 = 16777217::float4;  -- false (correct - detects precision loss)
```

### Example 2: Planner Transitivity Inference

```sql
-- With extension:
-- The query planner can infer transitive relationships across types for index optimization
CREATE EXTENSION pg_num2int_direct_comp;

CREATE TABLE orders (id SERIAL, customer_id NUMERIC);  -- numeric foreign key
CREATE TABLE customers (id INT4 PRIMARY KEY, name TEXT);
CREATE INDEX ON orders(customer_id);

-- Query with cross-type join: orders.customer_id (numeric) = customers.id (int4)
PREPARE find_orders(int4) AS
  SELECT o.* FROM orders o JOIN customers c ON o.customer_id = c.id WHERE c.id = $1;

-- With extension: planner infers o.customer_id = $1 across types, enabling index scan
EXPLAIN (COSTS OFF) EXECUTE find_orders(42);
--  Nested Loop
--    ->  Index Scan using customers_pkey on customers c
--          Index Cond: (id = 42)
--    ->  Index Scan using orders_customer_id_idx on orders o
--          Index Cond: (customer_id = 42)    ← inferred across numeric/int4!

-- Without extension: planner cannot infer transitivity across types
-- Would require scanning orders for each customer row without the inferred predicate
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

### Example 4: Range Queries

```sql
CREATE TABLE inventory (item_id INT4, quantity INT4);
INSERT INTO inventory VALUES (1, 10), (2, 11), (3, 12);

-- Exact boundary handling
SELECT * FROM inventory WHERE quantity > 10.5::float8;
-- Returns: (2, 11), (3, 12)
```

### Example 5: Hash Joins for Large Tables

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

### Example 7: Query Optimization - Impossible Predicate Detection

The extension's support functions transform constant predicates during query planning for optimal performance:

```sql
-- Impossible predicate detection: integer can never equal fractional value
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 10.5::numeric;
-- Result:
--  Result
--    One-Time Filter: false
-- The planner recognizes this is impossible and returns rows=0 estimate

-- Exact match transformation: uses native integer operator
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 100::numeric;
-- Result:
--  Index Scan using measurements_value_idx on measurements
--    Index Cond: (value = 100)
-- Transformed from cross-type to native int=int for optimal selectivity

-- Range boundary transformation: adjusts fractional boundaries
-- "value > 10.5" becomes "value >= 11" (since no integer is between 10 and 11)
SELECT * FROM measurements WHERE value > 10.5::numeric;
-- Returns values 11, 12, 13, ... (correctly excludes 10)
```

These optimizations ensure:
- Impossible predicates are detected early (rows=0 estimate, potential scan elimination)
- Exact integer matches use native operators for perfect selectivity
- Range boundaries are correctly adjusted for integer semantics

## Limitations

### Int × Float Merge Joins Not Supported

**What**: Merge joins between integer and float types (float4, float8) are not supported due to floating-point precision constraints.

**Why**: Adding int × float operators to btree families would require them in both `integer_ops` and `float_ops` families. Due to float precision limits, this could enable invalid transitive inferences by the query planner.

**Impact**: Int × float joins use hash joins or indexed nested loop joins, which provide excellent performance for most workloads.

### Int × Numeric Merge Joins

✅ **Fully Supported**: Int × numeric operators are in both `integer_ops` and `numeric_ops` btree families, enabling merge joins.

### Join Strategy Selection

- ✅ **Indexed Nested Loop Joins**: Fully supported for all type combinations
- ✅ **Hash Joins**: Fully supported for all type combinations
- ✅ **Merge Joins**: Supported for int × numeric
- ❌ **Merge Joins**: Not supported for int × float (use hash join or nested loop)

## Documentation

- [Installation Guide](doc/installation.md) - Detailed setup instructions
- [Operator Reference](doc/operator-reference.md) - Operator reference and usage guide
- [Development Guide](doc/development.md) - Contributing and testing
- [Research & Design](specs/001-num-int-direct-comp/research.md) - Architectural decisions and trade-offs

## Technical Details

- **Language**: C (C99)
- **Build System**: PGXS
- **Test Framework**: pg_regress
- **License**: MIT

### Project Structure

```
pg_num2int_direct_comp.c       # Core C implementation (comparison + support functions)
pg_num2int_direct_comp.h       # Header with type definitions and declarations
pg_num2int_direct_comp--1.0.0.sql  # Extension SQL (operators, families, triggers)
pg_num2int_direct_comp.control # Extension metadata
Makefile                       # PGXS build configuration
sql/                           # Regression test input files
expected/                      # Expected test output files
doc/                           # Documentation (installation, API, development)
specs/                         # Design specifications and research notes
```

See [Development Guide](doc/development.md#project-structure) for complete layout.

### Implementation Notes

This extension uses two PostgreSQL mechanisms to ensure correct behavior across DROP/CREATE EXTENSION cycles:

1. **Event Trigger for Operator Family Cleanup**: When adding built-in operators to existing operator families (like `integer_ops`), PostgreSQL doesn't track these memberships as extension-owned. An event trigger on `ddl_command_start` cleans up `pg_amop` entries when the extension is dropped, preventing "operator already exists" errors on reinstall.

2. **Syscache Invalidation Callback**: The support function maintains a static cache of operator OIDs for performance. A callback registered via `CacheRegisterSyscacheCallback(OPEROID, ...)` in `_PG_init()` automatically invalidates this cache when operators change, ensuring index optimization works correctly after extension reinstallation.

See [Research & Design](specs/001-num-int-direct-comp/research.md) sections 7.6 and 7.7 for detailed explanations.

## Compatibility

- PostgreSQL 12, 13, 14, 15, 16
- Linux, macOS, Windows (via MinGW)

## Performance Benchmarking

- **Index lookups**: Sub-millisecond on 1M+ row tables
- **Merge joins**: Efficient for sorted or indexed data (int × numeric only)
- **Hash joins**: Efficient for large table equijoins
- **Merge joins**: 23% faster than stock with optimal plan, 56% less memory
- **Overhead**: <10% vs native integer comparisons
- **Optimized**: Uses PostgreSQL's built-in numeric primitives and hash functions

For comprehensive performance testing, run the benchmark suite:

```bash
# Quick performance test (~seconds, 100K rows)
make installcheck REGRESS=performance

# Full benchmark (~70 seconds, 1M rows)
make installcheck REGRESS=benchmark
```

See [Benchmark Guide](doc/benchmark.md) for detailed methodology, results, and analysis of the planner's cost model behavior with merge joins.

## Support

- **Issues**: Report bugs via GitHub Issues
- **Documentation**: See `doc/` directory
- **Contributing**: See [DEVELOPMENT.md](doc/development.md)

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

This extension was developed by Dave Sharpe (<dave.sharpe@datastone.ca>) using VS Code Copilot in agent mode with [speckit](https://github.com/github/spec-kit) for spec-driven development. See [Development Guide](doc/development.md#development-methodology) for details. My collegue Justin made a different solution for the same problem, which inspired me to create this extension.
