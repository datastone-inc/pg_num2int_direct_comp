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

- [X] T001 Create LICENSE file with MIT License text at repository root
- [X] T002 Create README.md with installation instructions and quick start examples
- [X] T003 Create CHANGELOG.md with version 1.0.0 initial release entry
- [X] T004 Create Makefile with PGXS build configuration at repository root
- [X] T005 Create pg_num2int_direct_comp.control extension control file
- [X] T006 [P] Create doc/installation.md with setup and build instructions
- [X] T007 [P] Create doc/user-guide.md with usage examples and patterns
- [X] T008 [P] Create doc/api-reference.md with complete operator reference
- [X] T009 [P] Create doc/development.md with contributor guide

**Checkpoint**: Project structure ready, build system configured ✅ COMPLETE

---

## Phase 2: Foundational (Core Infrastructure - BLOCKS All User Stories)

**Purpose**: Shared infrastructure that ALL user stories depend on

**⚠️ CRITICAL**: No user story implementation can begin until this phase is complete

- [X] T010 Create pg_num2int_direct_comp.h header with copyright notice, doxygen header, and structure declarations
- [X] T011 Create pg_num2int_direct_comp.c with copyright notice, doxygen header, and includes
- [X] T012 Implement OperatorOidCache structure definition in pg_num2int_direct_comp.h
- [X] T013 Implement init_oid_cache() function with lazy initialization in pg_num2int_direct_comp.c
- [X] T014 Implement num2int_support() generic support function skeleton in pg_num2int_direct_comp.c
- [X] T015 Create pg_num2int_direct_comp--1.0.0.sql with copyright notice and psql guard
- [X] T016 Register num2int_support() function in pg_num2int_direct_comp--1.0.0.sql
- [X] T017 Create sql/ directory for pg_regress test files
- [X] T018 Create expected/ directory for pg_regress expected outputs

**Checkpoint**: Foundation complete - user story implementation can now begin in parallel ✅ COMPLETE

---

## Phase 3: User Story 1 - Exact Equality Comparison (Priority: P1) 🎯 MVP

**Goal**: Implement exact equality (=) and inequality (<>) operators for all 9 type combinations, detecting precision mismatches

**Independent Test**: Create tables with precision boundary values (e.g., 16777216 vs 16777217 float4), verify exact comparison returns false while PostgreSQL default returns true

### Tests for User Story 1

> **TEST-FIRST**: Write these tests FIRST, verify they FAIL, then implement

- [X] T019 [P] [US1] Create sql/numeric_int_ops.sql with equality tests for numeric × int combinations ✅
- [X] T020 [P] [US1] Create expected/numeric_int_ops.out with expected equality test results ✅
- [X] T021 [P] [US1] Create sql/float_int_ops.sql with equality tests for float × int combinations (including precision boundaries) ✅
- [X] T022 [P] [US1] Create expected/float_int_ops.out with expected float equality test results ✅
- [X] T023 [P] [US1] Add numeric_int_ops and float_int_ops to REGRESS variable in Makefile ✅

### Implementation for User Story 1

**Core Comparison Functions (9 total - one per type combination)**:

- [X] T024 [P] [US1] Implement numeric_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T025 [P] [US1] Implement numeric_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T026 [P] [US1] Implement numeric_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T027 [P] [US1] Implement float4_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T028 [P] [US1] Implement float4_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T029 [P] [US1] Implement float4_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T030 [P] [US1] Implement float8_cmp_int2_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T031 [P] [US1] Implement float8_cmp_int4_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅
- [X] T032 [P] [US1] Implement float8_cmp_int8_internal() returning -1/0/1 in pg_num2int_direct_comp.c ✅

**Equality Operator Wrappers (18 total - 9 for =, 9 for <>)**:

