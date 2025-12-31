# Quickstart Guide: pg_num2int_direct_comp

**Feature**: 001-num-int-direct-comp  
**Purpose**: Quick reference for developers implementing exact comparison operators

## Development Prerequisites

```bash
# Required software
- PostgreSQL 12+ with development headers
- C compiler (gcc or clang) supporting C99
- make utility
- Git

# Verify installation
pg_config --version
gcc --version
make --version
```

## Project Setup

```bash
# Clone repository
git clone https://github.com/datastone-inc/pg_num2int_direct_comp.git
cd pg_num2int_direct_comp

# Checkout feature branch
git checkout 001-num-int-direct-comp

# Verify directory structure
ls -la
# Should see: pg_num2int_direct_comp.c, Makefile, *.sql, sql/, expected/
```

## Build and Install

```bash
# Clean build
make clean

# Compile extension
make

# Install (requires sudo/admin privileges)
sudo make install

# Verify installation
psql -c "CREATE EXTENSION pg_num2int_direct_comp;"
psql -c "SELECT 10::int4 = 10.0::numeric;"  # Should return true
```

## Test-First Development Workflow

### 1. Write Test First

Create test file in `sql/` directory:

```sql
-- sql/new_feature.sql
\set ECHO none
\pset format unaligned

-- Test description
SELECT 'test_name'::text AS test, (10::int4 = 10.0::numeric)::text AS result;
```

### 2. Create Expected Output

```bash
# Run test to generate output
psql -d postgres -f sql/new_feature.sql > expected/new_feature.out

# Edit expected output to match what SHOULD happen (not what currently happens)
vim expected/new_feature.out
```

### 3. Verify Test Fails

```bash
# Add test to Makefile REGRESS list
vim Makefile
# Add "new_feature" to REGRESS line

# Run tests - should fail
make installcheck
# Check regression.diffs to see failure
```

### 4. Implement Feature

Edit `pg_num2int_direct_comp.c`:

```c
/**
 * @brief Compare int4 with numeric for exact equality
 * @param 0 int4 value
 * @param 1 numeric value
 * @return Boolean true if exactly equal, false otherwise
 */
PG_FUNCTION_INFO_V1(int4_eq_numeric);
Datum
int4_eq_numeric(PG_FUNCTION_ARGS) {
  int32 i = PG_GETARG_INT32(0);
  Numeric num = PG_GETARG_NUMERIC(1);
  
  // Check for fractional part
  Numeric truncated = numeric_trunc(num);
  if (!DatumGetBool(DirectFunctionCall2(numeric_eq,
                                        NumericGetDatum(num),
                                        NumericGetDatum(truncated)))) {
    PG_RETURN_BOOL(false);
  }
  
  // Check range and compare
  // ... implementation details
  
  PG_RETURN_BOOL(true);
}
```

### 5. Register in SQL

Edit `pg_num2int_direct_comp--1.0.0.sql`:

```sql
CREATE FUNCTION int4_eq_numeric(int4, numeric)
  RETURNS boolean
  AS 'MODULE_PATHNAME', 'int4_eq_numeric'
  LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR = (
  LEFTARG = int4,
  RIGHTARG = numeric,
  FUNCTION = int4_eq_numeric,
  COMMUTATOR = =,
  NEGATOR = <>,
  RESTRICT = eqsel,
  JOIN = eqjoinsel
);
```

### 6. Rebuild and Test

```bash
# Rebuild
make clean && make

# Reinstall
sudo make install

# Drop and recreate extension
psql -c "DROP EXTENSION IF EXISTS pg_num2int_direct_comp CASCADE;"
psql -c "CREATE EXTENSION pg_num2int_direct_comp;"

# Run tests
make installcheck

# Check results
cat regression.diffs  # Should be empty if all tests pass
```

## Common Development Tasks

### Add New Comparison Function

```bash
# 1. Add function declaration to header
vim pg_num2int_direct_comp.h
# Add: PG_FUNCTION_INFO_V1(function_name);

# 2. Implement function in C
vim pg_num2int_direct_comp.c
# Add function implementation with doxygen comments

# 3. Register in SQL
vim pg_num2int_direct_comp--1.0.0.sql
# Add CREATE FUNCTION and CREATE OPERATOR

# 4. Add tests
vim sql/test_file.sql
# Add test cases for new function

# 5. Update expected output
vim expected/test_file.out

# 6. Rebuild and test
make clean && make && sudo make install && make installcheck
```

### Debug Compilation Errors

```bash
# Compile with verbose output
make clean
make CFLAGS="-Wall -Wextra -Werror -g -O0"

# Check for missing headers
grep -r "include" pg_num2int_direct_comp.c
pg_config --includedir-server  # Find PostgreSQL headers

# Verify PGXS setup
make -n  # Dry run to see make commands
```

