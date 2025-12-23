# Tasks: Direct Exact Comparison for Numeric and Integer Types

**Feature**: pg_num2int_direct_comp  
**Branch**: 001-num-int-direct-comp  
**Input**: Design documents from `/specs/001-num-int-direct-comp/`

**Organization**: Tasks organized by user story for independent implementation and testing

**Tests**: Test-first development is MANDATORY per constitution. Each phase includes test tasks.

**Test-First Workflow**: 
1. Write test SQL with expected results (test task, e.g., T019-T023)
2. Verify test fails with "ERROR: operator does not exist" or "ERROR: function does not exist" (pre-implementation validation)
3. Implement operators/functions (implementation tasks, e.g., T024-T050)
4. Verify test passes with expected results (verification task, e.g., T054)

---

## Format: `- [ ] [ID] [P?] [Story?] Description with file path`

- **[P]**: Can run in parallel (different files, no blocking dependencies)
- **[Story]**: User story label (US1, US2, US3, US4, US5) - only for story-specific tasks
- All file paths are absolute from repository root

---

## Phase 1: Setup (Project Initialization)

**Purpose**: Create project structure and build system

- [ ] T001 Create LICENSE file with MIT License text at repository root
- [ ] T002 Create README.md with installation instructions and quick start examples
- [ ] T003 Create CHANGELOG.md with version 1.0.0 initial release entry
- [ ] T004 Create Makefile with PGXS build configuration at repository root
- [ ] T005 Create pg_num2int_direct_comp.control extension control file
- [ ] T006 [P] Create doc/installation.md with setup and build instructions
- [ ] T007 [P] Create doc/user-guide.md with usage examples and patterns
- [ ] T008 [P] Create doc/api-reference.md with complete operator reference
- [ ] T009 [P] Create doc/development.md with contributor guide

**Checkpoint**: Project structure ready, build system configured

---

## Phase 2: Foundational (Core Infrastructure - BLOCKS All User Stories)

**Purpose**: Shared infrastructure that ALL user stories depend on

**⚠️ CRITICAL**: No user story implementation can begin until this phase is complete

- [ ] T010 Create pg_num2int_direct_comp.h header with copyright notice, doxygen header, and structure declarations
- [ ] T011 Create pg_num2int_direct_comp.c with copyright notice, doxygen header, and includes
- [ ] T012 Implement OperatorOidCache structure definition in pg_num2int_direct_comp.h
- [ ] T013 Implement init_oid_cache() function with lazy initialization in pg_num2int_direct_comp.c
- [ ] T014 Implement num2int_support() generic support function skeleton in pg_num2int_direct_comp.c
- [ ] T015 Create pg_num2int_direct_comp--1.0.0.sql with copyright notice and psql guard
- [ ] T016 Register num2int_support() function in pg_num2int_direct_comp--1.0.0.sql
- [ ] T017 Create sql/ directory for pg_regress test files
- [ ] T018 Create expected/ directory for pg_regress expected outputs

**Checkpoint**: Foundation complete - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Exact Equality Comparison (Priority: P1) 🎯 MVP

**Goal**: Implement exact equality (=) and inequality (<>) operators for all 9 type combinations, detecting precision mismatches

**Independent Test**: Create tables with precision boundary values (e.g., 16777216 vs 16777217 float4), verify exact comparison returns false while PostgreSQL default returns true

### Tests for User Story 1

> **TEST-FIRST**: Write these tests FIRST, verify they FAIL, then implement

- [ ] T019 [P] [US1] Create sql/numeric_int_ops.sql with equality tests for numeric × int combinations
- [ ] T020 [P] [US1] Create expected/numeric_int_ops.out with expected equality test results
- [ ] T021 [P] [US1] Create sql/float_int_ops.sql with equality tests for float × int combinations (including precision boundaries)
- [ ] T022 [P] [US1] Create expected/float_int_ops.out with expected float equality test results
- [ ] T023 [P] [US1] Add numeric_int_ops and float_int_ops to REGRESS variable in Makefile

### Implementation for User Story 1

**Core Comparison Functions (9 total - one per type combination)**:

