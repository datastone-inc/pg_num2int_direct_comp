# Tasks: Remove Redundant SupportRequestIndexCondition Handler

**Input**: Design documents from `/specs/003-remove-index-condition/`
**Prerequisites**: plan.md ✓, spec.md ✓, research.md ✓, data-model.md ✓, contracts/ ✓

**Tests**: Not explicitly requested - using existing regression tests for verification.

**Organization**: Tasks grouped by user story to enable independent implementation.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (US1, US2)
- Include exact file paths in descriptions

---

## Phase 1: Setup

**Purpose**: Establish baseline before making changes

- [x] T001 Verify baseline: run `make clean && make && make installcheck` passes

---

## Phase 2: Foundational

**Purpose**: No foundational tasks needed - this is a removal/cleanup feature

**Checkpoint**: Proceed directly to user stories

---

## Phase 3: User Story 1 - Maintainer Simplifies Codebase (Priority: P1) 🎯 MVP

**Goal**: Remove dead code paths so the codebase is easier to understand, test, and maintain

**Independent Test**: All existing regression tests pass after removal (`make installcheck`)

### Implementation for User Story 1

- [x] T002 [US1] Update file header comment in pg_num2int_direct_comp.c (line 20) - remove SupportRequestIndexCondition reference
- [x] T003 [US1] Update GUC description in pg_num2int_direct_comp.c (_PG_init, ~line 411) - reference only SupportRequestSimplify
- [x] T004 [US1] Update helper section comment in pg_num2int_direct_comp.c (~line 612) - remove dual-handler reference
- [x] T005 [US1] Update num2int_support docblock in pg_num2int_direct_comp.c (~line 983) - document only SupportRequestSimplify
- [x] T006 [US1] Remove SupportRequestIndexCondition handler block in pg_num2int_direct_comp.c (~lines 1003-1107)
- [x] T007 [US1] Verify compilation: run `make clean && make` - should compile without errors
- [x] T008 [US1] Verify regression tests: run `make installcheck` - all tests should pass

**Checkpoint**: Code removal complete, functionality preserved (SC-001, SC-002)

---

## Phase 4: User Story 2 - Developer Understands Support Function (Priority: P2)

**Goal**: Single clear transformation path with clear documentation

**Independent Test**: Code review confirms single handler with clear docs; no SupportRequestIndexCondition references in user-facing files

### Implementation for User Story 2

- [x] T009 [P] [US2] Update num2int_support docblock in pg_num2int_direct_comp.h (~line 388)
- [x] T010 [P] [US2] Update support function mechanism text in doc/api-reference.md (line 184)
- [x] T011 [P] [US2] Update support function mechanism text in doc/operator-reference.md (line 178)
- [x] T012 [US2] Verify no stray references: run `grep -r "SupportRequestIndexCondition" . --include="*.c" --include="*.h" --include="*.md" | grep -v "specs/"` - should return no matches

**Checkpoint**: Documentation updated, FR-004/FR-005/FR-007 satisfied (SC-004)

---

## Phase 5: Polish & Cross-Cutting Concerns

**Purpose**: Final verification across all success criteria

- [x] T013 Final verification: run `make clean && make && make installcheck`
- [x] T014 Verify index optimization preserved: run `EXPLAIN` query showing Index Scan still used (SC-003)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - verify baseline first
- **User Story 1 (Phase 3)**: Depends on Setup - core code changes
- **User Story 2 (Phase 4)**: Can run in parallel with final verification of US1
- **Polish (Phase 5)**: Depends on US1 and US2 completion

### Within User Story 1

Tasks must be executed sequentially (T002→T003→T004→T005→T006→T007→T008) because:
- All edits are in the same file
- T006 (handler removal) is the critical change
- T007/T008 verify the changes work

### Within User Story 2

- T009, T010, T011 can run in parallel (different files)
- T012 must run after all documentation updates

---

## Parallel Opportunities

```bash
# After US1 code changes verified (T008), launch all doc updates in parallel:
Task T009: "Update num2int_support docblock in pg_num2int_direct_comp.h"
Task T010: "Update support function mechanism text in doc/api-reference.md"
Task T011: "Update support function mechanism text in doc/operator-reference.md"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete T001: Verify baseline
2. Complete T002-T008: Code changes and verification
3. **STOP and VALIDATE**: `make installcheck` passes
4. Feature is functionally complete at this point

### Full Delivery

1. Complete US1 (code removal)
2. Complete US2 (documentation cleanup)
3. Complete Polish (final verification)
4. All success criteria met

---

## Success Criteria Mapping

| Criterion | Verified By |
|-----------|-------------|
| SC-001: 100% regression tests pass | T008, T013 |
| SC-002: ~100 lines removed | T006 |
| SC-003: Index Scan preserved | T014 |
| SC-004: Zero doc references | T012 |

---

## Notes

- This is a code removal feature - no new code to write
- All edits specified in [contracts/file-changes.md](contracts/file-changes.md)
- Commit after each task or logical group
- T006 is the critical task - ~100 lines of handler removal
- Existing regression tests validate no functionality loss