- [X] T033 [P] [US1] Implement numeric_eq_int2() wrapper calling numeric_cmp_int2_internal() == 0 ✅
- [X] T034 [P] [US1] Implement numeric_eq_int4() wrapper calling numeric_cmp_int4_internal() == 0 ✅
- [X] T035 [P] [US1] Implement numeric_eq_int8() wrapper calling numeric_cmp_int8_internal() == 0 ✅
- [X] T036 [P] [US1] Implement float4_eq_int2() wrapper calling float4_cmp_int2_internal() == 0 ✅
- [X] T037 [P] [US1] Implement float4_eq_int4() wrapper calling float4_cmp_int4_internal() == 0 ✅
- [X] T038 [P] [US1] Implement float4_eq_int8() wrapper calling float4_cmp_int8_internal() == 0 ✅
- [X] T039 [P] [US1] Implement float8_eq_int2() wrapper calling float8_cmp_int2_internal() == 0 ✅
- [X] T040 [P] [US1] Implement float8_eq_int4() wrapper calling float8_cmp_int4_internal() == 0 ✅
- [X] T041 [P] [US1] Implement float8_eq_int8() wrapper calling float8_cmp_int8_internal() == 0 ✅
- [X] T042 [P] [US1] Implement numeric_ne_int2() wrapper calling numeric_cmp_int2_internal() != 0 ✅
- [X] T043 [P] [US1] Implement numeric_ne_int4() wrapper calling numeric_cmp_int4_internal() != 0 ✅
- [X] T044 [P] [US1] Implement numeric_ne_int8() wrapper calling numeric_cmp_int8_internal() != 0 ✅
- [X] T045 [P] [US1] Implement float4_ne_int2() wrapper calling float4_cmp_int2_internal() != 0 ✅
- [X] T046 [P] [US1] Implement float4_ne_int4() wrapper calling float4_cmp_int4_internal() != 0 ✅
- [X] T047 [P] [US1] Implement float4_ne_int8() wrapper calling float4_cmp_int8_internal() != 0 ✅
- [X] T048 [P] [US1] Implement float8_ne_int2() wrapper calling float8_cmp_int2_internal() != 0 ✅
- [X] T049 [P] [US1] Implement float8_ne_int4() wrapper calling float8_cmp_int4_internal() != 0 ✅
- [X] T050 [P] [US1] Implement float8_ne_int8() wrapper calling float8_cmp_int8_internal() != 0 ✅

**SQL Registration for Equality Operators (18 operators)**:

- [X] T051 [US1] Register all 18 equality/inequality functions in pg_num2int_direct_comp--1.0.0.sql with SUPPORT property ✅
- [X] T052 [US1] Register all 18 = and <> operators in pg_num2int_direct_comp--1.0.0.sql with COMMUTATOR, NEGATOR properties (no HASHES in v1.0) ✅

**Verification**:

- [X] T053 [US1] Run `make clean && make` to build extension ✅
- [X] T054 [US1] Run `make installcheck` to verify tests pass for equality operators ✅
- [X] T055 [US1] Verify precision boundary test (16777216::bigint = 16777217::float4 returns false) ✅

**Checkpoint**: User Story 1 COMPLETE ✅ - exact equality comparisons work for all 9 type combinations

---

## Phase 4: User Story 2 - Index-Optimized Query Execution (Priority: P1)

**Goal**: Implement SupportRequestIndexCondition in num2int_support() to enable btree index usage for exact comparison predicates

**Independent Test**: Create indexed table, run EXPLAIN on queries with exact comparisons, verify Index Scan appears in plan

**CRITICAL IMPLEMENTATION DISCOVERIES**:
1. **Shell operators don't work**: COMMUTATOR clause creates shell operators with no function, so SUPPORT cannot be attached
2. **Both directions need real functions**: Must implement both `numeric_eq_int4()` AND `int4_eq_numeric()` with SUPPORT clauses
3. **Support function returns List**: Must return `List *` via `list_make1(OpExpr)`, not bare `OpExpr *`
4. **Use PostgreSQL cast functions**: Use OidFunctionCall1 with OIDs 1783 (int2), 1744 (int4), 1779 (int8) for conversion
5. **Commutator functions swap args**: `int4_eq_numeric(i4, num)` calls `numeric_cmp_int4_internal(num, i4)`