- [ ] T024 [P] [US1] Implement numeric_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T025 [P] [US1] Implement numeric_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T026 [P] [US1] Implement numeric_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T027 [P] [US1] Implement float4_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T028 [P] [US1] Implement float4_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T029 [P] [US1] Implement float4_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T030 [P] [US1] Implement float8_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T031 [P] [US1] Implement float8_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c
- [ ] T032 [P] [US1] Implement float8_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c

**Equality Operator Wrappers (18 total - 9 for =, 9 for <>)**:

- [ ] T033 [P] [US1] Implement numeric_eq_int2() wrapper calling numeric_cmp_int2_internal() == 0
- [ ] T034 [P] [US1] Implement numeric_eq_int4() wrapper calling numeric_cmp_int4_internal() == 0
- [ ] T035 [P] [US1] Implement numeric_eq_int8() wrapper calling numeric_cmp_int8_internal() == 0
- [ ] T036 [P] [US1] Implement float4_eq_int2() wrapper calling float4_cmp_int2_internal() == 0
- [ ] T037 [P] [US1] Implement float4_eq_int4() wrapper calling float4_cmp_int4_internal() == 0
- [ ] T038 [P] [US1] Implement float4_eq_int8() wrapper calling float4_cmp_int8_internal() == 0
- [ ] T039 [P] [US1] Implement float8_eq_int2() wrapper calling float8_cmp_int2_internal() == 0
- [ ] T040 [P] [US1] Implement float8_eq_int4() wrapper calling float8_cmp_int4_internal() == 0
- [ ] T041 [P] [US1] Implement float8_eq_int8() wrapper calling float8_cmp_int8_internal() == 0
- [ ] T042 [P] [US1] Implement numeric_ne_int2() wrapper calling numeric_cmp_int2_internal() != 0
- [ ] T043 [P] [US1] Implement numeric_ne_int4() wrapper calling numeric_cmp_int4_internal() != 0
- [ ] T044 [P] [US1] Implement numeric_ne_int8() wrapper calling numeric_cmp_int8_internal() != 0
- [ ] T045 [P] [US1] Implement float4_ne_int2() wrapper calling float4_cmp_int2_internal() != 0
- [ ] T046 [P] [US1] Implement float4_ne_int4() wrapper calling float4_cmp_int4_internal() != 0
- [ ] T047 [P] [US1] Implement float4_ne_int8() wrapper calling float4_cmp_int8_internal() != 0
- [ ] T048 [P] [US1] Implement float8_ne_int2() wrapper calling float8_cmp_int2_internal() != 0
- [ ] T049 [P] [US1] Implement float8_ne_int4() wrapper calling float8_cmp_int4_internal() != 0
- [ ] T050 [P] [US1] Implement float8_ne_int8() wrapper calling float8_cmp_int8_internal() != 0

**SQL Registration for Equality Operators (18 operators)**:

- [ ] T051 [US1] Register all 18 equality/inequality functions in pg_num2int_direct_comp--1.0.0.sql with SUPPORT property
- [ ] T052 [US1] Register all 18 = and <> operators in pg_num2int_direct_comp--1.0.0.sql with HASHES, COMMUTATOR, NEGATOR properties

**Verification**:

- [ ] T053 [US1] Run `make clean && make` to build extension
- [ ] T054 [US1] Run `make installcheck` to verify tests pass for equality operators
- [ ] T055 [US1] Verify precision boundary test (16777216::bigint = 16777217::float4 returns false)

**Checkpoint**: User Story 1 complete - exact equality comparisons work for all 9 type combinations

---

## Phase 4: User Story 2 - Index-Optimized Query Execution (Priority: P1)

**Goal**: Implement SupportRequestIndexCondition in num2int_support() to enable btree index usage for exact comparison predicates

**Independent Test**: Create indexed table, run EXPLAIN on queries with exact comparisons, verify Index Scan appears in plan

### Tests for User Story 2

> **TEST-FIRST**: Write these tests FIRST, verify they FAIL (show Seq Scan), then implement

