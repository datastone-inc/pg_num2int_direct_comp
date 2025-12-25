# pg_num2int_direct_comp

PostgreSQL extension providing exact comparison operators between numeric/float types and integer types.

## Overview

This extension adds comparison operators (=, <>, <, >, <=, >=) for comparing inexact numeric types (numeric, float4, float8) with integer types (int2, int4, int8) using exact semantics. Unlike PostgreSQL's default behavior which casts types and may lose precision, these operators detect precision mismatches.

### Key Features

- **Exact Precision**: `16777216::bigint = 16777217::float4` correctly returns `false` (detects float4 precision loss)
- **Index Optimization**: Queries like `WHERE intcol = 10.0::numeric` use btree indexes via indexed nested loop joins
- **Hash Join Support**: Large table joins use hash joins when appropriate
- **Complete Coverage**: 72 operators (54 forward + 18 commutator) covering all combinations of (numeric, float4, float8) × (int2, int4, int8)
- **Type Alias Support**: Works with serial, bigserial, smallserial, and decimal types

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
SELECT 16777216::bigint = 16777217::float4;  -- true (incorrect due to float precision)

-- With pg_num2int_direct_comp (exact comparison)
CREATE EXTENSION pg_num2int_direct_comp;
SELECT 16777216::bigint = 16777217::float4;  -- false (correct - detects precision mismatch)
```

### Example 2: Index-Optimized Queries

```sql
CREATE TABLE measurements (id SERIAL, value INT4);
CREATE INDEX ON measurements(value);
INSERT INTO measurements (value) SELECT generate_series(1, 1000000);

-- Uses index efficiently
EXPLAIN SELECT * FROM measurements WHERE value = 500.0::numeric;
-- Result: Index Scan using measurements_value_idx
```

### Example 3: Fractional Comparisons

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

### Example 6: Type Aliases

```sql
-- Serial types work automatically
CREATE TABLE users (id SERIAL, score INT4);
SELECT * FROM users WHERE id = 100.0::numeric;  -- Uses exact comparison

-- Decimal type works automatically
SELECT 10::int4 = 10.0::decimal;  -- true (decimal is alias for numeric)
```

## Documentation

- [Installation Guide](doc/installation.md) - Detailed setup instructions
- [User Guide](doc/user-guide.md) - Usage patterns and best practices
- [API Reference](doc/api-reference.md) - Complete operator reference
- [Development Guide](doc/development.md) - Contributing and testing

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
