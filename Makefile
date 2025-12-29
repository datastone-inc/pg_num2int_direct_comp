# Makefile for pg_num2int_direct_comp PostgreSQL extension

EXTENSION = pg_num2int_direct_comp
DATA = pg_num2int_direct_comp--1.0.0.sql
MODULES = pg_num2int_direct_comp

# Regression tests (adding incrementally per phase)
# Phase 3 (User Story 1): numeric_int_ops float_int_ops
# Phase 4 (User Story 2): index_usage
# Phase 5 (User Story 3): range_boundary
# Phase 6 (User Story 4): transitivity
# Phase 7 (User Story 5): edge_cases null_handling special_values
# Phase 8: index_nested_loop (indexed nested loop optimization - works in v1.0)
# Phase 9: hash_joins (hash join optimization)
# Phase 10: merge_joins (documents why merge joins are not supported in v1.0)
# Phase 11: performance
# Phase 12: selectivity (constant predicate optimization FR-015/016/017)
REGRESS = numeric_int_ops float_int_ops index_usage range_boundary transitivity edge_cases null_handling special_values index_nested_loop hash_joins merge_joins performance selectivity

# Build configuration
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Compiler flags
PG_CPPFLAGS = -Wall -Wextra -Werror
