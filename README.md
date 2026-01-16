<!-- markdownlint-disable MD033 MD041 -->
<div align="center">

<img src="assets/datastone-logo.svg" alt="dataStone Logo" width="400"/>

# pg_num2int_direct_comp

[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-12%2B-blue.svg)](https://www.postgresql.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Language](https://img.shields.io/badge/Language-C-orange.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Extension](https://img.shields.io/badge/Type-PostgreSQL%20Extension-purple.svg)](https://www.postgresql.org/docs/current/extend-extensions.html)

Exact numeric-to-integer comparison operators for PostgreSQL

</div>
<!-- markdownlint-enable MD033 MD041 -->

---

## Table of Contents

- [Overview](#overview)
  - [Why not just fix the application to use consistent types?](#why-not-just-fix-the-application-to-use-consistent-types)
  - [What's the risk of custom operators?](#whats-the-risk-of-custom-operators)
  - [The Problem: Implicit Casting](#the-problem-implicit-casting)
    - [Implicit Cast: Mathematically Incorrect Comparison Results](#implicit-cast-mathematically-incorrect-comparison-results)
    - [Implicit Cast: Poor Query Performance](#implicit-cast-poor-query-performance)
    - [Why PostgreSQL Cannot Infer Transitive Equality](#why-postgresql-cannot-infer-transitive-equality)
    - [With the Extension: Transitive Equality Correct Results (and Better Plans)](#with-the-extension-transitive-equality-correct-results-and-better-plans)
    - [Example: Implicit Casting Problems](#example-implicit-casting-problems)
  - [The Solution: Exact Cross-Type Comparison](#the-solution-exact-cross-type-comparison)
    - [How Exact Comparison Works](#how-exact-comparison-works)
    - [Index Optimization via Support Functions](#index-optimization-via-support-functions)
    - [Hash Support for Joins, GROUP BY, and DISTINCT](#hash-support-for-joins-group-by-and-distinct)
    - [Merge Join Support via Btree Operator Families](#merge-join-support-via-btree-operator-families)
    - [Performance: Correctness Enables Speed](#performance-correctness-enables-speed)
  - [Key Features](#key-features)
  - [Supported Types](#supported-types)
- [SQL Equivalents With and Without the Extension](#sql-equivalents-with-and-without-the-extension)
- [Installation](#installation)
  - [Prerequisites](#prerequisites)
  - [Build and Install](#build-and-install)
  - [Enable in Database](#enable-in-database)
- [Support Function Chicken Switch](#support-function-chicken-switch)
  - [pg_num2int_direct_comp.enable_support_functions](#pg_num2int_direct_compenable_support_functions)
- [Limitations](#limitations)
- [Documentation](#documentation)
- [Technical Details](#technical-details)
  - [Project Structure](#project-structure)
  - [Implementation Notes](#implementation-notes)
- [Compatibility](#compatibility)
- [Support](#support)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

Database schemas drift and SQL expressions evolve such that cross-type comparisons between numeric types (numeric, float4, float8) and integer types (int2, int4, int8) can occur. If this is happening for you, PostgreSQL's default behavior injects implicit casts that cause two critical problems: **mathematically incorrect comparison results** and **poor query performance**. This extension provides **exact cross-type comparison operators** to address both problems.

### Why not just fix the application to use consistent types?

While type consistency is the ideal, real-world databases face type mismatches from:

- **Joins in evolving systems** between tables designed by different teams over time
- **API integrations** that pass parameters as `numeric`/`float` when columns are `integer`
- **ORMs and frameworks** that default to `numeric`/`float` for user inputs, even for `integer` IDs
- **Evolving SQL expressions** that introduce mixed-type comparisons unintentionally
- **Ad-hoc queries** by analysts exploring data without strict type discipline

### What's the risk of custom operators?

**Minimal operational risk:**

- Uses existing PostgreSQL comparison logic (no new C code for core logic)
- Follows established operator precedence and behavior patterns
- Drops cleanly: `DROP EXTENSION` removes all operators instantly
- No schema lock-in: queries work without the extension (just slower/less precise)

**Switching Behavior:** Users switching between standard PostgreSQL and this extension have [SQL Equivalents With and Without the Extension](#sql-equivalents-with-and-without-the-extension).

### The Problem: Implicit Casting

When no direct cross-type comparison operator exists, PostgreSQL implicitly casts operands to a common type before comparing. The type resolution depends on which types are involved:

- **Float vs. integer** (e.g., `float4 = int4`): Both operands are implicitly cast to `float8`, since it's the *preferred type* when floats are involved
- **Numeric vs. integer** (e.g., `numeric = int8`): The integer is implicitly cast to `numeric`

#### Implicit Cast: Mathematically Incorrect Comparison Results

When integers are implicitly cast to floating-point types, precision can be lost because
IEEE 754 floating-point types have limited mantissa bits:

- **float4**: 23-bit mantissa → exact integers only up to 2²⁴ (16,777,216)
- **float8**: 52-bit mantissa → exact integers only up to 2⁵³ (9,007,199,254,740,992)

Beyond these limits, integers round to the nearest representable float value, causing silent precision loss in comparisons.

#### Implicit Cast: Poor Query Performance

When integers are implicitly cast, the PostgreSQL query planner is crippled:

- cannot exploit btree indexes on indexed columns that are implicitly cast, often reducing to sequential scans.
- cannot choose indexed nested-loop joins or merge joins for cross-type equality conditions
- cannot infer transitive equality (see next)

#### Why PostgreSQL Cannot Infer Transitive Equality

Transitivity is a fundamental property of equality: if `a = b` and `b = c`, then `a = c` should hold. However, PostgreSQL's implicit casting violates this property, as shown in the next SQL example a=b and b=c however a≠c. So in stock PostgreSQL, the implicit cast expressions intentionally don't qualify for operator class membership, and the planner does not infer any transitive relationships because they would be wrong, see [Behavior of B-Tree Operator Classes](https://www.postgresql.org/docs/current/btree.html#BTREE-BEHAVIOR).

```sql
-- Example: Transitive Equality Violations not detected at the float8 2^53 boundary (different at the last digit)
-- Without extension (stock PostgreSQL):
SELECT 9007199254740993::int8 = 9007199254740993::float8 AS "a=b"  -- a=b TRUE (int8 cast to float8, rounds)
     , 9007199254740993::float8 = 9007199254740992::int8 AS "b=c"  -- b=c TRUE (int8 cast to float8, rounds)
     , 9007199254740993::int8 = 9007199254740992::int8   AS "a=c"; -- a=c FALSE!
--  a=b | b=c | a=c
-- -----+-----+-----
--  t   | t   | f
-- (1 row)

-- If PostgreSQL inferred a=c from a=b AND b=c, it would get wrong results.
-- PostgreSQL knows this, so it CANNOT use transitivity for plan optimization.
```

This limitation affects query optimization: the planner cannot propagate equality conditions across joins or eliminate redundant predicates when cross-type comparisons are involved.

#### With the Extension: Transitive Equality Correct Results (and Better Plans)

```sql
-- Example: Transitive Equality Violations detected at the float8 2^53 boundary (different at the last digit)
SELECT 9007199254740993::int8 = 9007199254740993::float8 AS "a=b"  -- a=b FALSE (b lossy NOT EQUAL)
     , 9007199254740993::float8 = 9007199254740992::int8 AS "b=c"  -- b=c TRUE (int8 cast to rounded float8)
     , 9007199254740993::int8 = 9007199254740992::int8   AS "a=c"; -- a=c FALSE!
--  a=b | b=c | a=c
-- -----+-----+-----
--  f   | t   | f
-- (1 row)
```

With the extension, the cross-type equality operators provide mathematically correct results, preserving transitivity where applicable. The planner can now safely infer relationships and optimize queries effectively, e.g., if a=b and b=c, then correctly infer a=c.

Note that the extension also adds such planning inference capabilities for integer x numeric comparisons which is missing in stock PostgreSQL.

> **This extension guarantees mathematical correctness.** All comparison operators satisfy reflexivity, symmetry, and transitivity for equality, and irreflexivity, transitivity, and trichotomy for ordering. Because the extension's operators are mathematically correct, PostgreSQL's planner CAN safely infer transitivity for optimizations.

#### Example: Implicit Casting Problems

The following example demonstrates how stock PostgreSQL produces "wrong" results and suffers poor performance due to forced sequential scans and lack of transitivity inference:

```sql
-- Example: Wrong results, seq scans, and no transitivity from cross-type comparison
-- Without extension (stock PostgreSQL):
CREATE TABLE orders (orderid int8 PRIMARY KEY, customer text);
CREATE TABLE order_items (id serial, orderid int8, product text);
CREATE INDEX ON order_items(orderid);

-- Bulk insert, plus two orders with adjacent IDs at the float8 precision boundary
INSERT INTO orders SELECT g, 'customer' || g FROM generate_series(1, 100000) g;
INSERT INTO orders VALUES (9007199254740992, 'Alice'), (9007199254740993, 'Bob');

INSERT INTO order_items (orderid, product) SELECT g, 'product' || g FROM generate_series(1, 100000) g;
INSERT INTO order_items VALUES (DEFAULT, 9007199254740992, 'Widget'), (DEFAULT, 9007199254740993, 'Gadget');
ANALYZE orders; ANALYZE order_items;

-- Application queries for Alice's order (9007199254740992) using float8 parameter
EXPLAIN (COSTS OFF)
SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;
--                                            QUERY PLAN
-- -------------------------------------------------------------------------------------------------
--  Hash Join
--    Hash Cond: (oi.orderid = o.orderid)
--    ->  Seq Scan on order_items oi                              ← NO TRANSITIVITY! (filter not pushed)
--    ->  Hash
--          ->  Seq Scan on orders o                              ← SLOW! (100K+ rows scanned)
--                Filter: ((orderid)::double precision = '9.007199254740992e+15'::double precision)
-- (6 rows)

SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;
--  customer | product
-- ----------+---------
--  Alice    | Widget
--  Bob      | Gadget                                              ← WRONG! (asked for Alice's order)
-- (2 rows)
```

**Summary without the extension installed:**

- We asked for ONE order (Alice's 9007199254740992) but got TWO rows (Alice AND Bob), effectively violating the PK constraint.
- WRONG: both int8 IDs round to the same float8 value (9007199254740992.0)
- SLOW: seq scans on BOTH tables because PostgreSQL casts orderid to float8
- NO TRANSITIVITY: planner cannot infer order_items.orderid = constant from join + filter

**Same exact SQL explain and query with the extension installed:**

```sql
-- Example: Correct results with index scans and transitive equality
-- Application queries for Alice's order (9007199254740992) using float8 parameter
EXPLAIN (COSTS OFF)
SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;
--                                            QUERY PLAN
-- -------------------------------------------------------------------------------------------------
--  Nested Loop
--    ->  Index Scan using orders_pkey on orders o               ← FAST! (index scan)
--          Index Cond: (orderid = 9007199254740992)
--    ->  Index Scan using order_items_orderid_idx on order_items oi  ← FAST! (index scan)
--          Index Cond: (orderid = 9007199254740992)              ← TRANSITIVITY inferred!
-- (6 rows)

SELECT o.customer, oi.product FROM orders o
JOIN order_items oi ON o.orderid = oi.orderid
WHERE o.orderid = 9007199254740992::float8;
--  customer | product
-- ----------+---------
--  Alice    | Widget
-- (1 row)
```

**Summary with the extension installed:**

- We get the CORRECT result: only Alice's order is returned, i.e., 9007199254740992::int8 ≠ 9007199254740993::int8
- FAST: index lookups on BOTH tables (no casts on indexed columns)
- TRANSITIVITY: filter on orders.orderid is pushed to order_items through the join

### The Solution: Exact Cross-Type Comparison

This extension provides 108 comparison operators that compare integer and numeric types **directly**, without implicit casting and adds them to the appropriate operator classes. The comparison logic ensures mathematical correctness. This enables PostgreSQL to use indexes, hash joins, and merge joins effectively. This extension also implements [support function callbacks](https://www.postgresql.org/docs/current/xfunc-optimization.html) that transform cross-type comparisons into native integer comparisons when possible.

#### How Exact Comparison Works

Implemented in C for efficiency, e.g., int=num (float or numeric) equality calculated roughly as:

1. **Special values**: Handle if num is NaN or ±Infinity according to PostgreSQL semantics
2. **Bounds check**: If num is outside the integer type's range (e.g., > 2³¹-1 for int4), return the appropriate ordering result immediately
3. **Truncate to integer part**: Extract the integer portion of the num value
4. **Check for nonzero fractional part**: If num has any nonzero fractional component, equality is false. (For ordering, the nonzero fractional part determines if num is greater/less than the truncated integer)
5. **Compare integer parts**: If truncated value ≠ integer, they're not equal

This approach is **mathematically correct** because:

- No precision is lost, so we never cast integers to floats
- Fractional parts are preserved and compared exactly
- Values outside representable ranges are handled correctly
- Special values (NaN, ±Infinity) follow standard PostgreSQL semantics. In particular, NaN > all integer values [search "NaN" here Comparison](https://www.postgresql.org/docs/current/datatype-numeric.html)

#### Index Optimization via Support Functions

The extension uses PostgreSQL's [`SupportRequestSimplify`](https://www.postgresql.org/docs/current/xfunc-optimization.html) mechanism to enable index usage for all comparison operators (`=`, `<`, `<=`, `>`, `>=`, `<>`). When the query contains a cross-type comparison like `intkey >= 10.0::numeric` or `intkey = $1` (where `$1` is a numeric parameter and the statement is prepared with a custom plan), the support function performs these optimizations:

1. The support function intercepts the comparison
2. It checks if the numeric value is exactly representable as the integer type
3. If yes: transforms to `intkey >= 10` (pure integer comparison), enabling index scan
4. If no: adjusts the comparison appropriately, e.g., `intkey > 10.5` becomes `intkey >= 11`, and `intkey = 10.5` becomes `FALSE` (impossible predicate)

```sql
-- Example: API passes product ID as numeric parameter
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

#### Hash Support for Joins, GROUP BY, and DISTINCT

The extension provides cross-type hash functions that ensure equal values hash identically regardless of type. This enables PostgreSQL to use hash-based operations across type boundaries:

**How it works:**

- Cross-type equality operators are added to the `numeric_ops` hash operator family (for numeric × integer comparisons)
- Cross-type equality operators are added to the `float_ops` hash operator family (for float × integer comparisons)
- When hashing an integer for cross-type comparison with numeric, it converts to numeric (lossless). For float comparisons, it hashes the float8 representation. Collisions at precision boundaries are correctly filtered by the (exact) equality check.
  - Note: the same number of hash buckets are used with or without the extension, so there is no change in hash table sizing or performance characteristics.
- This guarantees `a = b → hash(a) = hash(b)` across types, enabling hash joins

GROUP BY and DISTINCT also benefit when mixing types, as PostgreSQL can use HashAggregate instead of Sort-based approaches.

Note that stock PostgreSQL also supports these hash operations with implicit cast equi-joins, so this extension is fully compatible. So the hash join plans are the same as stock PostgreSQL, but hash collisions are resolved with the extension's exact comparison operators.

#### Merge Join Support via Btree Operator Families

For merge joins to work across type boundaries, PostgreSQL requires that cross-type comparison operators belong to the same btree operator *family*. This extension registers all cross-type operators into the appropriate btree families:

**How it works:**

- Cross-type operators are added to the `integer_ops`, `numeric_ops`, and `float_ops` btree families
- All six comparison operators (`<`, `<=`, `=`, `>=`, `>`, `<>`) are registered for each type pair
- The operators share common btree comparison functions that return consistent ordering across types
- This enables PostgreSQL's planner to recognize that cross-type comparisons can be merge-joined directly

**Why this matters:**

Stock PostgreSQL cannot use merge joins for cross-type comparisons because the implicit-cast operators don't belong to a common operator family. The planner falls back to hash joins or nested loops. With this extension:

- **Pre-sorted inputs** (e.g., from index scans) can be merge-joined efficiently
- **Large sorted datasets** benefit from streaming merge without hash table memory overhead
- **Memory efficiency**: Merge joins don't require building a hash table, reducing memory pressure

**Implementation complexity:**

Adding operators to btree families requires careful attention to:

- Consistent ordering semantics across all type combinations
- Proper strategy numbers (1-5 for `<`, `<=`, `=`, `>=`, `>`)
- Correct `oprcom` and `oprnegate` relationships between operators
- Registration in both directions (e.g., `int8 < numeric` and `numeric > int8`)
- Cleanup on extension drop (see [Implementation Notes](#implementation-notes))

See [Float × Integer Btree Ops](specs/002-float-btree-ops/research.md) for more detailed design rationale.

#### Performance: Correctness Enables Speed

Is this per-comparison logic slower than PostgreSQL's simple cast-and-compare?

**Per-operation**: Yes. The exact comparison involves bounds checking, truncation, and fractional-part detection which is more expensive than a raw `numeric`/`float` comparison. However, this overhead is in C. It is negligible compared to I/O costs and is usually outweighed by query-level optimizations.

**Overall query performance**: This extension is typically faster than stock PostgreSQL for cross-type comparisons because:

1. **Index scans instead of sequential scans**: Stock PostgreSQL casts the indexed column, forcing sequential scans. This extension enables index usage, reducing I/O and CPU usage.
2. **SupportRequestSimplify**: Transforms cross-type comparisons into native integer comparisons when possible, enabling the planner to generate more efficient plans.
3. **Merge joins instead of nested loops**: With index-optimized operators, the planner can choose merge joins for sorted inputs rather than nested loops that repeatedly scan one table.
4. **Transitive equality inference**: The planner can propagate equality conditions, enabling more efficient filtering and join strategies.

The irony is that PostgreSQL's "fast" implicit cast approach is actually *slow* for real queries because it defeats the planner. **Correctness and performance are not at odds here. The correct approach enables fast query plans.**

**Benchmark highlights:**

- **Index lookups**: Sub-millisecond on 1M+ row tables
- **Hash joins**: Efficient cross-type hashing, faster than cast-based joins
- **Merge joins**: Faster than stock with less memory usage
- **Overhead**: <10% vs native integer comparisons

For comprehensive performance testing, run the benchmark suite:

```bash
# Quick performance test (~seconds, 100K rows)
make installcheck REGRESS=performance

# Full benchmark (~70 seconds, 1M rows)
make installcheck REGRESS=benchmark
```

See [Benchmark Guide](doc/benchmark.md) for detailed methodology and analysis.

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

## SQL Equivalents With and Without the Extension

Users migrating between standard PostgreSQL and this extension can use these expressions to switch comparison behavior:

| Comparison Type | With Extension | Stock PostgreSQL (both → float8) | Exact without Extension |
| --------------- | -------------- | --------------------------------- | ----------------------- |
| `int4 = float4` | `int4_col = float4_col` | `int4_col::float8 = float4_col::float8` | `int4_col::numeric = float4_col::numeric` |
| `int8 = float4` | `int8_col = float4_col` | `int8_col::float8 = float4_col::float8` | `int8_col::numeric = float4_col::numeric` |
| `int8 = float8` | `int8_col = float8_col` | `int8_col::float8 = float8_col` | `int8_col::numeric = float8_col::numeric` |
| `int4 = numeric` | `int4_col = numeric_col` | `int4_col::numeric = numeric_col` | Same (already exact) |

**Key Insights:**

- **Extension operators**: Exact semantics, index-optimized, mathematically correct
- **Explicit float8 casts**: Emulate stock PostgreSQL's implicit behavior (precision loss possible)
- **Numeric casts**: Force exact comparison without extension, however with the extension is index-incompatible and generally better plans.

## Installation

See the [Installation Guide](doc/installation.md) for more detailed instructions.

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

For usage examples, see the [Quick Start Guide](doc/installation.md#quick-start) in the Installation documentation.

## Support Function Chicken Switch

The extension provides a configuration parameter to control query optimization behavior:

### pg_num2int_direct_comp.enable_support_functions

Controls whether the extension's query optimizations are active.

**Default**: `on`
**Context**: Can be changed by any user (PGC_USERSET)
**Values**: `on`, `off`

**Purpose**:
- **`on`**: Enable SupportRequestSimplify optimization
- **`off`**: Disable optimizations for testing, troubleshooting, or compatibility

**Usage**:

```sql
-- Check current setting
SHOW pg_num2int_direct_comp.enable_support_functions;

-- Disable optimizations for current session
SET pg_num2int_direct_comp.enable_support_functions = off;

-- Re-enable optimizations
SET pg_num2int_direct_comp.enable_support_functions = on;

-- Verify optimization is disabled (should show cross-type comparison)
SET pg_num2int_direct_comp.enable_support_functions = off;
EXPLAIN SELECT * FROM table WHERE 10.0::float8 = integer_column;
-- Filter: ('10'::double precision = table.integer_column)

-- Verify optimization is enabled (should show same-type comparison)
SET pg_num2int_direct_comp.enable_support_functions = on;
EXPLAIN SELECT * FROM table WHERE 10.0::float8 = integer_column;
-- Filter: (table.integer_column = 10)
```

**When to disable**: Testing original PostgreSQL behavior, troubleshooting query plans, or if optimizations cause unexpected behavior.

---

## Limitations

Numeric x float are not supported because IEEE 754 floating-point numbers do not have a fixed precision, making exact comparison with numeric types infeasible.

## Documentation

- [Installation Guide](doc/installation.md) - Detailed setup instructions
- [Operator Reference](doc/operator-reference.md) - Operator reference and usage guide
- [Benchmark Guide](doc/benchmark.md) - Performance testing methodology and results
- [Development Guide](doc/development.md) - Contributing and testing
- Research & Design:
  - [Numeric × Integer Comparison](specs/001-num-int-direct-comp/research.md) - Architectural decisions and trade-offs
  - [Float × Integer Btree Ops](specs/002-float-btree-ops/research.md) - Float comparison design

## Technical Details

- **Language**: C (C99)
- **Build System**: PGXS
- **Test Framework**: pg_regress
- **License**: MIT

### Project Structure

See [Development Guide](doc/development.md#project-structure) for complete layout.

### Implementation Notes

This extension uses two PostgreSQL mechanisms to ensure correct behavior across DROP/ALTER..UPDATE/CREATE EXTENSION cycles:

1. **Event Trigger for Operator Family Cleanup**: When adding built-in operators to existing operator families (like `integer_ops`), PostgreSQL doesn't track these memberships as extension-owned. An event trigger on `ddl_command_start` cleans up `pg_amop` entries when the extension is dropped, preventing "operator already exists" errors on reinstall.

2. **Syscache Invalidation Callback**: The support function maintains a static cache of operator OIDs for performance. A callback registered via `CacheRegisterSyscacheCallback(OPEROID, ...)` in `_PG_init()` automatically invalidates this cache when operators change, ensuring index optimization works correctly after extension reinstallation.

See [Research & Design](specs/001-num-int-direct-comp/research.md) sections 7.6 and 7.7 for detailed explanations.

## Compatibility

- PostgreSQL 12, 13, 14, 15, 16
- Linux, macOS, Windows (via MinGW)

## Support

- **Issues**: Report bugs via GitHub Issues
- **Documentation**: See `doc/` directory
- **Contributing**: See [DEVELOPMENT.md](doc/development.md)

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

This extension was developed by Dave Sharpe (<dave.sharpe@datastone.ca>) using AI Assistance of VS Code Copilot in agent mode with [speckit](https://github.com/github/spec-kit) for spec-driven development. See [Development Guide](doc/development.md#development-methodology) for details. My collegue Justin made a different solution for the same problem, which inspired me to create this extension.
