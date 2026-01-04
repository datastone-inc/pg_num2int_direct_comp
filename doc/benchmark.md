# Benchmark Guide

**Test Date**: 2026-01-02  
**PostgreSQL Version**: 17.x  
**Table Size**: 1,000,000 rows per table  
**Hardware**: macOS (Apple Silicon)

## Overview

This extension provides direct comparison operators between integer types (`int2`, `int4`, `int8`) and numeric/float types (`numeric`, `float4`, `float8`). The key performance benefits are:

1. **Index Usage**: Cross-type predicates can use btree indexes on integer columns
2. **Constant Transformation**: Fractional bounds are tightened (e.g., `val > 10.5` → `val >= 11`)
3. **Join Strategies**: Hash, Merge, and Indexed Nested Loop joins work across types without casting

## Running the Benchmark

The comprehensive benchmark creates 3 tables with 1M rows each and tests all join strategies.

### Quick Run (Manual)

```bash
# Run directly with psql (extension must be installed)
psql -d your_database -f sql/benchmark.sql
```

### Via pg_regress

```bash
# Run as a regression test (compares against expected output)
make installcheck REGRESS=benchmark
```

**Note**: The benchmark will show timing differences between runs, so pg_regress will report "failures" for timing variations. This is expected—review the diffs to verify plan shapes are stable.

### Quick Performance Tests

For fast CI/testing, use the lightweight performance test instead:

```bash
make installcheck REGRESS=performance
```

This runs in seconds with 100K rows instead of ~70 seconds with 1M rows.

## Benchmark Summary

### Test Results

| Test | Scenario | Extension | Stock PostgreSQL | Speedup |
|------|----------|-----------|------------------|---------|
| **Key Lookup** | `id = 500000::numeric` | 0.025 ms (Index Scan) | 17.3 ms (Seq Scan) | **~690x** |
| **Range Scan** | `val BETWEEN 100::numeric AND 200::numeric` | 0.8 ms (Bitmap Index) | 23.4 ms (Seq Scan) | **~29x** |
| **Nested Loop** | 1000-row probe, 1M-row lookup | 1.7 ms (Indexed NL) | 49,386 ms (Cartesian) | **~29,000x** |
| **Hash Join** | 1M × 1M rows | 304 ms | 376 ms | **~20% faster** |
| **Merge Join** | 1M × 1M rows (optimal plan) | 210 ms | 274 ms | **~23% faster** |

### Constant Transformation (Test 1)

The extension transforms fractional constants to integer bounds at plan time:

| Expression | Transformed To | Result |
|------------|----------------|--------|
| `val > 10.5::numeric` | `val >= 11` | Uses index, correct semantics |
| `val < 10.5::numeric` | `val <= 10` | Uses index, correct semantics |
| `val = 10.5::numeric` | `false` | One-Time Filter, 0.003 ms |
| `val = 100.0::numeric` | `val = 100` | Index Cond, 0.2 ms |

### Join Strategies

| Join Type | Extension | Stock (with cast) | Notes |
|-----------|-----------|-------------------|-------|
| **Hash Join** | ✅ 304 ms | ✅ 376 ms | 20% faster with extension |
| **Merge Join** | ✅ 210 ms | ✅ 274 ms | 23% faster (see note below) |
| **Indexed Nested Loop** | ✅ 1.7 ms | ❌ 49,386 ms | **29,000x faster** |

## Merge Join Analysis

### Planner Cost Model Limitation

When comparing extension vs stock merge joins, the PostgreSQL planner may choose different plan shapes due to a cost model limitation:

**The issue**: PostgreSQL's sort cost model uses a single `cpu_operator_cost` (0.0025) for all comparison functions. It doesn't know that:
- `btint4cmp` (sorting int4): ~1 CPU cycle per comparison
- `numeric_cmp` (sorting numeric): ~100+ cycles per comparison (function call, digit extraction, multi-word arithmetic)

This causes the planner to undervalue the benefit of sorting smaller int4 values vs larger numeric values.

**Default plan (extension)**:
- Index Scan `int_table_pkey` + Sort `numeric_table` → 613 ms

**Optimal plan (with `enable_seqscan = off`)**:
- Index Scan `idx_numeric_table_int_ref` + Sort `int_table` → 210 ms

The benchmark uses `SET enable_seqscan = off` for merge join tests to force identical plan shapes, enabling a fair apples-to-apples comparison. This compensates for the planner's inability to model comparison function costs.

### Memory Efficiency

Even when the planner picks a suboptimal plan, the extension's merge join uses significantly less memory:

| Metric | Extension | Stock |
|--------|-----------|-------|
| Sort Memory | 24 MB | 56 MB |

This is because sorting int4 values (4 bytes) requires less working memory than sorting numeric values (variable length, typically 8-16 bytes).

## Why Stock PostgreSQL is Slower

Without this extension, cross-type comparisons require casting the indexed column:

```sql
-- Stock: must cast column, defeating index
WHERE id::numeric = 500000::numeric  -- Seq Scan

-- Extension: direct comparison, uses index
WHERE id = 500000::numeric           -- Index Scan
```

The cast on the indexed column (`id::numeric`) prevents the optimizer from using the btree index, forcing a sequential scan.

## Internal Optimizations (v1.0)

The v1.0 implementation includes several low-level optimizations to minimize per-comparison overhead:

| Optimization | Technique |
|--------------|-----------|
| **Direct Numeric Access** | NUM2INT_NUMERIC_* macros bypass OidFunctionCall for comparison |
| **Stack-Based Hash** | Hash computation uses stack arrays, avoiding palloc |
| **Out-of-Range Constants** | `int2_col = 99999` folds to FALSE at plan time |
| **Boundary Cache** | Pre-computed int2/4/8 min/max as Numeric values |
| **Compact OID Cache** | Array-based lookup vs 108 individual struct fields |

## Stability Techniques

The benchmark uses several techniques to reduce run-to-run variance:

```sql
-- Disable parallel workers to eliminate coordination jitter
SET max_parallel_workers_per_gather = 0;

-- High work_mem forces in-memory quicksort (no disk spill variance)
SET work_mem = '256MB';

-- Warmup queries load tables into buffer cache
SELECT COUNT(*) FROM int_table;
SELECT COUNT(*) FROM numeric_table;
```

The benchmark also runs each join test 3 times—use the median for reporting.

## Conclusion

The extension delivers significant performance improvements for cross-type comparisons:

- **Index lookups**: 690x+ faster for point queries
- **Range scans**: 29x+ faster for range predicates  
- **Nested loop joins**: 29,000x+ faster when index can be leveraged
- **Hash joins**: 20% faster due to efficient cross-type hashing
- **Merge joins**: 23% faster with optimal plan, 56% less memory
- **Correct semantics**: Fractional constants properly transformed