- [ ] T056 [US2] Create sql/index_usage.sql with EXPLAIN tests for Index Scan verification
- [ ] T057 [US2] Create expected/index_usage.out with expected EXPLAIN plans showing Index Scan
- [ ] T058 [US2] Add index_usage to REGRESS variable in Makefile

### Implementation for User Story 2

- [ ] T059 [US2] Implement OID cache population for all 18 equality/inequality operators in init_oid_cache()
- [ ] T060 [US2] Implement SupportRequestIndexCondition handler in num2int_support() for equality operators
- [ ] T061 [US2] Add operator OID inspection logic to identify which type combination is being used
- [ ] T062 [US2] Add constant detection and range checking logic for numeric/float constants
- [ ] T063 [US2] Add index predicate transformation logic to rewrite predicate for btree index usage
- [ ] T064 [US2] Update function registration in SQL to include SUPPORT num2int_support for all equality functions

**Verification**:

- [ ] T065 [US2] Run `make clean && make` to rebuild extension
- [ ] T066 [US2] Run `make installcheck` to verify Index Scan appears in EXPLAIN output
- [ ] T067 [US2] Verify sub-millisecond execution time on 1M row table with indexed int column

**Checkpoint**: User Story 2 complete - index optimization works for exact equality predicates

---

## Phase 5: User Story 3 - Range Comparisons (Priority: P2)

**Goal**: Implement ordering operators (<, >, <=, >=) for all 9 type combinations with exact semantics

**Independent Test**: Execute range queries with boundary values (e.g., `intcol > 10.5`), verify correct boundary handling

### Tests for User Story 3

> **TEST-FIRST**: Write tests FIRST, verify they FAIL, then implement

- [ ] T068 [P] [US3] Add range operator tests to sql/numeric_int_ops.sql for numeric × int combinations
- [ ] T069 [P] [US3] Update expected/numeric_int_ops.out with range operator expected results
- [ ] T070 [P] [US3] Add range operator tests to sql/float_int_ops.sql for float × int combinations
- [ ] T071 [P] [US3] Update expected/float_int_ops.out with range operator expected results

### Implementation for User Story 3

**Range Operator Wrappers (36 total - 4 operators × 9 type combinations)**:

- [ ] T072 [P] [US3] Implement numeric_lt_int2() wrapper calling numeric_cmp_int2_internal() < 0
- [ ] T073 [P] [US3] Implement numeric_lt_int4() wrapper calling numeric_cmp_int4_internal() < 0
- [ ] T074 [P] [US3] Implement numeric_lt_int8() wrapper calling numeric_cmp_int8_internal() < 0
- [ ] T075 [P] [US3] Implement float4_lt_int2() wrapper calling float4_cmp_int2_internal() < 0
- [ ] T076 [P] [US3] Implement float4_lt_int4() wrapper calling float4_cmp_int4_internal() < 0
- [ ] T077 [P] [US3] Implement float4_lt_int8() wrapper calling float4_cmp_int8_internal() < 0
- [ ] T078 [P] [US3] Implement float8_lt_int2() wrapper calling float8_cmp_int2_internal() < 0
- [ ] T079 [P] [US3] Implement float8_lt_int4() wrapper calling float8_cmp_int4_internal() < 0
- [ ] T080 [P] [US3] Implement float8_lt_int8() wrapper calling float8_cmp_int8_internal() < 0
- [ ] T081 [P] [US3] Implement numeric_gt_int2() wrapper calling numeric_cmp_int2_internal() > 0
- [ ] T082 [P] [US3] Implement numeric_gt_int4() wrapper calling numeric_cmp_int4_internal() > 0
- [ ] T083 [P] [US3] Implement numeric_gt_int8() wrapper calling numeric_cmp_int8_internal() > 0
- [ ] T084 [P] [US3] Implement float4_gt_int2() wrapper calling float4_cmp_int2_internal() > 0
- [ ] T085 [P] [US3] Implement float4_gt_int4() wrapper calling float4_cmp_int4_internal() > 0
- [ ] T086 [P] [US3] Implement float4_gt_int8() wrapper calling float4_cmp_int8_internal() > 0
- [ ] T087 [P] [US3] Implement float8_gt_int2() wrapper calling float8_cmp_int2_internal() > 0
- [ ] T088 [P] [US3] Implement float8_gt_int4() wrapper calling float8_cmp_int4_internal() > 0
- [ ] T089 [P] [US3] Implement float8_gt_int8() wrapper calling float8_cmp_int8_internal() > 0
- [ ] T090 [P] [US3] Implement numeric_le_int2() wrapper calling numeric_cmp_int2_internal() <= 0
- [ ] T091 [P] [US3] Implement numeric_le_int4() wrapper calling numeric_cmp_int4_internal() <= 0
- [ ] T092 [P] [US3] Implement numeric_le_int8() wrapper calling numeric_cmp_int8_internal() <= 0
- [ ] T093 [P] [US3] Implement float4_le_int2() wrapper calling float4_cmp_int2_internal() <= 0
- [ ] T094 [P] [US3] Implement float4_le_int4() wrapper calling float4_cmp_int4_internal() <= 0
- [ ] T095 [P] [US3] Implement float4_le_int8() wrapper calling float4_cmp_int8_internal() <= 0
- [ ] T096 [P] [US3] Implement float8_le_int2() wrapper calling float8_cmp_int2_internal() <= 0
- [ ] T097 [P] [US3] Implement float8_le_int4() wrapper calling float8_cmp_int4_internal() <= 0
- [ ] T098 [P] [US3] Implement float8_le_int8() wrapper calling float8_cmp_int8_internal() <= 0
- [ ] T099 [P] [US3] Implement numeric_ge_int2() wrapper calling numeric_cmp_int2_internal() >= 0
- [ ] T100 [P] [US3] Implement numeric_ge_int4() wrapper calling numeric_cmp_int4_internal() >= 0
- [ ] T101 [P] [US3] Implement numeric_ge_int8() wrapper calling numeric_cmp_int8_internal() >= 0
- [ ] T102 [P] [US3] Implement float4_ge_int2() wrapper calling float4_cmp_int2_internal() >= 0
- [ ] T103 [P] [US3] Implement float4_ge_int4() wrapper calling float4_cmp_int4_internal() >= 0
- [ ] T104 [P] [US3] Implement float4_ge_int8() wrapper calling float4_cmp_int8_internal() >= 0
- [ ] T105 [P] [US3] Implement float8_ge_int2() wrapper calling float8_cmp_int2_internal() >= 0
- [ ] T106 [P] [US3] Implement float8_ge_int4() wrapper calling float8_cmp_int4_internal() >= 0
- [ ] T107 [P] [US3] Implement float8_ge_int8() wrapper calling float8_cmp_int8_internal() >= 0

**SQL Registration for Range Operators (36 operators)**:

- [ ] T108 [US3] Register all 36 range operator functions in pg_num2int_direct_comp--1.0.0.sql with SUPPORT property
- [ ] T109 [US3] Register all 36 <, >, <=, >= operators in pg_num2int_direct_comp--1.0.0.sql with MERGES, COMMUTATOR, NEGATOR properties

**Index Support for Range Operators**:

- [ ] T110 [US3] Update init_oid_cache() to populate OIDs for all 36 range operators
- [ ] T111 [US3] Extend SupportRequestIndexCondition handler in num2int_support() to handle range operators

**Verification**:

- [ ] T112 [US3] Run `make clean && make` to rebuild extension
- [ ] T113 [US3] Run `make installcheck` to verify range operator tests pass
- [ ] T114 [US3] Verify boundary handling test (intcol > 10.5::float8 returns only [11, 12])

**Checkpoint**: User Story 3 complete - range comparisons work with exact semantics for all 9 type combinations

---

## Phase 6: User Story 4 - Non-Transitive Comparison Semantics (Priority: P2)

**Goal**: Verify query optimizer does not incorrectly apply transitivity across different type comparisons

**Independent Test**: Construct query with chained comparisons, examine EXPLAIN to verify no transitive predicates inferred

### Tests for User Story 4

> **TEST-FIRST**: Write tests FIRST to establish baseline behavior

- [ ] T115 [US4] Create sql/transitivity.sql with chained comparison queries and EXPLAIN VERBOSE output
- [ ] T116 [US4] Create expected/transitivity.out with expected plans showing no transitive inference
- [ ] T117 [US4] Add transitivity to REGRESS variable in Makefile