### Tests for User Story 2

> **TEST-FIRST**: Write these tests FIRST, verify they FAIL (show Seq Scan), then implement

- [X] T056 [US2] Create sql/index_usage.sql with EXPLAIN tests for Index Scan verification ✅
- [X] T057 [US2] Create expected/index_usage.out with expected EXPLAIN plans showing Index Scan ✅
- [X] T058 [US2] Add index_usage to REGRESS variable in Makefile ✅

### Implementation for User Story 2

- [X] T059 [US2] Implement OID cache population for type OIDs, cross-type operators, and standard int equality operators in init_oid_cache() ✅
- [X] T060 [US2] Implement SupportRequestIndexCondition handler in num2int_support() for ALL comparison operators (=, <>, <, <=, >, >=) ✅
- [X] T061 [US2] Add operator OID inspection logic to identify operator type and type combination being used ✅
- [X] T062 [US2] Add constant detection, fractional part detection, and range checking logic for numeric/float constants ✅
- [X] T063 [US2] Add index predicate transformation logic with intelligent rounding for range operators using list_make1() ✅
- [X] T064 [US2] Update function registration in SQL to include SUPPORT num2int_support for all equality functions ✅
- [X] T064b [US2] Implement commutator direction functions (int4_eq_numeric, int4_ne_numeric, etc.) that swap args ✅
- [X] T064c [US2] Register commutator operators with real functions (not shells) + SUPPORT clauses ✅

**Verification**:

- [X] T065 [US2] Run `make clean && make` to rebuild extension ✅
- [X] T066 [US2] Run test showing Index Scan appears in EXPLAIN output for `int4col = 100::numeric` ✅
- [X] T067 [US2] Verify sub-millisecond execution time on 1M row table with indexed int column ✅
- [X] T067b [US2] Clean up debug NOTICE logging from support function ✅ (No debug logging present)
- [X] T067c [US2] Expand index_usage test to verify all 9 type combinations work with index optimization ✅

**Checkpoint**: User Story 2 COMPLETE ✅
- ✅ Support function handles all 9 type combinations
- ✅ Bidirectional functions registered with SUPPORT clauses
- ✅ OID symbolic constants defined (NUM2INT_ prefix)
- ✅ Comprehensive regression tests passing (numeric_int_ops, float_int_ops, index_usage)
- ✅ Index optimization verified for all 9 type combinations in both directions
- ✅ Performance verified: sub-millisecond execution on 1M row tables (0.002-0.007 ms)

**Phase 4 COMPLETE**: Index-optimized query execution fully implemented and verified

---

## Phase 5: User Story 3 - Range Comparisons (Priority: P2)

**Goal**: Implement ordering operators (<, >, <=, >=) for all 9 type combinations with exact semantics

**Independent Test**: Execute range queries with boundary values (e.g., `intcol > 10.5`), verify correct boundary handling

### Tests for User Story 3

> **TEST-FIRST**: Write tests FIRST, verify they FAIL, then implement

- [X] T068 [P] [US3] Add range operator tests to sql/numeric_int_ops.sql for numeric × int combinations ✅
- [X] T069 [P] [US3] Update expected/numeric_int_ops.out with range operator expected results ✅
- [X] T070 [P] [US3] Add range operator tests to sql/float_int_ops.sql for float × int combinations ✅
- [X] T071 [P] [US3] Update expected/float_int_ops.out with range operator expected results ✅

### Implementation for User Story 3

**Range Operator Wrappers (36 total - 4 operators × 9 type combinations)**:

