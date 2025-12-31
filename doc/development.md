# Development Guide

## Development Methodology

This extension was developed using **VS Code Copilot in agent mode** with [speckit](https://github.com/github/spec-kit), a spec-driven development framework.

### Workflow

1. **Specification First**: Features are defined in `specs/` with detailed requirements, research, and implementation plans before any code is written
2. **Agent-Assisted Implementation**: VS Code Copilot (Claude) implements features based on specifications, with human review and guidance
3. **Test-Driven Verification**: All features have regression tests that validate behavior against the specification
4. **Iterative Refinement**: Specifications and implementations are refined through agent-human collaboration

### Specification Structure

The `specs/001-num-int-direct-comp/` directory contains:

- **spec.md** - Feature requirements and acceptance criteria
- **research.md** - PostgreSQL internals research and design decisions
- **plan.md** - Implementation approach and phases
- **tasks.md** - Detailed task breakdown with status tracking
- **implementation-notes.md** - Development journal with lessons learned

### Benefits of This Approach

- **Traceable decisions**: Design rationale is documented in research.md
- **Reproducible development**: Specifications enable consistent agent behavior
- **Quality assurance**: Human review at specification and implementation stages
- **Knowledge capture**: Implementation notes preserve context for future maintenance

## Contributing

Contributions are welcome! This guide covers development setup, testing, and contribution guidelines.

## Development Setup

### Prerequisites

- PostgreSQL 12+ with development headers
- GCC or Clang with C99 support
- Git
- Make

### Clone and Build

```bash
git clone https://github.com/datastone-inc/pg_num2int_direct_comp.git
cd pg_num2int_direct_comp
make
```

### Development Workflow

```bash
# Edit source files
vim pg_num2int_direct_comp.c

# Rebuild
make clean && make

# Run tests
make installcheck

# Install locally
make install
```

## Code Style

### C Code Standards

- **Style**: K&R with 2-space indentation
- **Standard**: C99 minimum
- **Naming**: camelCase for functions/variables
- **Line length**: 100 characters recommended

Example:
```c
static int
compareValues(Datum left, Datum right) {
  if (left < right) {
    return -1;
  } else if (left > right) {
    return 1;
  }
  return 0;
}
```

### SQL Code Standards

- **Keywords**: UPPERCASE
- **Identifiers**: lowercase
- **Indentation**: 2 spaces

Example:
```sql
CREATE OR REPLACE FUNCTION numeric_eq_int4(numeric, int4)
RETURNS boolean
AS 'MODULE_PATHNAME', 'numeric_eq_int4'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;
```

### Documentation Standards

All functions require Doxygen comments:

```c
/**
 * @brief Compare numeric value with int4 value for exact equality
 * @param fcinfo Function call context containing arguments
 * @return Boolean datum indicating exact equality
 */
PG_FUNCTION_INFO_V1(numeric_eq_int4);
Datum
numeric_eq_int4(PG_FUNCTION_ARGS) {
  // Implementation
}
```

All files require copyright notice:

```c
/*
 * Copyright (c) 2026 dataStone Inc.
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
 */
```

## Testing

### Test Structure

Tests use PostgreSQL's pg_regress framework:

```
sql/               # Test input SQL files
expected/          # Expected output files
```

### Running Tests

```bash
# All tests
make installcheck

# Specific test
make installcheck REGRESS=numeric_int_ops

# Verbose output
make installcheck REGRESS=numeric_int_ops EXTRA_REGRESS_OPTS="--verbose"
```

### Test Categories

1. **numeric_int_ops.sql** - Numeric × integer comparisons
2. **float_int_ops.sql** - Float × integer comparisons
3. **index_usage.sql** - Index optimization verification
4. **hash_joins.sql** - Hash join functionality
5. **merge_joins.sql** - Merge join functionality
6. **null_handling.sql** - NULL semantics
7. **special_values.sql** - NaN, Infinity handling
8. **edge_cases.sql** - Boundary values, overflow
9. **transitivity.sql** - Non-transitivity verification
10. **performance.sql** - Performance benchmarks

### Writing Tests

Follow test-first development:

1. **Write test** in `sql/new_feature.sql`
2. **Create expected output** in `expected/new_feature.out`
3. **Verify test fails** (operators not yet implemented)
4. **Implement feature**
5. **Verify test passes**

Example test:

```sql
-- sql/new_feature.sql
-- Test new comparison behavior
SELECT 10::int4 = 10.0::numeric;  -- Should return true
SELECT 10::int4 = 10.5::numeric;  -- Should return false
```

### Debugging Failed Tests

```bash
# View diff between expected and actual output
make installcheck
cat regression.diffs

# Run single test with verbose output
psql -d contrib_regression -f sql/test_name.sql > actual_output.txt
diff expected/test_name.out actual_output.txt
```

## Architecture

### Project Structure

```
pg-num2int-direct-comp/
├── pg_num2int_direct_comp.c       # Core C implementation
│                                  #   - Comparison functions (54 operators)
│                                  #   - Support function (num2int_support)
│                                  #   - OID cache and syscache callback
├── pg_num2int_direct_comp.h       # Header with type definitions
│                                  #   - OperatorOidCache structure
│                                  #   - Function declarations
├── pg_num2int_direct_comp--1.0.0.sql  # Extension SQL definitions
│                                  #   - Function registrations
│                                  #   - Operator definitions (108 total)
│                                  #   - Operator family memberships
│                                  #   - Hash wrapper functions
│                                  #   - Event trigger for DROP cleanup
├── pg_num2int_direct_comp.control # Extension metadata (name, version, etc.)
├── Makefile                       # PGXS build configuration
├── README.md                      # Project overview and quick start
├── LICENSE                        # MIT license
├── CHANGELOG.md                   # Version history
├── PERFORMANCE.md                 # Benchmark results
├── CODE_REVIEW.md                 # Code review checklist
│
├── sql/                           # Regression test SQL files
│   ├── numeric_int_ops.sql        #   - Numeric × integer tests
│   ├── float_int_ops.sql          #   - Float × integer tests
│   ├── index_usage.sql            #   - Index scan verification
│   ├── hash_joins.sql             #   - Hash join tests
│   ├── merge_joins.sql            #   - Merge join tests
│   ├── null_handling.sql          #   - NULL semantics
│   ├── special_values.sql         #   - NaN, Infinity handling
│   ├── edge_cases.sql             #   - Boundary values, overflow
│   ├── transitivity.sql           #   - Transitivity verification
│   ├── selectivity.sql            #   - Selectivity estimation
│   ├── range_boundary.sql         #   - Range predicate transformation
│   ├── index_nested_loop.sql      #   - Indexed nested loop joins
│   ├── extension_lifecycle.sql    #   - DROP/CREATE extension cycles
│   ├── doc_examples.sql           #   - Documentation examples
│   └── performance.sql            #   - Performance benchmarks
│
├── expected/                      # Expected test output (*.out files)
│
├── results/                       # Actual test output (generated)
│
├── doc/                           # Documentation
│   ├── installation.md            #   - Setup and prerequisites
│   ├── operator-reference.md      #   - Operator reference and usage guide
│   └── development.md             #   - Contributing guide (this file)
│
├── specs/                         # Design specifications
│   └── 001-num-int-direct-comp/
│       ├── spec.md                #   - Feature specification
│       ├── research.md            #   - Research and design decisions
│       ├── plan.md                #   - Implementation plan
│       ├── tasks.md               #   - Task breakdown
│       └── implementation-notes.md #  - Implementation journal
│
└── assets/                        # Images and logos
```

### Key Components

1. **OperatorOidCache**: Per-backend cache of operator OIDs
2. **init_oid_cache()**: Lazy initialization of OID cache
3. **num2int_support()**: Generic support function for index optimization
4. **_cmp_internal()**: Core comparison functions (9 total, one per type combination)
5. **Operator wrappers**: Thin wrappers around _cmp_internal functions (54 total)

### Adding New Operators

1. **Add C function** in `pg_num2int_direct_comp.c`
2. **Declare in header** in `pg_num2int_direct_comp.h`
3. **Register function** in `pg_num2int_direct_comp--1.0.0.sql`
4. **Register operator** in `pg_num2int_direct_comp--1.0.0.sql`
5. **Add to OID cache** in `init_oid_cache()`
6. **Add support logic** in `num2int_support()` if index-optimized
7. **Write tests** in appropriate `sql/*.sql` file
8. **Update documentation** in `doc/operator-reference.md`

## Performance Testing

### Benchmarking

```sql
-- Create large test table
CREATE TABLE perf_test (id SERIAL, value INT4);
INSERT INTO perf_test (value) SELECT generate_series(1, 1000000);
CREATE INDEX ON perf_test(value);
ANALYZE perf_test;

-- Test query performance
EXPLAIN (ANALYZE, BUFFERS) 
SELECT * FROM perf_test WHERE value = 500000::numeric;
```

### Performance Goals

- Index scans: <1ms overhead vs native
- Table scans: <10% overhead vs native
- Selective queries: Sub-millisecond on 1M+ rows

## Debugging

### Enable Debug Output

```c
// Add to pg_num2int_direct_comp.c
#define DEBUG_OUTPUT 1

#if DEBUG_OUTPUT
  elog(NOTICE, "Comparing value: %d", DatumGetInt32(arg));
#endif
```

### GDB Debugging

```bash
# Build with debug symbols
make CFLAGS="-g -O0"

# Attach to PostgreSQL backend
gdb -p $(pgrep -f "postgres.*contrib_regression")

# Set breakpoint
break numeric_eq_int4
continue

# Inspect variables
print arg1
print arg2
```

### Logging

Use PostgreSQL's elog for diagnostic output:

```c
elog(DEBUG1, "Debug message");    // Debug level
elog(INFO, "Info message");       // Informational
elog(NOTICE, "Notice message");   // User notice
elog(WARNING, "Warning message"); // Warning
elog(ERROR, "Error message");     // Error (rolls back transaction)
```

## Continuous Integration

### GitHub Actions

Create `.github/workflows/ci.yml`:

```yaml
name: CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        pg_version: [12, 13, 14, 15, 16]
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Install PostgreSQL
        run: |
          sudo apt-get update
          sudo apt-get install -y postgresql-${{ matrix.pg_version }} \
            postgresql-server-dev-${{ matrix.pg_version }}
      
      - name: Build
        run: make PG_CONFIG=/usr/lib/postgresql/${{ matrix.pg_version }}/bin/pg_config
      
      - name: Test
        run: make installcheck PG_CONFIG=/usr/lib/postgresql/${{ matrix.pg_version }}/bin/pg_config
```

## Release Process

### Version Bumping

1. Update version in `pg_num2int_direct_comp.control`
2. Create new SQL file: `pg_num2int_direct_comp--1.0.0--1.1.0.sql`
3. Update `CHANGELOG.md`
4. Update `README.md` if needed

### Creating Release

```bash
# Tag release
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin v1.0.0

# Create tarball
make dist
```

### PGXN Publishing

1. Create `META.json` for PGXN
2. Test with `pgxn load`
3. Publish: `pgxn publish pg_num2int_direct_comp-1.0.0.zip`

## Contribution Guidelines

### Pull Request Process

1. **Fork** repository
2. **Create feature branch**: `git checkout -b feature/new-operator`
3. **Make changes** following code style
4. **Add tests** for new functionality
5. **Ensure tests pass**: `make installcheck`
6. **Update documentation**
7. **Submit pull request**

### Commit Messages

Follow conventional commits:

```
feat: add support for decimal type alias
fix: correct overflow handling in float4_cmp_int8
docs: update API reference for serial types
test: add edge case tests for NaN handling
```

### Code Review

All changes require:
- Passing CI tests
- Code style compliance
- Documentation updates
- Test coverage

## Support

- **Issues**: GitHub issue tracker
- **Discussions**: GitHub discussions
- **Email**: maintainer contact

## License

MIT License - see LICENSE file for details.