### Implementation for User Story 4

- [ ] T118 [US4] Review operator registration in pg_num2int_direct_comp--1.0.0.sql to confirm operators are NOT added to btree operator families
- [ ] T119 [US4] Verify COMMUTATOR and NEGATOR properties are correctly specified (these are safe)
- [ ] T120 [US4] Verify operator properties are correct: equality/inequality operators have HASHES only (no MERGES), range operators (<, >, <=, >=) have MERGES only (no HASHES). This property separation prevents transitivity inference.

**Verification**:

- [ ] T121 [US4] Run `make installcheck` to verify transitivity tests pass
- [ ] T122 [US4] Verify EXPLAIN output shows no inferred predicates for chained comparisons
- [ ] T123 [US4] Verify result sets are correct (no extra rows from transitive optimization)

**Checkpoint**: User Story 4 complete - transitivity correctly prevented across type combinations

---

## Phase 7: User Story 5 - Comprehensive Type Coverage (Priority: P3)

**Goal**: Verify all type aliases (serial types, decimal) work correctly, add comprehensive edge case tests

**Independent Test**: Test serial and decimal types in comparisons, verify they behave identically to underlying types

### Tests for User Story 5

> **TEST-FIRST**: Write comprehensive tests for edge cases

- [ ] T124 [P] [US5] Create sql/edge_cases.sql with precision boundary, overflow, serial type, and decimal type tests
- [ ] T125 [P] [US5] Create expected/edge_cases.out with expected edge case test results
- [ ] T126 [P] [US5] Create sql/null_handling.sql with NULL propagation tests for all 54 operators
- [ ] T127 [P] [US5] Create expected/null_handling.out with expected NULL test results
- [ ] T128 [P] [US5] Create sql/special_values.sql with NaN/Infinity tests for float operators
- [ ] T129 [P] [US5] Create expected/special_values.out with expected special value test results
- [ ] T130 [P] [US5] Add edge_cases, null_handling, special_values to REGRESS variable in Makefile

### Implementation for User Story 5

- [ ] T131 [US5] Add IEEE 754 special value handling (NaN, Infinity) to float_cmp_* functions in pg_num2int_direct_comp.c
- [ ] T132 [US5] Add NULL handling verification to all operator wrapper functions
- [ ] T133 [US5] Add overflow detection for numeric values exceeding integer type ranges
- [ ] T134 [US5] Document serial type behavior (aliases work automatically) in doc/user-guide.md
- [ ] T135 [US5] Document decimal type behavior (alias works automatically) in doc/user-guide.md

**Verification**:

- [ ] T136 [US5] Run `make installcheck` to verify all edge case tests pass
- [ ] T137 [US5] Verify serial types work correctly (smallserial, serial, bigserial)
- [ ] T138 [US5] Verify decimal type works correctly (alias for numeric)
- [ ] T139 [US5] Verify NaN/Infinity handling follows IEEE 754 semantics

**Checkpoint**: User Story 5 complete - comprehensive type coverage verified, all edge cases handled

---

## Phase 8: Hash and Merge Join Support

**Goal**: Verify HASHES and MERGES properties enable hash joins and merge joins

**Independent Test**: Force hash/merge joins with query hints, verify EXPLAIN shows correct join strategy

### Tests for Hash and Merge Joins

- [ ] T140 [P] Create sql/hash_joins.sql with hash join tests using SET enable_mergejoin = off (matches test-cases.md Category 2a)
- [ ] T141 [P] Create expected/hash_joins.out with expected Hash Join plans
- [ ] T142 [P] Create sql/merge_joins.sql with merge join tests using SET enable_hashjoin = off (matches test-cases.md Category 2b)
- [ ] T143 [P] Create expected/merge_joins.out with expected Merge Join plans
- [ ] T144 Add hash_joins and merge_joins to REGRESS variable in Makefile

**Verification**:

- [ ] T145 Run `make installcheck` to verify hash and merge join tests pass
- [ ] T146 Verify Hash Join appears in EXPLAIN for equality predicates with hash join forced
- [ ] T147 Verify Merge Join appears in EXPLAIN for ordering predicates with merge join forced