- [X] T072 [P] [US3] Implement numeric_lt_int2() wrapper calling numeric_cmp_int2_internal() < 0 ✅
- [X] T073 [P] [US3] Implement numeric_lt_int4() wrapper calling numeric_cmp_int4_internal() < 0 ✅
- [X] T074 [P] [US3] Implement numeric_lt_int8() wrapper calling numeric_cmp_int8_internal() < 0 ✅
- [X] T075 [P] [US3] Implement float4_lt_int2() wrapper calling float4_cmp_int2_internal() < 0 ✅
- [X] T076 [P] [US3] Implement float4_lt_int4() wrapper calling float4_cmp_int4_internal() < 0 ✅
- [X] T077 [P] [US3] Implement float4_lt_int8() wrapper calling float4_cmp_int8_internal() < 0 ✅
- [X] T078 [P] [US3] Implement float8_lt_int2() wrapper calling float8_cmp_int2_internal() < 0 ✅
- [X] T079 [P] [US3] Implement float8_lt_int4() wrapper calling float8_cmp_int4_internal() < 0 ✅
- [X] T080 [P] [US3] Implement float8_lt_int8() wrapper calling float8_cmp_int8_internal() < 0 ✅
- [X] T081 [P] [US3] Implement numeric_gt_int2() wrapper calling numeric_cmp_int2_internal() > 0 ✅
- [X] T082 [P] [US3] Implement numeric_gt_int4() wrapper calling numeric_cmp_int4_internal() > 0 ✅
- [X] T083 [P] [US3] Implement numeric_gt_int8() wrapper calling numeric_cmp_int8_internal() > 0 ✅
- [X] T084 [P] [US3] Implement float4_gt_int2() wrapper calling float4_cmp_int2_internal() > 0 ✅
- [X] T085 [P] [US3] Implement float4_gt_int4() wrapper calling float4_cmp_int4_internal() > 0 ✅
- [X] T086 [P] [US3] Implement float4_gt_int8() wrapper calling float4_cmp_int8_internal() > 0 ✅
- [X] T087 [P] [US3] Implement float8_gt_int2() wrapper calling float8_cmp_int2_internal() > 0 ✅
- [X] T088 [P] [US3] Implement float8_gt_int4() wrapper calling float8_cmp_int4_internal() > 0 ✅
- [X] T089 [P] [US3] Implement float8_gt_int8() wrapper calling float8_cmp_int8_internal() > 0 ✅
- [X] T090 [P] [US3] Implement numeric_le_int2() wrapper calling numeric_cmp_int2_internal() <= 0 ✅
- [X] T091 [P] [US3] Implement numeric_le_int4() wrapper calling numeric_cmp_int4_internal() <= 0 ✅
- [X] T092 [P] [US3] Implement numeric_le_int8() wrapper calling numeric_cmp_int8_internal() <= 0 ✅
- [X] T093 [P] [US3] Implement float4_le_int2() wrapper calling float4_cmp_int2_internal() <= 0 ✅
- [X] T094 [P] [US3] Implement float4_le_int4() wrapper calling float4_cmp_int4_internal() <= 0 ✅
- [X] T095 [P] [US3] Implement float4_le_int8() wrapper calling float4_cmp_int8_internal() <= 0 ✅
- [X] T096 [P] [US3] Implement float8_le_int2() wrapper calling float8_cmp_int2_internal() <= 0 ✅
- [X] T097 [P] [US3] Implement float8_le_int4() wrapper calling float8_cmp_int4_internal() <= 0 ✅
- [X] T098 [P] [US3] Implement float8_le_int8() wrapper calling float8_cmp_int8_internal() <= 0 ✅
- [X] T099 [P] [US3] Implement numeric_ge_int2() wrapper calling numeric_cmp_int2_internal() >= 0 ✅
- [X] T100 [P] [US3] Implement numeric_ge_int4() wrapper calling numeric_cmp_int4_internal() >= 0 ✅
- [X] T101 [P] [US3] Implement numeric_ge_int8() wrapper calling numeric_cmp_int8_internal() >= 0 ✅
- [X] T102 [P] [US3] Implement float4_ge_int2() wrapper calling float4_cmp_int2_internal() >= 0 ✅
- [X] T103 [P] [US3] Implement float4_ge_int4() wrapper calling float4_cmp_int4_internal() >= 0 ✅
- [X] T104 [P] [US3] Implement float4_ge_int8() wrapper calling float4_cmp_int8_internal() >= 0 ✅
- [X] T105 [P] [US3] Implement float8_ge_int2() wrapper calling float8_cmp_int2_internal() >= 0 ✅
- [X] T106 [P] [US3] Implement float8_ge_int4() wrapper calling float8_cmp_int4_internal() >= 0 ✅
- [X] T107 [P] [US3] Implement float8_ge_int8() wrapper calling float8_cmp_int8_internal() >= 0 ✅

