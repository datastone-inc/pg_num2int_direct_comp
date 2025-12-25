# Changelog

All notable changes to pg_num2int_direct_comp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-12-25

### Added

- Initial release of pg_num2int_direct_comp extension
- **72 operators total**: 54 forward + 18 commutator operators for exact comparisons
  - Exact equality operators (=, <>) for numeric/float × integer comparisons (18 operators)
  - Exact ordering operators (<, >, <=, >=) for numeric/float × integer comparisons (36 operators)
  - Commutator operators for int × numeric/float (18 operators)
- **Btree operator family support**: Operators added to `numeric_ops` and `float_ops` btree families
  - Enables indexed nested loop joins with index conditions on both sides
  - 9 btree comparison support functions (numeric_cmp_int2/4/8, float4_cmp_int2/4/8, float8_cmp_int2/4/8)
- **Hash operator family support**: Operators added to `numeric_ops` and `float_ops` hash families
  - Enables hash joins for large table equijoins
  - 18 hash wrapper functions (cast integers to numeric/float before hashing)
  - HASHES property on all equality operators
- Index optimization support via SupportRequestIndexCondition for btree indexes
- Complete type coverage: (numeric, float4, float8) × (int2, int4, int8)
- Type alias support: serial, bigserial, smallserial, decimal
- NULL handling per SQL standard
- IEEE 754 special value handling (NaN, Infinity) for float types
- Comprehensive test suite with 11 test files via pg_regress
- Documentation: README, installation guide, user guide, API reference, research documentation
- PostgreSQL version support: 12, 13, 14, 15, 16, 17
- Performance benchmarks showing <10% overhead vs native comparisons

### Features

- **Precision Detection**: Identifies float precision loss (e.g., 16777216::bigint = 16777217::float4 returns false)
- **Index Integration**: Predicates like `WHERE intcol = 10.0::numeric` use btree indexes via indexed nested loop joins
- **Hash Joins**: Large table equijoins use hash joins when appropriate
- **Query Optimizer Friendly**: Operators in higher-precision families only (not in integer_ops) to avoid invalid transitive inferences
- **Production Ready**: Sub-millisecond performance on million-row tables

### Technical Details

- 9 core comparison functions using `_cmp_internal` pattern
- 72 operator wrapper functions (54 forward + 18 commutator)
- 1 generic support function for index optimization
- 9 btree comparison support functions
- 18 hash wrapper functions (9 base + 9 extended versions)
- Lazy per-backend OID cache for operator lookups
- HASHES property for all 18 equality operators
- Merge joins intentionally not supported (would require integer_ops membership causing transitive inference issues)

### Performance

- Indexed nested loop joins: Sub-millisecond on 1M+ row tables
- Hash joins: Efficient for large table equijoins
- Direct comparisons: <10% overhead vs native integer comparisons
- Index scans: Uses existing btree indexes without rebuild

### Not Implemented

- **Merge joins**: Not possible due to transitive inference constraints
  - Adding operators to `integer_ops` would enable invalid planner inferences
  - Example: Planner could infer `int_col = 10.5` from `int_col = 10` and `10 = 10.5`
  - Indexed nested loop joins provide equivalent or better performance

[1.0.0]: https://github.com/dsharpe/pg-num2int-direct-comp/releases/tag/v1.0.0
