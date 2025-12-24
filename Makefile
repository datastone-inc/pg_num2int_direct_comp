# Makefile for pg_num2int_direct_comp PostgreSQL extension

EXTENSION = pg_num2int_direct_comp
DATA = pg_num2int_direct_comp--1.0.0.sql
MODULES = pg_num2int_direct_comp

# Regression tests (adding incrementally per phase)
# Phase 3 (User Story 1): numeric_int_ops float_int_ops
# Phase 4 (User Story 2): index_usage
# Phase 5 (User Story 3): range_boundary
# Phase 6 (User Story 4): transitivity
REGRESS = numeric_int_ops float_int_ops index_usage range_boundary transitivity

# Build configuration
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# Compiler flags
PG_CPPFLAGS = -Wall -Wextra -Werror
