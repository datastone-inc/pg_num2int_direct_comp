# Quickstart: Remove SupportRequestIndexCondition

**Feature**: 003-remove-index-condition  
**Estimated Time**: 30 minutes  
**Prerequisites**: Working PostgreSQL development environment with extension installed

## Overview

This is a code cleanup feature that removes redundant code. The `SupportRequestIndexCondition` handler in `num2int_support()` is unnecessary because `SupportRequestSimplify` already handles all the same cases and runs earlier in query planning.

## Implementation Steps

### Step 1: Verify Current Test State (5 min)

Before making changes, ensure all tests pass:

```bash
make clean && make && make installcheck
```

All tests should pass. This establishes the baseline.

### Step 2: Update File Header Comment (1 min)

In [pg_num2int_direct_comp.c](../../pg_num2int_direct_comp.c), line 20:

Remove `and SupportRequestIndexCondition` from the header description.

### Step 3: Update GUC Description (2 min)

In [pg_num2int_direct_comp.c](../../pg_num2int_direct_comp.c), around line 411:

Update the `DefineCustomBoolVariable` call to reference only `SupportRequestSimplify`.

### Step 4: Update Helper Section Comment (1 min)

In [pg_num2int_direct_comp.c](../../pg_num2int_direct_comp.c), around line 612:

Update the comment to indicate helpers are used by `SupportRequestSimplify` only.

### Step 5: Update num2int_support Docblock (2 min)

In [pg_num2int_direct_comp.c](../../pg_num2int_direct_comp.c), around line 983:

Replace the function docblock to describe only `SupportRequestSimplify` behavior.

### Step 6: Remove SupportRequestIndexCondition Handler (5 min)

In [pg_num2int_direct_comp.c](../../pg_num2int_direct_comp.c), around lines 1003-1107:

1. Delete the entire `if (IsA(rawreq, SupportRequestIndexCondition))` block
2. Change `else if (IsA(rawreq, SupportRequestSimplify))` to `if (IsA(rawreq, SupportRequestSimplify))`

### Step 7: Update Header File Docblock (2 min)

In [pg_num2int_direct_comp.h](../../pg_num2int_direct_comp.h), around line 388:

Update the `num2int_support` docblock to describe only `SupportRequestSimplify`.

### Step 8: Update Documentation (5 min)

Update both documentation files:

- [doc/api-reference.md](../../doc/api-reference.md) line 184
- [doc/operator-reference.md](../../doc/operator-reference.md) line 178

Change mechanism description to reference only `SupportRequestSimplify`.

### Step 9: Verify Changes (5 min)

1. **Compile**: `make clean && make`
2. **Test**: `make installcheck` - all tests should still pass
3. **Verify no stray references**:
   ```bash
   grep -r "SupportRequestIndexCondition" . --include="*.c" --include="*.h" --include="*.md" | grep -v "specs/"
   ```
   Should return no matches.

## Success Criteria

- [ ] `make installcheck` passes (100% of regression tests)
- [ ] Code compiles without warnings
- [ ] No references to `SupportRequestIndexCondition` outside `specs/` directory
- [ ] ~100 lines of code removed from main source file

## Rollback

If issues are discovered, revert all changes:

```bash
git checkout -- pg_num2int_direct_comp.c pg_num2int_direct_comp.h doc/
```

## Notes

- The SupportRequestSimplify handler code is unchanged - only the wrapper and documentation change
- Index optimization continues to work because transformed expressions use native operators
- The GUC `enableSupportFunctions` continues to work - it now controls only `SupportRequestSimplify`