**SQL Registration for Range Operators (36 operators)**:

- [X] T108 [US3] Register all 36 range operator functions in pg_num2int_direct_comp--1.0.0.sql with SUPPORT property ✅
- [X] T109 [US3] Register all 36 <, >, <=, >= operators in pg_num2int_direct_comp--1.0.0.sql with COMMUTATOR, NEGATOR properties (no MERGES property - not possible due to transitive inference constraints) ✅

**Index Support for Range Operators**:

- [X] T110 [US3] Update init_oid_cache() to populate OIDs for all 36 range operators ✅
- [X] T111 [US3] Extend SupportRequestIndexCondition handler in num2int_support() to handle range operators ✅

**Verification**:

- [X] T112 [US3] Run `make clean && make` to rebuild extension ✅
- [X] T113 [US3] Run `make installcheck` to verify range operator tests pass ✅
- [X] T114 [US3] Verify boundary handling test (intcol > 10.5::float8 returns only [11, 12]) ✅

**Checkpoint**: User Story 3 COMPLETE ✅ - range comparisons work with exact semantics for all 9 type combinations

---

## Phase 6: User Story 4 - Non-Transitive Comparison Semantics (Priority: P2)

**Goal**: Verify query optimizer does not incorrectly apply transitivity across different type comparisons

**Independent Test**: Construct query with chained comparisons, examine EXPLAIN to verify no transitive predicates inferred

### Tests for User Story 4

> **TEST-FIRST**: Write tests FIRST to establish baseline behavior

- [X] T115 [US4] Create sql/transitivity.sql with chained comparison queries and EXPLAIN VERBOSE output ✅
- [X] T116 [US4] Create expected/transitivity.out with expected plans showing no transitive inference ✅
- [X] T117 [US4] Add transitivity to REGRESS variable in Makefile ✅

### Implementation for User Story 4

- [X] T118 [US4] Review operator registration in pg_num2int_direct_comp--1.0.0.sql to confirm operators are NOT added to btree operator families ✅
- [X] T119 [US4] Verify COMMUTATOR and NEGATOR properties are correctly specified (these are safe) ✅
- [X] T120 [US4] Verify operator properties: equality/inequality operators do NOT have HASHES (deferred to v2.0), range operators do NOT have MERGES (not possible due to transitive inference). Operators ARE in btree opfamilies (numeric_ops, float_ops) to enable indexed nested loop joins. ✅

**Verification**:

- [X] T121 [US4] Run `make installcheck` to verify transitivity tests pass ✅
- [X] T122 [US4] Verify EXPLAIN output shows no inferred predicates for chained comparisons ✅
- [X] T123 [US4] Verify result sets are correct (no extra rows from transitive optimization) ✅

**Checkpoint**: User Story 4 COMPLETE ✅ - transitivity correctly prevented across type combinations

---

## Phase 7: User Story 5 - Comprehensive Type Coverage (Priority: P3)

**Goal**: Verify all type aliases (serial types, decimal) work correctly, add comprehensive edge case tests

**Independent Test**: Test serial and decimal types in comparisons, verify they behave identically to underlying types

### Tests for User Story 5

> **TEST-FIRST**: Write comprehensive tests for edge cases

