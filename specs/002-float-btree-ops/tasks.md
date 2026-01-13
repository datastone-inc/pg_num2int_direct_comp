# Tasks: Float x Integer Btree Operator Family Integration

**Input**: Design documents from `/specs/002-float-btree-ops/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/operators.md ✅, quickstart.md ✅

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup & Failing Tests

**Purpose**: Prepare test infrastructure and write failing tests BEFORE implementation (constitution.md requirement)

- [X] T001 Create new regression test file sql/float_int_ops.sql with header comments and initial failing test stubs (merge join, catalog queries, transitivity)
- [X] T002 [P] Create expected/float_int_ops.out with expected output (tests will FAIL until Phase 2 completes)
- [X] T003 [P] Add float_int_ops to REGRESS list in Makefile
- [X] T003a Verify tests FAIL with `make installcheck REGRESS=float_int_ops` (constitution gate)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T004 Add int x float4 operators to float_ops btree family in pg_num2int_direct_comp--1.0.0.sql
- [X] T005 Add int x float8 operators to float_ops btree family in pg_num2int_direct_comp--1.0.0.sql
- [X] T006 Add float4/float8 same-type and cross-type operators to integer_ops btree family in pg_num2int_direct_comp--1.0.0.sql
- [X] T007 Extend event trigger cleanup array with new btree family DROP statements in pg_num2int_direct_comp--1.0.0.sql
- [X] T008 Add GUC parameter `pg_num2int_direct_comp.enable_support_functions` with DefineCustomBoolVariable in _PG_init()
- [X] T009 [P] Add GUC checks to SupportRequestSimplify and SupportRequestIndexCondition functions
- [X] T010 Add MERGES property to all equality operators to enable merge join optimization

**Checkpoint**: Btree family registration complete - user story testing can now begin

---

## Phase 3: User Story 1 - Merge Joins for Integer x Float Columns (Priority: P1) 🎯 MVP

**Goal**: Enable merge join optimization for queries joining tables on integer and float columns

**Independent Test**: Create indexed tables with integer and float columns, execute join queries with `enable_hashjoin=off` and `enable_nestloop=off`, verify EXPLAIN shows Merge Join

### Tests for User Story 1

- [X] T008 [US1] Add merge join test for int4 x float4 in sql/float_int_ops.sql (verify EXPLAIN shows Merge Join)
- [X] T009 [P] [US1] Add merge join test for int4 x float8 in sql/float_int_ops.sql
- [X] T010 [P] [US1] Add merge join test for int8 x float4 in sql/float_int_ops.sql
- [X] T011 [P] [US1] Add merge join test for int8 x float8 in sql/float_int_ops.sql
- [X] T012 [US1] Add pg_amop catalog verification query for float_ops btree in sql/float_int_ops.sql (expect 60 entries)
- [X] T014 [P] [US1] Add pg_amop catalog verification query for integer_ops btree float entries in sql/float_int_ops.sql (expect 70 entries)
- [X] T014a [P] [US1] Add pg_operator verification query to confirm MERGES property on equality operators (FR-008)
- [X] T015 [US1] Run regression test and capture expected/float_int_ops.out

**Checkpoint**: Merge join functionality verified - US1 complete

---

## Phase 4: User Story 2 - Mathematical Transitivity Verification (Priority: P1)

**Goal**: Validate that exact comparison operators satisfy btree transitivity requirements

**Independent Test**: Create chained equality conditions, verify planner transitive inference, prove exact comparison semantics preserve transitivity for stored float values

### Tests for User Story 2

- [X] T016 [US2] Add transitivity test for int4 = float4 = int4 chain in sql/float_int_ops.sql
- [X] T017 [P] [US2] Add transitivity test for int8 = float8 = int8 chain in sql/float_int_ops.sql
- [X] T018 [P] [US2] Add NaN transitivity test (NaN = NaN, NaN > all non-NaN) in sql/float_int_ops.sql
- [X] T019 [P] [US2] Add +/-Infinity ordering test (-Inf < all ints < +Inf) in sql/float_int_ops.sql
- [X] T020 [US2] Add precision boundary transitivity test (16777216 boundary for float4) in sql/float_int_ops.sql
- [X] T020a [US2] Add EXPLAIN test for transitive inference (verify planner simplifies chained conditions) in sql/float_int_ops.sql
- [X] T021 [US2] Update expected/float_int_ops.out with transitivity test results

**Checkpoint**: Transitivity proven - US2 complete

---

## Phase 5: User Story 3 - Documentation Clarity and Persuasiveness (Priority: P2)

**Goal**: Reduce documentation by 20% word count while addressing "non-standard operators" concern

**Independent Test**: Measure word count before/after, verify doc-example-reviewer reports 100% compliance

### Implementation for User Story 3

- [X] T022 [US3] Measure baseline word count for README.md and doc/*.md (target: reduce from 11,234 to ≤8,987)
- [X] T023 [US3] Update README.md: add "Why Custom Operators?" section addressing non-standard concern
- [X] T024 [US3] Update README.md: remove duplicate precision loss explanations
- [X] T025 [US3] Update README.md: add equivalence expressions section (with/without extension)
- [X] T026 [US3] Update README.md: update Limitations section to reflect merge join support for int x float
- [X] T026a [US3] Update README.md: add GUC configuration section documenting pg_num2int_direct_comp.enable_support_functions
- [X] T027 [P] [US3] Update doc/api-reference.md: add float btree family information concisely
- [X] T028 [P] [US3] Update doc/operator-reference.md: add float operators to tables
- [X] T029 [US3] Add all README SQL examples to sql/doc_examples.sql per constitution.md
- [X] T029a [US3] Add GUC usage examples to sql/doc_examples.sql (enable/disable, SHOW command)
- [X] T030 [US3] Run doc-example-reviewer skill to verify 100% compliance (manual review completed, all examples tested)
- [X] T031 [US3] Measure final word count and verify ≥20% reduction

**Checkpoint**: Documentation reduced and examples tested - US3 complete

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Validation and cleanup across all user stories

- [X] T032 [P] Add extension lifecycle test (DROP/CREATE cycle) for float btree families in sql/extension_lifecycle.sql
- [X] T032a [P] Add GUC configuration testing to sql/extension_lifecycle.sql (SHOW, SET, enable/disable verification)
- [X] T033 [P] Update expected/extension_lifecycle.out with new cleanup verification
- [X] T034 Run full regression suite: make installcheck
- [X] T035 Verify all pg_amop entries removed after DROP EXTENSION via catalog query
- [X] T036 Run quickstart.md validation: execute all code blocks and verify expected output

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies - start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 - BLOCKS all user stories
- **Phase 3-5 (User Stories)**: All depend on Phase 2 completion
  - US1 and US2 are both P1 and can run in parallel
  - US3 (P2) can run in parallel with US1/US2 or after
- **Phase 6 (Polish)**: Depends on Phase 2 minimum; ideally after all stories

### Within Each User Story

- Tests written first (constitution.md test-first requirement)
- Tests must FAIL before implementation (already done in Phase 2)
- Update expected output after test runs pass

### Parallel Opportunities

**Phase 1**: T002, T003 can run in parallel with T001
**Phase 3**: T009-T011 can run in parallel; T013-T014 can run in parallel
**Phase 4**: T017-T019 can run in parallel
**Phase 5**: T027-T028 can run in parallel
**Phase 6**: T032-T033 can run in parallel

---

## Implementation Strategy

### MVP First (User Stories 1 + 2)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (SQL implementation)
3. Complete Phase 3: User Story 1 (merge join tests)
4. Complete Phase 4: User Story 2 (transitivity tests)
5. **STOP and VALIDATE**: Run `make installcheck` - all tests pass
6. Merge to main if documentation update not critical

### Full Delivery

1. Complete MVP (Phases 1-4)
2. Complete Phase 5: User Story 3 (documentation)
3. Complete Phase 6: Polish
4. Final validation: all tests pass, word count reduced, doc-example-reviewer clean

---

## Summary

| Phase | Task Count | Purpose |
|-------|------------|---------|
| Setup | 3 | Test infrastructure |
| Foundational | 4 | SQL implementation |
| US1 - Merge Joins | 8 | Core functionality tests |
| US2 - Transitivity | 6 | Correctness verification |
| US3 - Documentation | 10 | Word reduction + examples |
| Polish | 5 | Cross-cutting validation |
| **Total** | **36** | |

### Parallel Opportunities

- Phase 1: 2 tasks parallelizable
- Phase 3: 6 tasks parallelizable
- Phase 4: 3 tasks parallelizable
- Phase 5: 2 tasks parallelizable
- Phase 6: 2 tasks parallelizable

### MVP Scope

User Stories 1 + 2 (Phases 1-4): **21 tasks** → Core merge join functionality with transitivity proof

### Format Validation

✅ All tasks follow checklist format: `- [ ] [TaskID] [P?] [Story?] Description with file path`
