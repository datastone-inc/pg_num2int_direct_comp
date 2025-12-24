# Changelog

All notable changes to pg_num2int_direct_comp will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - TBD

### Added

- Initial release of pg_num2int_direct_comp extension
- Exact equality operators (=, <>) for numeric/float × integer comparisons (18 operators)
- Exact ordering operators (<, >, <=, >=) for numeric/float × integer comparisons (36 operators)
- Index optimization support via SupportRequestIndexCondition for btree indexes
- Complete type coverage: (numeric, float4, float8) × (int2, int4, int8)
- Type alias support: serial, bigserial, smallserial, decimal
- NULL handling per SQL standard
- IEEE 754 special value handling (NaN, Infinity) for float types
- Comprehensive test suite with 9 test categories via pg_regress
- Documentation: README, installation guide, user guide, API reference
- PostgreSQL version support: 12, 13, 14, 15, 16
- Performance benchmarks showing <10% overhead vs native comparisons

### Features

- **Precision Detection**: Identifies float precision loss (e.g., 16777216::bigint = 16777217::float4 returns false)
- **Index Integration**: Predicates like `WHERE intcol = 10.0::numeric` use indexes efficiently
- **Non-Transitive**: Query optimizer correctly avoids transitive inference across type boundaries
- **Production Ready**: Sub-millisecond performance on million-row tables

### Technical Details

- 9 core comparison functions using `_cmp_internal` pattern
- 54 operator wrapper functions
- 1 generic support function for index optimization
- Lazy per-backend OID cache for operator lookups
- HASHES property for equality operators (enables hash joins)
- MERGES property for ordering operators (enables merge joins)

[1.0.0]: https://github.com/dsharpe/pg-num2int-direct-comp/releases/tag/v1.0.0
