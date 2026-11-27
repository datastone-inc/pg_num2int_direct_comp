# Changelog

All notable changes to pg_num2int_direct_comp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-11-27

First tagged release, and the first on PGXN. A pre-release snapshot of the
code was described here as 1.0.0 in January 2026 but never tagged; this
entry replaces that description. If you built that snapshot from git, drop
and recreate the extension: there is no upgrade script from it, and it had
the wrong-result bugs listed under "Fixed in this release" below.

### Added

- **108 operators**: `=`, `<>`, `<`, `<=`, `>`, `>=` for (numeric, float4, float8) × (int2, int4, int8), plus their commutators. Every comparison is exact: no value is rounded before it is compared.
- **Btree operator families**: the operators join `integer_ops`, `numeric_ops` and `float_ops`, with 9 cross-type comparison support functions, so indexes, indexed nested loop joins and merge joins apply to both int × numeric and int × float. See [Float × Integer Btree Ops research](specs/002-float-btree-ops/research.md).
- **Hash operator families**: the equality operators are `HASHES` and join the `numeric_ops` and `float_ops` hash families (integers are hashed as numeric or float8), so hash joins apply.
- **Constant predicate rewriting** via `SupportRequestSimplify`:
  - Impossible predicates: `int_col = 10.5::numeric` → `FALSE`
  - Exact matches: `int_col = 100::numeric` → `int_col = 100` (native operator)
  - Range boundaries: `int_col > 10.5` → `int_col >= 11`
  - Out-of-range constants: `int2_col = 99999` → `FALSE` at plan time
  - The constant may be on either side, and NULL semantics are kept on nullable columns.
- **`DROP EXTENSION` cleanup**: an event trigger removes the extension's entries from the built-in operator families, which `pg_depend` cannot track, so the extension can be reinstalled cleanly.
- Type aliases (`serial`, `bigserial`, `smallserial`, `decimal`), SQL NULL handling, and IEEE 754 special values (NaN, Infinity).
- **PostgreSQL 16, 17, 18 and 19.** 16 is the minimum: the NULL handling uses `Var.varnullingrels`, which 16 added, and building against an older server stops with an `#error`. PostgreSQL 19 has alternate expected outputs for its `Replaces:` EXPLAIN line.
- **Regression suite** (pg_regress), including `comparison_exactness`, which checks every int × numeric/float comparison against an exact reference, constant on either side, on NOT NULL and nullable columns, plus hash/equality consistency for every hashable operator. `returning_old_new` runs only on PostgreSQL 18 and later.
- **PGXN packaging**: `META.json`, `make dist` (writes the release zip from the committed tree) and `make check-version` (checks `META.json` against the control file). `.gitattributes` keeps the agent and editor files out of the archive.
- **CI**: GitHub Actions builds with warnings as errors and runs the regression tests on PostgreSQL 16 to 19, on x86_64 and arm64.
- `-Werror` is opt-in (`make WERROR=1`), so a compiler with warnings this code has not met still builds it.
- Documentation: README, `doc/installation.md`, `doc/operator-reference.md`, `doc/benchmark.md`, `doc/development.md`, and research notes under `specs/`.

### Performance

- Numeric values are compared by reading their digit arrays directly (`numeric_cmp_int64_direct()`), without `OidFunctionCall` overhead.
- Hash functions compute on stack arrays, with no palloc/pfree.
- Integer type bounds are cached as Numeric values; operator OIDs are kept in a lazy per-backend array cache, checked against the catalog by tests.

Benchmark (2026-01-02):

| Operation | Extension | Stock PostgreSQL | Speedup |
|-----------|-----------|------------------|---------|
| Point lookup | 0.025 ms (Index) | 17.3 ms (Seq Scan) | **~690x** |
| Range scan | 0.8 ms (Bitmap) | 23.4 ms (Seq Scan) | **~29x** |
| Nested Loop Join | 3 ms | 49,658 ms | **~16,500x** |

### Fixed in this release (relative to the January 2026 snapshot)

- **Wrong results with the constant on the left of a range comparison**: `const OP column` was rewritten as if it were `column OP const`, so `<`, `<=`, `>` and `>=` returned the complement of the right answer (for example `0.5 > col` matched no rows for `col = 0`). `=` and `<>` were not affected.
- **Wrong int × float results at the integer type limits**: out-of-range floats were converted back to integers (undefined behaviour in C), and `(float) INT_MAX` was used as a range bound, which rounds up to 2^31 (float4) or 2^63 (float8). `2147483647::int4 = 2147483648::float4` returned true, and hash joins on such values used mismatched hashes.
- **Missing `MERGES`** on int2 × float4, int2 × float8 and int4 × float8 `=`, so the planner never chose a merge join for them.
- **NULL semantics** were dropped by some support-function rewrites.
- **Build on PostgreSQL 16 and 17**: `Var.varreturningtype`, which exists only from 18, is now read only there.
- **`DROP EXTENSION` cleanup** ran for any extension whose name started with ours, and ignored all errors; it now matches the whole name and ignores only `undefined_object`.

### Removed (relative to the January 2026 snapshot)

- The `SupportRequestIndexCondition` callback, superseded by `SupportRequestSimplify`.
- `doc/api-reference.md` and `doc/user-guide.md`, merged into `doc/operator-reference.md`.

[Unreleased]: https://github.com/datastone-inc/pg_num2int_direct_comp/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/datastone-inc/pg_num2int_direct_comp/releases/tag/v1.0.0
