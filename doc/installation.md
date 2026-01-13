# Installation Guide

## Prerequisites

### Required Software

- **PostgreSQL**: Version 12 or later
- **Build Tools**: GCC or Clang with C99 support
- **PostgreSQL Development Headers**:
  - Debian/Ubuntu: `postgresql-server-dev-<version>`
  - Red Hat/CentOS: `postgresql<version>-devel`
  - macOS (Homebrew): Included with `postgresql`
  - Windows: PostgreSQL installer includes development files

### Verify Prerequisites

```bash
# Check PostgreSQL version
psql --version

# Check pg_config (required for PGXS)
pg_config --version

# Verify development headers
pg_config --includedir-server
```

## Building from Source

### Clone Repository

```bash
git clone https://github.com/datastone-inc/pg_num2int_direct_comp.git
cd pg-num2int-direct-comp
```

### Build Extension

```bash
make
```

Expected output:
```
gcc -Wall -Wextra -Werror -std=gnu99 -fPIC ...
```

### Run Tests (Optional)

```bash
make installcheck
```

All tests should pass (100% pass rate required).

### Install Extension

```bash
sudo make install
```

This copies:

- `pg_num2int_direct_comp.so` → `$libdir`
- `pg_num2int_direct_comp--1.0.0.sql` → `$sharedir/extension`
- `pg_num2int_direct_comp.control` → `$sharedir/extension`

## Enabling in Database

### Create Extension

Connect to your database and run:

```sql
CREATE EXTENSION pg_num2int_direct_comp;
```

### Verify Installation

```sql
-- Check extension is loaded
\dx pg_num2int_direct_comp

-- List all extension objects (108 operators, 138 functions, etc.)
\dx+ pg_num2int_direct_comp

-- Test precision loss detection (stock PostgreSQL returns true for both)
SELECT 16777216::int4 = 16777217::float4;  -- true (float4 rounds to 16777216)
SELECT 16777217::int4 = 16777217::float4;  -- false (extension detects mismatch!)
```

## Quick Start

### Example 1: Detecting Float Precision Loss

```sql
-- Example: Detecting Float Precision Loss
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
-- Example: Planner Transitivity Inference
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
-- Example: Index-Optimized Queries
-- With extension:
CREATE TABLE measurements (id SERIAL, value INT4);
CREATE INDEX ON measurements(value);
INSERT INTO measurements (value) SELECT generate_series(1, 1000000);

-- Uses index efficiently (no cast on indexed column)
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 500::numeric;
--                       QUERY PLAN
-- ---------------------------------------------------------
--  Index Scan using measurements_value_idx on measurements
--    Index Cond: (value = 500)
-- (2 rows)
```

### Example 4: Fractional Comparisons

```sql
-- Example: Fractional Comparisons
-- With extension:
-- Fractional values never equal integers
SELECT 10::int4 = 10.5::numeric;  -- false
SELECT 10::int4 < 10.5::numeric;  -- true
```

### Example 5: Range Queries

```sql
-- Example: Range Queries
CREATE TABLE inventory (item_id INT4, quantity INT4);
INSERT INTO inventory VALUES (1, 10), (2, 11), (3, 12);

-- Exact boundary handling
SELECT * FROM inventory WHERE quantity > 10.5::float8;
--  item_id | quantity
-- ---------+----------
--        2 |       11
--        3 |       12
-- (2 rows)
```

### Example 6: Hash Joins for Large Tables

```sql
-- Example: Hash Joins for Large Tables
-- Hash joins work automatically for large table joins
CREATE TABLE sales (id SERIAL, amount NUMERIC(10,2));
CREATE TABLE targets (id SERIAL, threshold INT4);
INSERT INTO sales SELECT generate_series(1, 1000000), random() * 1000;
INSERT INTO targets SELECT generate_series(1, 1000000), (random() * 1000)::int4;

-- Planner chooses hash join for large equijoin
EXPLAIN (COSTS OFF) SELECT COUNT(*) FROM sales s JOIN targets t ON s.amount = t.threshold;
--                  QUERY PLAN
-- ---------------------------------------------
--  Aggregate
--    ->  Hash Join
--          Hash Cond: (t.threshold = s.amount)
--          ->  Seq Scan on targets t
--          ->  Hash
--                ->  Seq Scan on sales s
-- (6 rows)
```

### Example 7: Type Aliases

```sql
-- Example: Type Aliases
-- Serial types work automatically
CREATE TABLE users (id SERIAL, score INT4);
SELECT * FROM users WHERE id = 100.0::numeric;  -- Uses exact comparison

-- Decimal type works automatically
SELECT 10::int4 = 10.0::decimal;  -- true (decimal is alias for numeric)
```

### Example 8: Query Optimization - Impossible Predicate Detection

The extension's support functions transform constant predicates during query planning for optimal performance:

```sql
-- Example: Query Optimization - Impossible Predicate Detection
-- Assuming: CREATE TABLE measurements (id SERIAL, value INT4);
--           CREATE INDEX ON measurements(value);

-- Impossible predicate detection: integer can never equal fractional value
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 10.5::numeric;
--         QUERY PLAN
-- --------------------------
--  Result
--    One-Time Filter: false
-- (2 rows)
-- The planner recognizes this is impossible and returns rows=0 estimate

-- Exact match transformation: uses native integer operator
EXPLAIN (COSTS OFF) SELECT * FROM measurements WHERE value = 100::numeric;
--                        QUERY PLAN
-- ---------------------------------------------------------
--  Index Scan using measurements_value_idx on measurements
--    Index Cond: (value = 100)
-- (2 rows)
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

## Troubleshooting

### Build Errors

**Error**: `pg_config: command not found`

**Solution**: Add PostgreSQL bin directory to PATH:
```bash
export PATH=/usr/pgsql-<version>/bin:$PATH  # Red Hat/CentOS
export PATH=/usr/lib/postgresql/<version>/bin:$PATH  # Debian/Ubuntu
```

**Error**: `postgres.h: No such file or directory`

**Solution**: Install development headers:
```bash
sudo apt install postgresql-server-dev-<version>  # Debian/Ubuntu
sudo yum install postgresql<version>-devel         # Red Hat/CentOS
```

### Installation Errors

**Error**: `permission denied` during `make install`

**Solution**: Use sudo or install to custom directory:
```bash
sudo make install
# OR
make install DESTDIR=/custom/path
```

### Runtime Errors

**Error**: `could not access file "$libdir/pg_num2int_direct_comp"`

**Solution**: Verify library is in correct location:
```bash
pg_config --pkglibdir
ls $(pg_config --pkglibdir)/pg_num2int_direct_comp.so
```

## Platform-Specific Notes

### Linux

Standard PGXS build works on all distributions.

### macOS

Homebrew PostgreSQL includes all required headers. Use:
```bash
make PG_CONFIG=/opt/homebrew/bin/pg_config  # Apple Silicon
make PG_CONFIG=/usr/local/bin/pg_config     # Intel
```

### Windows

Build with MinGW or MSVC:
```cmd
SET PATH=C:\Program Files\PostgreSQL\<version>\bin;%PATH%
make
```

## Uninstallation

```sql
-- In database
DROP EXTENSION pg_num2int_direct_comp;
```

```bash
# Remove files
sudo make uninstall
```

## Next Steps

- [Operator Reference](operator-reference.md) - Operator reference and usage guide
- [Development Guide](development.md) - Contributing and testing
