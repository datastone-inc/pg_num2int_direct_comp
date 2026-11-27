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
# doc_examples: validates SQL examples from README.md and doc/*.md
# extension_lifecycle: tests DROP/CREATE extension cycles with cleanup trigger
#
# Long-running tests (not included by default):
# benchmark: comprehensive benchmark with 1M row tables (~70 seconds)
#            Run manually with: make installcheck REGRESS=benchmark
REGRESS = numeric_int_ops float_int_ops index_usage range_boundary transitivity edge_cases comparison_exactness null_handling special_values index_nested_loop hash_joins merge_joins performance selectivity doc_examples extension_lifecycle

# Compiler flags. PGXS reads PG_CPPFLAGS when it is included, so they are
# set before the include. Callbacks such as the syscache invalidation one
# have signatures PostgreSQL fixes, so unused parameters are not warnings.
PG_CPPFLAGS = -Wall -Wextra -Wno-unused-parameter
# Warnings are errors only on request (make WERROR=1), so that a compiler or
# PostgreSQL version with warnings this code has not met still builds it.
ifdef WERROR
PG_CPPFLAGS += -Werror
endif

# Build configuration
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

# RETURNING OLD/NEW exists from PostgreSQL 18.
ifeq ($(shell test $(MAJORVERSION) -ge 18 && echo yes),yes)
REGRESS += returning_old_new
endif

# Release. META.json carries the extension's version for PGXN, at the top
# and in its provides entry; the control file's default_version must match
# it and name an install script. check-version fails if they disagree.
# dist writes the PGXN archive of the committed tree (HEAD), so uncommitted
# changes are not in it; .gitattributes keeps the agent and design files out.
EXT_VERSION = $(shell sed -n \
    "s/^default_version = '\(.*\)'/\1/p" $(EXTENSION).control)
check-version:
	@test -n "$(EXT_VERSION)" || \
	    { echo "no default_version in $(EXTENSION).control"; exit 1; }
	@grep -q '^   "version": "$(EXT_VERSION)",$$' META.json || \
	    { echo "META.json is not version $(EXT_VERSION)"; exit 1; }
	@grep -q '^         "version": "$(EXT_VERSION)"$$' META.json || \
	    { echo "META.json provides another version than $(EXT_VERSION)"; \
	    exit 1; }
	@test -f "$(EXTENSION)--$(EXT_VERSION).sql" || \
	    { echo "no install script for version $(EXT_VERSION)"; exit 1; }
	@echo "version $(EXT_VERSION): consistent"
dist: check-version
	git archive --format zip --prefix=$(EXTENSION)-$(EXT_VERSION)/ \
	    -o $(EXTENSION)-$(EXT_VERSION).zip HEAD
.PHONY: check-version dist