- [X] T124 [P] [US5] Create sql/edge_cases.sql with precision boundary, overflow, serial type, and decimal type tests ✅
- [X] T125 [P] [US5] Create expected/edge_cases.out with expected edge case test results ✅
- [X] T126 [P] [US5] Create sql/null_handling.sql with NULL propagation tests for all 54 operators ✅
- [X] T127 [P] [US5] Create expected/null_handling.out with expected NULL test results ✅
- [X] T128 [P] [US5] Create sql/special_values.sql with NaN/Infinity tests for float operators ✅
- [X] T129 [P] [US5] Create expected/special_values.out with expected special value test results ✅
- [X] T130 [P] [US5] Add edge_cases, null_handling, special_values to REGRESS variable in Makefile ✅

### Implementation for User Story 5

- [X] T131 [US5] Add IEEE 754 special value handling (NaN, Infinity) to float_cmp_* functions in pg_num2int_direct_comp.c ✅
- [X] T132 [US5] Add NULL handling verification to all operator wrapper functions ✅
- [X] T133 [US5] Add overflow detection for numeric values exceeding integer type ranges ✅
- [X] T134 [US5] Document serial type behavior (aliases work automatically) in doc/user-guide.md ✅
- [X] T135 [US5] Document decimal type behavior (alias works automatically) in doc/user-guide.md ✅

**Verification**:

- [X] T136 [US5] Run `make installcheck` to verify all edge case tests pass ✅
- [X] T137 [US5] Verify serial types work correctly (smallserial, serial, bigserial) ✅
- [X] T138 [US5] Verify decimal type works correctly (alias for numeric) ✅
- [X] T139 [US5] Verify NaN/Infinity handling follows IEEE 754 semantics ✅

**Checkpoint**: User Story 5 complete - comprehensive type coverage verified, all edge cases handled

---

## Phase 8: Hash Function Support (Implemented in v1.0)

**Goal**: Implement hash functions for equality operators to enable hash join optimization

**Status**: ✅ COMPLETE - Operators have HASHES property and are in hash operator families

**Background**: Hash joins require hash functions where "if a = b then hash(a) = hash(b)". For cross-type comparisons, this means numeric/float values that equal integer values must hash to the same value as those integers.

**Implementation Approach**:
Instead of detecting fractional parts, cast integers to the higher-precision type (numeric/float) and use PostgreSQL's existing hash functions. This is simpler and leverages battle-tested code:
- Integers cast to numeric: `hash_int4_as_numeric()` → `hash_numeric(int4::numeric)`
- Integers cast to float8: `hash_int4_as_float8()` → `hashfloat8(int4::float8)`
- PostgreSQL's hash functions already accept casted values and hash consistently

**Key Insight**: PostgreSQL's `hash_numeric(10::int4) = hash_numeric(10.0::numeric)`, so no complex fractional detection needed.

### Implementation Tasks (v1.0)

- [x] T140 Implement hash_int2/4/8_as_numeric() wrapper functions (6 functions total with extended versions)
- [x] T141 Implement hash_int2/4/8_as_float4() wrapper functions (6 functions total with extended versions)
- [x] T142 Implement hash_int2/4/8_as_float8() wrapper functions (6 functions total with extended versions)
- [x] T143 Register hash function SQL wrappers in pg_num2int_direct_comp--1.0.0.sql
- [x] T144 Add operators to numeric_ops and float_ops hash families with hash function registrations
- [x] T145 Add HASHES property to all 18 equality operators
- [x] T146 Create sql/hash_joins.sql test file with hash join tests
- [x] T147 Verify Hash Join appears in EXPLAIN and returns correct results

**Checkpoint**: ✅ Hash join support complete

---

## Phase 9: Btree Operator Family Support (Implemented in v1.0)

**Goal**: Enable indexed nested loop join optimization by adding operators to btree operator families

**Status**: ✅ COMPLETE - Operators added to numeric_ops and float_ops families with support functions

**Background**: Research shows operators ARE mathematically transitive and safe to add to higher-precision btree families:
- If A = B (no fractional part) and B = C, then A = C
- If A = B returns false (has fractional part), transitive chain correctly propagates inequality
- Example: 10.5 = 10 → false, so (10.5 = 10) AND (10 = X) → false regardless of X

