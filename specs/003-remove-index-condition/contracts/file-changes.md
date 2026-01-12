# Contracts: Remove SupportRequestIndexCondition

**Feature**: 003-remove-index-condition  
**Created**: 2026-01-12

This document specifies the exact changes required for each file.

## Contract 1: pg_num2int_direct_comp.c

### C1.1: File Header Comment (Line 20)

**Current**:
```c
 * support btree index optimization via SupportRequestSimplify and
 * SupportRequestIndexCondition.
```

**Replace with**:
```c
 * support btree index optimization via SupportRequestSimplify.
```

### C1.2: GUC Description (Lines 411-415)

**Current**:
```c
    DefineCustomBoolVariable("pg_num2int_direct_comp.enableSupportFunctions",
                           "Enable SupportRequestSimplify and SupportRequestIndexCondition optimizations",
                           "When enabled, allows the extension to optimize queries by transforming "
                           "cross-type comparisons into same-type comparisons. "
                           "Disable for testing or if optimization causes issues.",
```

**Replace with**:
```c
    DefineCustomBoolVariable("pg_num2int_direct_comp.enableSupportFunctions",
                           "Enable SupportRequestSimplify optimization",
                           "When enabled, allows the extension to optimize queries by transforming "
                           "cross-type comparisons into same-type comparisons at plan time. "
                           "Disable for testing or if optimization causes issues.",
```

### C1.3: Helper Section Comment (Lines 609-614)

**Current**:
```c
/*
 * ============================================================================
 * Support Function Helper Types and Functions
 * ============================================================================
 * These helpers are shared between SupportRequestIndexCondition and
 * SupportRequestSimplify handlers to avoid code duplication.
 */
```

**Replace with**:
```c
/*
 * ============================================================================
 * Support Function Helper Types and Functions
 * ============================================================================
 * These helpers are used by the SupportRequestSimplify handler.
 */
```

### C1.4: num2int_support Docblock (Lines 983-993)

**Current**:
```c
/**
 * @brief Generic support function for query optimization
 * @param fcinfo Function call context
 * @return Node pointer or NULL
 *
 * Handles two request types:
 * - SupportRequestIndexCondition: Transforms predicates for btree index scans
 * - SupportRequestSimplify: Simplifies constant predicates (FR-015, FR-016, FR-017)
 *   - FR-015: Equality with fractional value → FALSE
 *   - FR-016: Equality with exact integer → native operator
 *   - FR-017: Range with fraction → adjusted boundary
 */
```

**Replace with**:
```c
/**
 * @brief Support function for query optimization via SupportRequestSimplify
 * @param fcinfo Function call context
 * @return Node pointer for simplified expression or NULL
 *
 * Simplifies constant predicates during query planning:
 * - Equality with fractional value → FALSE (no integer can equal a fraction)
 * - Equality with exact integer → native integer operator (enables index use)
 * - Range with fraction → adjusted boundary (e.g., > 10.5 becomes >= 11)
 * - Out-of-range constants → TRUE/FALSE based on operator semantics
 *
 * Index optimization works because transformed expressions use native integer
 * operators which the planner automatically recognizes for btree index scans.
 */
```

### C1.5: Remove SupportRequestIndexCondition Handler (Lines 1003-1107)

**Remove entire block**:
```c
  if (IsA(rawreq, SupportRequestIndexCondition)) {
    ... (entire handler ~100 lines)
  }
  else if (IsA(rawreq, SupportRequestSimplify)) {
```

**Replace with**:
```c
  if (IsA(rawreq, SupportRequestSimplify)) {
```

The SupportRequestSimplify handler body remains unchanged.

---

## Contract 2: pg_num2int_direct_comp.h

### C2.1: num2int_support Docblock (Lines 388-397)

**Current**:
```c
/**
 * @brief Generic support function for all comparison operators
 * @param fcinfo Function call information structure
 * @return Node pointer for index condition or NULL if not supported
 *
 * Implements SupportRequestIndexCondition to enable btree index usage for
 * exact comparison predicates. Inspects the operator OID to determine the
 * type combination being compared, validates that the constant operand is
 * within range, and transforms the predicate for index scanning.
 */
```

**Replace with**:
```c
/**
 * @brief Support function for query optimization via SupportRequestSimplify
 * @param fcinfo Function call information structure
 * @return Node pointer for simplified expression or NULL
 *
 * Implements SupportRequestSimplify to transform cross-type comparisons with
 * constants into native integer comparisons. This enables btree index usage
 * because the planner recognizes native integer operators for index scans.
 */
```

---

## Contract 3: doc/api-reference.md

### C3.1: Support Function Mechanism (Line 184)

**Current**:
```markdown
**Mechanism**: Implements `SupportRequestSimplify` and `SupportRequestIndexCondition` to transform predicates at plan time
```

**Replace with**:
```markdown
**Mechanism**: Implements `SupportRequestSimplify` to transform predicates during query planning
```

---

## Contract 4: doc/operator-reference.md

### C4.1: Support Function Mechanism (Line 178)

**Current**:
```markdown
**Mechanism**: Implements `SupportRequestSimplify` and `SupportRequestIndexCondition` to transform predicates at plan time
```

**Replace with**:
```markdown
**Mechanism**: Implements `SupportRequestSimplify` to transform predicates during query planning
```

---

## Verification

After applying all contracts:

1. Run `make clean && make` - should compile without errors
2. Run `make installcheck` - all regression tests should pass
3. Run `grep -r "SupportRequestIndexCondition" . --include="*.c" --include="*.h" --include="*.md" | grep -v "specs/"` - should return no matches outside specs directory
