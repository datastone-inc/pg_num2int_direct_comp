# Research: Remove Redundant SupportRequestIndexCondition Handler

**Feature**: 003-remove-index-condition  
**Created**: 2026-01-12  
**Purpose**: Document findings that led to the decision to remove `SupportRequestIndexCondition`

## 1. Discovery Summary

During code review of the `num2int_support()` function, we analyzed both support request handlers and discovered they are functionally equivalent for all supported use cases.

### 1.1 Key Finding

**`SupportRequestSimplify` subsumes `SupportRequestIndexCondition`** because:

1. **Identical preconditions**: Both handlers require the same pattern: `Var op Const` or `Const op Var`
2. **Identical transformations**: Both convert the numeric/float constant to an integer and build a native operator
3. **Identical outputs**: Both produce a native integer comparison expression
4. **Timing advantage**: Simplify runs earlier (expression simplification phase), allowing transformed native operators to naturally use indexes

### 1.2 Code Evidence

Both handlers have identical pattern matching:

```c
// SupportRequestSimplify handler
if (IsA(leftop, Const) && IsA(rightop, Var)) {
  constNode = (Const *) leftop;
  varNode = (Var *) rightop;
} else if (IsA(leftop, Var) && IsA(rightop, Const)) {
  varNode = (Var *) leftop;
  constNode = (Const *) rightop;
} else {
  PG_RETURN_POINTER(NULL);  // Neither pattern matched
}

// SupportRequestIndexCondition handler - IDENTICAL CODE
if (IsA(leftop, Const) && IsA(rightop, Var)) {
  constNode = (Const *) leftop;
  varNode = (Var *) rightop;
} else if (IsA(leftop, Var) && IsA(rightop, Const)) {
  varNode = (Var *) leftop;
  constNode = (Const *) rightop;
} else {
  PG_RETURN_POINTER(NULL);
}
```

## 2. Why IndexCondition Is Redundant

### 2.1 Purpose of Each Handler

| Handler | Purpose | When Called |
|---------|---------|-------------|
| `SupportRequestSimplify` | Transform expression tree during constant folding | Early in planning (expression preprocessing) |
| `SupportRequestIndexCondition` | Provide index-compatible condition | During index path generation |

### 2.2 Why Simplify Makes IndexCondition Unnecessary

When `SupportRequestSimplify` transforms `int_col = 10.0::numeric` to `int_col = 10`:

1. The resulting expression uses native `int4 = int4` operator (OID 96)
2. The planner already knows how to use btree indexes with native integer operators
3. No additional index condition hint is needed

**Flow with Simplify only**:
```
Original: int_col = 10.0::numeric
  ↓ (SupportRequestSimplify)
Transformed: int_col = 10
  ↓ (Standard planner)
Index Scan using int_col_idx (native int4 = int4)
```

**Flow with IndexCondition (if Simplify didn't fire)**:
```
Original: int_col = 10.0::numeric
  ↓ (SupportRequestIndexCondition)
Index Condition: int_col = 10
  ↓ (Planner uses provided condition)
Index Scan using int_col_idx
```

Since Simplify fires first and has identical preconditions, IndexCondition never gets a case that Simplify couldn't handle.

### 2.3 Theoretical Use Case for IndexCondition

`SupportRequestIndexCondition` would only add value if it could handle cases Simplify cannot:

| Scenario | Simplify Can Handle? | IndexCondition Needed? |
|----------|---------------------|------------------------|
| `int_col = 10.0::numeric` (constant) | ✅ Yes | ❌ No |
| `int_col = $1::numeric` (parameter) | ❌ No (not a Const) | ❌ No (same limitation) |
| `int_col = other_col::numeric` (join) | ❌ No (not a Const) | ❌ No (same limitation) |
| Lossy index conditions | N/A | Possible but unused |

Both handlers have identical limitations. IndexCondition provides no additional coverage.

### 2.4 Lossy Index Conditions

`SupportRequestIndexCondition` could theoretically provide lossy index conditions (return a superset of matching rows, with recheck). For example, for `int_col = 10.5::numeric`:

```c
// Lossy: return range scan with recheck
req->lossy = true;
// Return: int_col >= 10 AND int_col <= 11
```

However, this is not useful because:
1. `SupportRequestSimplify` correctly transforms fractional equality to `FALSE`
2. The planner can then eliminate the predicate entirely
3. A lossy index scan would be slower than knowing the result is empty

## 3. Prior Research Reference

This finding confirms and extends the analysis in [research.md Section 9](../001-num-int-direct-comp/research.md#9-supportrequestsimplify-vs-supportrequestindexcondition), which recommended:

> "Implement `SupportRequestSimplify` as the primary mechanism for constant predicate optimization."

And noted:

> "Remove SupportRequestIndexCondition (optional): Redundant when Simplify handles constants"

## 4. Impact Analysis

### 4.1 Code to Remove

From `pg_num2int_direct_comp.c`:
- ~100 lines of `SupportRequestIndexCondition` handler code (lines 1005-1107)

### 4.2 Code to Update

**Header file** (`pg_num2int_direct_comp.h`):
- Update file header comment describing support function behavior

**Documentation** (`doc/api-reference.md`, `doc/operator-reference.md`):
- Remove references to `SupportRequestIndexCondition`
- Update support function description to only mention `SupportRequestSimplify`

**GUC description** (in `_PG_init()`):
- Update description to only reference `SupportRequestSimplify`

### 4.3 Regression Risk

**Low risk** because:
1. `SupportRequestSimplify` already handles all cases IndexCondition handled
2. All existing regression tests will continue to pass
3. The transformation produces identical output expressions
4. Index usage is preserved (verified by EXPLAIN tests)

## 5. Recommendation

**Remove `SupportRequestIndexCondition` handler** from `num2int_support()` function:

1. Reduces code complexity (~100 fewer lines)
2. Eliminates dead code path that never fires
3. Simplifies future maintenance
4. No functionality loss

The GUC `pg_num2int_direct_comp.enableSupportFunctions` should continue to control `SupportRequestSimplify` behavior.