### Debug Runtime Errors

```bash
# Enable core dumps
ulimit -c unlimited

# Run PostgreSQL with debugging
psql -d postgres

# In psql, enable verbose error output
\set VERBOSITY verbose

# Test problematic query
SELECT 10::int4 = 10.5::numeric;

# Check PostgreSQL logs
tail -f /var/log/postgresql/postgresql-*.log
```

### Run Specific Test

```bash
# Run single test file
psql -d postgres -f sql/numeric_int_ops.sql

# Compare with expected output
diff -u expected/numeric_int_ops.out <(psql -d postgres -f sql/numeric_int_ops.sql)

# Run specific test set
make installcheck REGRESS="numeric_int_ops null_handling"
```

## Code Style Checklist

Before committing, verify:

```bash
# Check K&R style (2-space indent, opening brace on same line)
# - Use editor formatting or manual inspection

# Verify copyright headers present
head -n 10 pg_num2int_direct_comp.c

# Check doxygen comments on all functions
grep -A 3 "^PG_FUNCTION_INFO_V1" pg_num2int_direct_comp.c

# Verify SQL uses UPPERCASE keywords
grep -E "^[a-z]+ " pg_num2int_direct_comp--1.0.0.sql  # Should find no lowercase SQL keywords

# Compile with strict warnings
make clean
make CFLAGS="-Wall -Wextra -Werror"

# Run all tests
make installcheck
```

## Quick Reference: Key Files

| File | Purpose | When to Edit |
|------|---------|-------------|
| `pg_num2int_direct_comp.c` | C implementation | Adding/modifying comparison functions |
| `pg_num2int_direct_comp.h` | Function declarations | Adding new public functions |
| `pg_num2int_direct_comp--1.0.0.sql` | Extension SQL | Registering functions/operators |
| `pg_num2int_direct_comp.control` | Extension metadata | Version updates |
| `Makefile` | Build configuration | Adding test files |
| `sql/*.sql` | Test inputs | Writing new tests |
| `expected/*.out` | Expected test outputs | Updating test expectations |
| `README.md` | User documentation | User-facing feature changes |
| `CHANGELOG.md` | Version history | Every release |

## Performance Verification

```bash
# Create test table with index
psql -d postgres <<EOF
CREATE TABLE perf_test (id SERIAL, value INT4);
CREATE INDEX idx_value ON perf_test(value);
INSERT INTO perf_test SELECT generate_series(1, 1000000);
ANALYZE perf_test;
EOF

# Verify index usage
psql -d postgres -c "EXPLAIN ANALYZE SELECT * FROM perf_test WHERE value = 500000::numeric;"
# Should show "Index Scan" or "Index Only Scan"

# Check execution time
psql -d postgres <<EOF
\timing on
SELECT COUNT(*) FROM perf_test WHERE value = 500000;
SELECT COUNT(*) FROM perf_test WHERE value = 500000::numeric;
\timing off
EOF
# Second query should be within 10% of first
```

## Troubleshooting

### Extension Won't Install

```bash
# Check PostgreSQL extensions directory
pg_config --sharedir
ls -la $(pg_config --sharedir)/extension/

# Verify library installed
pg_config --libdir
ls -la $(pg_config --libdir)/pg_num2int_direct_comp.so

# Reinstall with verbose output
sudo make install V=1
```

### Tests Failing

```bash
# Check regression.diffs
cat regression.diffs

# Run test manually to see output
psql -d postgres -f sql/failing_test.sql

# Compare line by line
diff -y expected/failing_test.out <(psql -d postgres -f sql/failing_test.sql)

# Check for platform-specific differences (e.g., float formatting)
psql -c "SELECT 1.0::float4;"  # May format differently on different platforms
```

### Index Not Being Used

```bash
# Verify support function registered
psql -d postgres -c "\df+ numeric_eq_int4_support"

# Check operator definition
psql -d postgres -c "SELECT oprname, oprcode, oprleft::regtype, oprright::regtype, oprresult::regtype FROM pg_operator WHERE oprname = '=' AND oprleft = 'numeric'::regtype AND oprright = 'int4'::regtype;"

# Force index usage to verify support function works
psql -d postgres -c "SET enable_seqscan = off; EXPLAIN SELECT * FROM test_table WHERE intcol = 10.0::numeric;"
```

## Next Steps

1. Review [data-model.md](data-model.md) for complete operator structure
2. Check [contracts/operators.md](contracts/operators.md) for operator specifications
3. See [contracts/test-cases.md](contracts/test-cases.md) for comprehensive test coverage
4. Read constitution at `.specify/memory/constitution.md` for coding standards

## Support

- Documentation: `doc/` directory
- Examples: `sql/` test files
- Constitution: `.specify/memory/constitution.md`
- Planning docs: `specs/001-num-int-direct-comp/`
