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

PostgreSQL database schema drift and software evolution mean that cross-type comparisons between numeric types (numeric, float4, float8) and integer types (int2, int4, int8) can occur. However, PostgreSQL's default behavior injects implicit casts that cause two critical problems:

1. **Precision Loss**: Implicit casting can destroy precision, making distinct values appear equal, e.g., IEEE floats and doubles cannot represent all the integral values, e.g., there is no representation of the int 16777217 as a float4. That int value is rounded down to float4 value 16777216. PostgreSQL compares `16777217::int4 = 16777216::float4` as `true` (!) because it implicitly casts to float4 and then compares as true by default due to the rounding "error". Many think this should be `false` as the source values are not equal and this extension fixes such comparisons to be exact.
2. **Performance Degradation**: Implicit casts prevent index SARG predicates on integer key columns (degrading to whole table sequential scans) and also disables hash joins for cross-type comparisons

This extension solves both problems by providing exact comparison operators (=, <>, <, >, <=, >=) that:

- Are implemented in C functions for performance
- Detect precision mismatches without implicit casting
- Support direct btree index SARG lookups and range scans on integer key columns
- Support hash joins for large table cross-type equijoins
- Maintain PostgreSQL's correctness guarantees with regard to transitive equality, e.g., no invalid transitive equality: if `a = b` and `b = c`, then `a = c` only when mathematically valid. It remains not valid if `a` is integral (int2, int4, int8) and `b` is numeric, float4, or float8, e.g., `16

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

### Merge Joins Not Supported

**What**: PostgreSQL's merge join optimization cannot be used with these operators.

**Why**: Merge joins require operator family membership on both sides of the join. Adding operators to the `integer_ops` family would enable invalid transitive inferences (e.g., the planner could incorrectly infer that `int_col = 10.5` from `int_col = 10` and `10 = 10.5::numeric`), violating exact comparison semantics.

**Impact**: Queries will use indexed nested loop joins or hash joins instead. For most real-world queries with selective predicates and indexes, indexed nested loop joins provide equal or better performance than merge joins.

**Details**: See [specs/001-num-int-direct-comp/research.md](specs/001-num-int-direct-comp/research.md#82-merge-joins-are-architecturally-impossible) for the complete architectural analysis.

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
