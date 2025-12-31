# Performance Benchmark Results

**Test Date**: 2024-12-31  
**PostgreSQL Version**: 17.x  
**Table Size**: 1,000,000 rows per table  
**Hardware**: macOS (Apple Silicon)

## Overview

This extension provides direct comparison operators between integer types (`int2`, `int4`, `int8`) and numeric/float types (`numeric`, `float4`, `float8`). The key performance benefits are:

1. **Index Usage**: Cross-type predicates can use btree indexes on integer columns
2. **Constant Transformation**: Fractional bounds are tightened (e.g., `val > 10.5` → `val >= 11`)
3. **Join Strategies**: Hash and Merge joins work across types without casting

## Benchmark Summary

Run `benchmark_performance.sql` to reproduce. Full output saved to `benchmark_performance.out`.

### Test Results

| Test | Scenario | Extension | Stock PostgreSQL | Speedup |
|------|----------|-----------|------------------|---------|
| **2: Key Lookup** | `id = 500000::numeric` | 0.014 ms (Index Scan) | 18 ms (Seq Scan) | **~1,300x** |
| **3: Range Scan** | `val BETWEEN 100::numeric AND 200::numeric` | 0.8 ms (Bitmap Index) | 34 ms (Seq Scan) | **~42x** |
| **6: Nested Loop Join** | 1000-row probe, 1M-row lookup | 2.4 ms (Indexed NL) | 50,475 ms (Cartesian) | **~21,000x** |

### Constant Transformation (Test 1)

The extension transforms fractional constants to integer bounds at plan time:

| Expression | Transformed To | Result |
|------------|----------------|--------|
| `val > 10.5::numeric` | `val >= 11` | Uses index, correct semantics |
| `val < 10.5::numeric` | `val <= 10` | Uses index, correct semantics |
| `val = 10.5::numeric` | `false` | One-Time Filter, 0.003 ms |
| `val = 100.0::numeric` | `val = 100` | Index Cond, 0.2 ms |

### Join Strategies (Tests 4-8)

| Join Type | Extension | Stock (with cast) |
|-----------|-----------|-------------------|
| **Hash Join** | ✅ Enabled cross-type | ✅ Works via cast |
| **Merge Join** | ✅ Enabled for int×numeric | ✅ Works via cast |
| **Indexed Nested Loop** | ✅ Uses index on inner | ❌ Full scan (21,000x slower) |

## Why Stock PostgreSQL is Slower

Without this extension, cross-type comparisons require casting the indexed column:

```sql
-- Stock: must cast column, defeating index
WHERE id::numeric = 500000::numeric  -- Seq Scan

-- Extension: direct comparison, uses index
WHERE id = 500000::numeric           -- Index Scan
```

The cast on the indexed column (`id::numeric`) prevents the optimizer from using the btree index, forcing a sequential scan.

## Running the Benchmark

```bash
# Setup (once)
createdb pg_num2int_test
psql -d pg_num2int_test -c "CREATE EXTENSION pg_num2int_direct_comp;"

# Run benchmark
psql -d pg_num2int_test -f benchmark_performance.sql > benchmark_performance.out 2>&1
```

## Conclusion

The extension delivers significant performance improvements for cross-type comparisons:

- **Index lookups**: 1,000x+ faster for point queries
- **Range scans**: 40x+ faster for range predicates  
- **Nested loop joins**: 20,000x+ faster when index can be leveraged
- **Correct semantics**: Fractional constants properly transformed