**Checkpoint**: Hash and merge join support verified for all applicable operators

---

## Phase 9: Performance Benchmarks

**Goal**: Verify performance overhead is within acceptable bounds (<10% vs native operators)

**Independent Test**: Create 1M row table, compare execution time for exact comparisons vs native integer comparisons

### Tests for Performance

- [ ] T148 Create sql/performance.sql with timing tests comparing exact vs native comparisons
- [ ] T149 Create expected/performance.out with expected performance characteristics
- [ ] T150 Add performance to REGRESS variable in Makefile

**Verification**:

- [ ] T151 Run `make installcheck` to execute performance benchmarks
- [ ] T152 Verify sub-millisecond execution time for selective predicates
- [ ] T153 Verify overhead is within 10% of native integer comparisons

**Checkpoint**: Performance requirements met - extension is production-ready

---

## Phase 10: Polish & Cross-Cutting Concerns

**Purpose**: Documentation, cleanup, and final verification

- [ ] T154 [P] Update README.md with complete installation instructions and 5+ practical examples
- [ ] T155 [P] Update doc/user-guide.md with all operator usage patterns
- [ ] T156 [P] Update doc/api-reference.md with complete 54-operator reference
- [ ] T157 [P] Verify all C functions have doxygen comments with @brief, @param, @return
- [ ] T158 [P] Verify all files have copyright notices and AI assistance caveat
- [ ] T159 Run code style verification (K&R style, 2-space indentation, camelCase)
- [ ] T160 Run `make clean && make` with -Wall -Wextra -Werror to verify no warnings
- [ ] T161 Run full regression test suite `make installcheck` and verify 100% pass rate
- [ ] T162 Test extension on PostgreSQL 12, 13, 14, 15, 16 (multi-version compatibility)
- [ ] T163 Update CHANGELOG.md with complete 1.0.0 release notes
- [ ] T164 Review quickstart.md and verify all steps work correctly

**Checkpoint**: Extension complete and ready for release

---

## Dependencies & Execution Order

### Phase Dependencies

```
Setup (Phase 1) → Foundational (Phase 2) → User Stories (Phase 3-7) → Join Support (Phase 8) → Performance (Phase 9) → Polish (Phase 10)
                                              ↓
                                    All can run in parallel after Phase 2
```

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup - BLOCKS all user stories
- **User Story 1 (Phase 3 - P1)**: Depends on Phase 2 - MVP baseline
- **User Story 2 (Phase 4 - P1)**: Depends on Phase 2 and Phase 3 (needs equality operators for index tests)
- **User Story 3 (Phase 5 - P2)**: Depends on Phase 2 (can be parallel with US2 if different developers)
- **User Story 4 (Phase 6 - P2)**: Depends on Phase 2 (verification only, minimal implementation)
- **User Story 5 (Phase 7 - P3)**: Depends on Phases 3-4 (needs all operators for edge case tests)
- **Join Support (Phase 8)**: Depends on Phases 3-5 (needs all operators)
- **Performance (Phase 9)**: Depends on Phases 3-5 (needs all operators and index support)
- **Polish (Phase 10)**: Depends on all previous phases

### User Story Independence

Each user story is designed to be independently testable:

- **US1 (Equality)**: Can be tested with just equality operators, no index support needed
- **US2 (Index)**: Extends US1 with index optimization, independently verifiable via EXPLAIN
- **US3 (Range)**: Can be tested independently with range queries, separate from equality
- **US4 (Transitivity)**: Verification only, depends on having operators but tests independently
- **US5 (Coverage)**: Edge cases and type aliases, independently testable

### MVP Strategy

**Minimum Viable Product** = Phase 1 + Phase 2 + Phase 3 (User Story 1)

This delivers:
- Exact equality and inequality comparisons for all 9 type combinations
- Test-first verified correctness
- Precision mismatch detection (16777216 vs 16777217 float4 case)
- Foundation for adding index support and range operators

**Next Increment** = Phase 4 (User Story 2)
- Adds index optimization for equality predicates
- Makes extension production-usable for large tables