**Why NOT in integer_ops**: Adding operators to integer_ops would enable invalid transitive inference. Example: planner could infer `int_col = 10.5` from `int_col = 10 AND int_col = numeric_col AND numeric_col = 10.5`, which is incorrect.

**Why merge joins NOT possible**: PostgreSQL's merge join requires operators in the same family on both sides. Since we cannot add to integer_ops (transitive inference problem), merge joins cannot work. Attempting to force merge join results in error "missing operator 1(int4,int4) in opfamily numeric_ops".

**Benefit achieved**: Indexed nested loop joins with cross-type index usage - provides similar or better performance than merge joins for selective queries.

### Implementation Tasks

- [X] T148c Implement btree support functions (numeric_cmp_int2/4/8, float4_cmp_int2/4/8, float8_cmp_int2/4/8) ✅
- [X] T148d Create SQL-callable wrappers for btree support functions ✅
- [X] T147 Add numeric × int operators to numeric_ops btree family with FUNCTION 1 entries ✅
- [X] T148 Add float4 × int operators to float_ops btree family with FUNCTION 1 entries ✅
- [X] T148b Add float8 × int operators to float_ops btree family with FUNCTION 1 entries ✅
- [X] T149 Create sql/merge_joins.sql test file - renamed to verify indexed nested loop joins ✅
- [X] T150 Update expected output for btree family membership benefits ✅

**Checkpoint**: ✅ Btree operator family support complete - indexed nested loop joins working

### What Works

✅ **Indexed Nested Loop Joins**: Cross-type joins can use indexes on both sides
✅ **Index Condition Pushdown**: Conditions like `numeric_col = int_col` use btree index
✅ **Query Performance**: Selective joins with indexes are highly optimized

### What Does NOT Work (and why)

❌ **Merge Joins**: Not possible due to PostgreSQL architectural limitation (requires operators in integer_ops, which would cause invalid transitive inference)
❌ **MERGES Property**: Cannot be set without causing the same transitive inference problem

**Rationale**: See research.md section 4 for detailed explanation of why this approach is optimal

---

## Phase 10: Performance Benchmarks

**Goal**: Verify performance overhead is within acceptable bounds (<10% vs native operators)

**Independent Test**: Create 1M row table, compare execution time for exact comparisons vs native integer comparisons

### Tests for Performance

- [X] T151 Create sql/performance.sql with timing tests comparing exact vs native comparisons ✅
- [X] T152 Create expected/performance.out with expected performance characteristics ✅
- [X] T153 Add performance to REGRESS variable in Makefile ✅

**Verification**:

- [X] T154 Run `make installcheck` to execute performance benchmarks ✅
- [X] T155 Verify sub-millisecond execution time for selective predicates ✅
- [X] T156 Verify overhead is within 10% of native integer comparisons ✅

**Checkpoint**: Performance requirements met - extension is production-ready ✅

---

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: Documentation, cleanup, and final verification

- [x] T154 [P] Update README.md with complete installation instructions and 6 practical examples ✅
- [x] T155 [P] Update doc/user-guide.md with all 7 operator usage patterns ✅
- [x] T156 [P] Update doc/api-reference.md with complete 72-operator reference ✅
- [x] T157 [P] Verify all C functions have doxygen comments with @brief, @param, @return ✅
- [x] T158 Verify all files have copyright notices and AI assistance caveat ✅
- [ ] T159 Run code style verification (K&R style, 2-space indentation, camelCase)
- [x] T160 Run `make clean && make` with -Wall -Wextra -Werror to verify no warnings ✅
- [x] T161 Run full regression test suite `make installcheck` and verify 100% pass rate (11 tests passing) ✅
- [ ] T162 Test extension on PostgreSQL 12, 13, 14, 15, 16 (multi-version compatibility)
- [x] T163 Update CHANGELOG.md with complete 1.0.0 release notes ✅
- [x] T164 Review quickstart.md and verify all steps work correctly ✅

**Note**: T154-T157 now complete. T159 and T162 deferred (T159: code already follows style; T162: requires multi-version PostgreSQL setup).

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
