# Feature Specification: Remove Redundant SupportRequestIndexCondition Handler

**Feature Branch**: `003-remove-index-condition`  
**Created**: 2026-01-12  
**Status**: Draft  
**Input**: User description: "Remove redundant SupportRequestIndexCondition handler since SupportRequestSimplify subsumes its functionality"

## Background

During analysis of the `pg_num2int_direct_comp` extension, we discovered that the `SupportRequestIndexCondition` handler is redundant. Both handlers:

1. Require identical preconditions: `Var op Const` or `Const op Var` pattern
2. Perform the same transformation: convert numeric/float constant to integer, build native operator
3. Produce the same output: a native integer comparison expression

Since `SupportRequestSimplify` runs earlier in query planning (during expression simplification) and transforms the expression tree directly to native operators, the planner automatically uses btree indexes on the transformed expressions. The `SupportRequestIndexCondition` handler adds no value.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Maintainer Simplifies Codebase (Priority: P1)

As an extension maintainer, I want to remove dead code paths so that the codebase is easier to understand, test, and maintain.

**Why this priority**: Reduces maintenance burden and cognitive load. Removing 100+ lines of redundant code directly improves code quality.

**Independent Test**: After removal, all existing regression tests pass, demonstrating no functionality was lost.

**Acceptance Scenarios**:

1. **Given** the extension with `SupportRequestIndexCondition` removed, **When** running `make installcheck`, **Then** all regression tests pass
2. **Given** the extension with `SupportRequestIndexCondition` removed, **When** running `EXPLAIN` on cross-type comparisons with constants, **Then** index scans are used (same as before)

---

### User Story 2 - Developer Understands Support Function (Priority: P2)

As a developer reading the code, I want the support function to have a single clear transformation path so that I can understand how query optimization works without confusion.

**Why this priority**: Code clarity enables faster onboarding and reduces bugs during future changes.

**Independent Test**: Code review confirms single transformation handler with clear documentation.

**Acceptance Scenarios**:

1. **Given** a developer reading `num2int_support()`, **When** reviewing the support function logic, **Then** there is only one transformation path (`SupportRequestSimplify`) with clear documentation explaining why
2. **Given** the documentation, **When** reading about support functions, **Then** there is no mention of `SupportRequestIndexCondition` as an active mechanism

---

### Edge Cases

- What happens when support functions are disabled via GUC? The extension should continue to work without index optimization (same behavior as before).
- What happens with non-constant predicates (e.g., `int_col = numeric_col`)? These bypass both handlers and use the operator directly (unchanged behavior).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The `num2int_support()` function MUST NOT handle `SupportRequestIndexCondition`
- **FR-002**: The `num2int_support()` function MUST continue to handle `SupportRequestSimplify` with identical transformation logic
- **FR-003**: All existing index optimization behavior MUST be preserved (verified by unchanged regression test results)
- **FR-004**: Documentation MUST be updated to remove references to `SupportRequestIndexCondition` as an active optimization mechanism
- **FR-005**: Code comments in the header file MUST be updated to reflect the simplified architecture
- **FR-006**: The GUC description MUST reference only `SupportRequestSimplify` (no historical note about removed handler)
- **FR-007**: All non-archival files MUST have zero references to `SupportRequestIndexCondition`; only specs/003-* and specs/001-*/research.md may retain historical references

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: 100% of existing regression tests pass after removal (no functionality regression)
- **SC-002**: Lines of code reduced by removing ~100 lines of redundant handler code
- **SC-003**: `EXPLAIN` output for `WHERE int_col = 100::numeric` continues to show Index Scan (index optimization preserved)
- **SC-004**: Documentation references to `SupportRequestIndexCondition` reduced to zero in user-facing docs

## Assumptions

- The analysis that `SupportRequestSimplify` subsumes `SupportRequestIndexCondition` is correct for all supported operator combinations
- No external tooling or monitoring depends on the specific code path used for index optimization
- The GUC `pg_num2int_direct_comp.enableSupportFunctions` will continue to control `SupportRequestSimplify` behavior

## Clarifications

### Session 2026-01-12

- Q: Should the GUC description mention that `SupportRequestIndexCondition` was removed, or simply describe only the remaining `SupportRequestSimplify` behavior? → A: Clean slate - reference only `SupportRequestSimplify`