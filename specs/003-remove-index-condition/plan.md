# Implementation Plan: Remove Redundant SupportRequestIndexCondition Handler

**Branch**: `003-remove-index-condition` | **Date**: 2026-01-12 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-remove-index-condition/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/commands/plan.md` for the execution workflow.

## Summary

Remove the redundant `SupportRequestIndexCondition` handler from `num2int_support()` function. The handler is unnecessary because `SupportRequestSimplify` already transforms cross-type comparisons to native integer operators during expression simplification, enabling automatic index usage through standard planner mechanisms.

## Technical Context

**Language/Version**: C (PostgreSQL 12+ backend extension)  
**Primary Dependencies**: PostgreSQL PGXS build system, PostgreSQL support function API  
**Storage**: N/A (pure code cleanup, no schema changes)  
**Testing**: PostgreSQL pg_regress framework (`make installcheck`)  
**Target Platform**: Linux/macOS PostgreSQL servers  
**Project Type**: Single project (PostgreSQL extension)  
**Performance Goals**: No performance impact (code removal only)  
**Constraints**: Must maintain 100% regression test pass rate  
**Scale/Scope**: ~100 lines of code removal, 4 documentation files updated

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate | Status | Notes |
|------|--------|-------|
| C code follows K&R style with 2-space indent | ✅ PASS | Removal only, no new code |
| C++ forbidden in backend code | ✅ PASS | C only |
| Function documentation includes @brief, @param, @return | ✅ PASS | Will update num2int_support docblock |
| TDD using pg_regress | ✅ PASS | Existing tests verify no regression |
| Extension lifecycle tests | ✅ PASS | Not affected by this change |
| All SQL examples have regression tests | ✅ PASS | No new examples |
| PGXS build system | ✅ PASS | No Makefile changes |

**Pre-design gate status**: PASS - Proceed to Phase 0

## Project Structure

### Documentation (this feature)

```text
specs/003-remove-index-condition/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Prior research (already complete)
├── data-model.md        # Phase 1 output (minimal - no data model changes)
├── quickstart.md        # Phase 1 output (implementation guide)
├── contracts/           # Phase 1 output (file change contracts)
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (repository root)

```text
# PostgreSQL Extension (PGXS structure)
pg_num2int_direct_comp.c        # Main source - SupportRequestIndexCondition handler removal
pg_num2int_direct_comp.h        # Header - update documentation comments
pg_num2int_direct_comp.control  # Extension control (unchanged)
pg_num2int_direct_comp--1.0.0.sql  # SQL definitions (unchanged)

doc/
├── api-reference.md            # Update support function documentation
└── operator-reference.md       # Update support function documentation

sql/                            # Regression tests (unchanged, verify behavior)
expected/                       # Expected outputs (unchanged)
```

**Structure Decision**: This is a pure refactoring/cleanup change. Only the main C source file and documentation require modifications. No schema changes, no new tests (existing tests verify no regression).

## Complexity Tracking

> No violations - this is a simplification that reduces complexity.

| Metric | Before | After |
|--------|--------|-------|
| Lines in num2int_support() | ~270 | ~170 |
| Support request handlers | 2 | 1 |
| Documentation references | Multiple | Single clear mechanism |

## Post-Design Constitution Re-Check

*GATE: Re-evaluated after Phase 1 design completion.*

| Gate | Status | Notes |
|------|--------|-------|
| C code follows K&R style with 2-space indent | ✅ PASS | No new code, only removal |
| C++ forbidden in backend code | ✅ PASS | C only |
| Function documentation includes @brief, @param, @return | ✅ PASS | Updated docblocks specified in contracts |
| TDD using pg_regress | ✅ PASS | Existing tests verify behavior unchanged |
| Extension lifecycle tests | ✅ PASS | No schema changes |
| All SQL examples have regression tests | ✅ PASS | No new examples added |
| PGXS build system | ✅ PASS | Makefile unchanged |
| FR-007: Zero refs to IndexCondition outside specs | ✅ PASS | Contracts remove all references |

**Post-design gate status**: PASS - Ready for `/speckit.tasks`