**Remaining Increments** = Phases 5-7 (User Stories 3-5)
- Range operators (US3)
- Transitivity verification (US4)
- Edge case coverage (US5)

### Parallel Opportunities

**Within Phase 2 (Foundational)**:
- T010-T012 (headers and structures) can be done in parallel
- T013-T014 (cache and support skeleton) depend on T010-T012
- T015-T018 (SQL and test directories) can be done in parallel with T010-T014

**Within Phase 3 (User Story 1)**:
- T019-T023 (all test file creation) can be done in parallel
- T024-T032 (all 9 _cmp_internal functions) can be done in parallel
- T033-T050 (all 18 operator wrappers) can be done in parallel after T024-T032
- T051-T052 (SQL registration) must be sequential after T033-T050

**Within Phase 5 (User Story 3)**:
- T068-T071 (test file updates) can be done in parallel
- T072-T107 (all 36 range operator wrappers) can be done in parallel
- T108-T109 (SQL registration) must be sequential

**Across User Stories**:
- US3 (Range) and US4 (Transitivity) can be worked on in parallel by different developers after Phase 2 completes
- All test file creation tasks marked [P] can run in parallel

### Example: Parallel Execution of User Story 1 Implementation

```bash
# After Phase 2 completes, these can all run in parallel:

# Developer 1: numeric comparisons
T024 (numeric_cmp_int2_internal)
T025 (numeric_cmp_int4_internal) 
T026 (numeric_cmp_int8_internal)
T033-T035 (numeric equality wrappers)
T042-T044 (numeric inequality wrappers)

# Developer 2: float4 comparisons
T027 (float4_cmp_int2_internal)
T028 (float4_cmp_int4_internal)
T029 (float4_cmp_int8_internal)
T036-T038 (float4 equality wrappers)
T045-T047 (float4 inequality wrappers)

# Developer 3: float8 comparisons
T030 (float8_cmp_int2_internal)
T031 (float8_cmp_int4_internal)
T032 (float8_cmp_int8_internal)
T039-T041 (float8 equality wrappers)
T048-T050 (float8 inequality wrappers)

# Once all complete, sequential tasks:
T051 (register all functions in SQL)
T052 (register all operators in SQL)
T053-T055 (build and verify)
```

---

## Implementation Strategy Summary

**Total Tasks**: 164

**By Phase**:
- Phase 1 (Setup): 9 tasks
- Phase 2 (Foundational): 9 tasks
- Phase 3 (US1 - Equality): 37 tasks
- Phase 4 (US2 - Index): 12 tasks
- Phase 5 (US3 - Range): 43 tasks
- Phase 6 (US4 - Transitivity): 9 tasks
- Phase 7 (US5 - Coverage): 16 tasks
- Phase 8 (Join Support): 8 tasks
- Phase 9 (Performance): 6 tasks
- Phase 10 (Polish): 11 tasks

**By Story**:
- US1 (Equality): 37 tasks - 9 core functions + 18 wrappers + tests + SQL
- US2 (Index): 12 tasks - Support function enhancement + tests
- US3 (Range): 43 tasks - 36 wrappers + tests + SQL
- US4 (Transitivity): 9 tasks - Verification tests + SQL review
- US5 (Coverage): 16 tasks - Edge case tests + special value handling

**Parallelization**:
- 89 tasks marked [P] can run in parallel within their phase
- 75 tasks are sequential (SQL registration, verification, integration)
- Estimated critical path: ~40-50 sequential task units (with parallel execution)

**Test-First Emphasis**:
- Every user story phase starts with test creation (T019-T023, T056-T058, T068-T071, etc.)
- Tests must FAIL before implementation begins (constitutional requirement)
- 100% test pass rate required before phase completion (T054, T066, T113, T121, T136, etc.)

**MVP Delivery Path**: 
Phase 1 → Phase 2 → Phase 3 → Verify → Deploy MVP

**Full Feature Delivery Path**:
Phase 1 → Phase 2 → Phases 3-7 (in priority order or parallel) → Phase 8 → Phase 9 → Phase 10 → Release 1.0.0
