# Developer Quick Start: Float-Integer Btree Operators

**Feature**: 002-float-btree-ops

## What's Changing

Adding int x float operators to btree operator families to enable **merge joins** between integer and float columns.

### Before (v1.0.0)
- Int x float operators support: indexed nested loop, hash join
- Merge join: NOT available (operators not in btree families)

### After (this feature)
- Int x float operators support: indexed nested loop, hash join, **merge join**
- Merge join: Available when both sides have btree indexes

## Files Modified

| File | Change |
|------|--------|
| `pg_num2int_direct_comp--1.0.0.sql` | Add ALTER OPERATOR FAMILY statements |
| `sql/float_int_ops.sql` | New regression test for btree families |
| `sql/merge_joins.sql` | Extend for float x int merge join tests |
| `expected/*.out` | Update expected outputs |
| `README.md` | Shorten, add persuasive framing |
| `doc/api-reference.md` | Add float btree info |
| `doc/operator-reference.md` | Update operator tables |

## Testing Locally

```bash
# Rebuild extension
make clean && make

# Reinstall (required since no upgrade script)
psql -d pg_num2int_test -c "DROP EXTENSION IF EXISTS pg_num2int_direct_comp CASCADE"
psql -d pg_num2int_test -c "CREATE EXTENSION pg_num2int_direct_comp"

# Run specific regression test
make installcheck REGRESS=float_int_ops

# Run all tests
make installcheck
```

## Verify Merge Join Works

```sql
-- Create test tables
CREATE TABLE int_test (id SERIAL PRIMARY KEY, val int4);
CREATE TABLE float_test (id SERIAL PRIMARY KEY, val float4);
CREATE INDEX ON int_test(val);
CREATE INDEX ON float_test(val);

INSERT INTO int_test (val) SELECT generate_series(1, 1000);
INSERT INTO float_test (val) SELECT generate_series(1, 1000)::float4;

-- Force merge join
SET enable_hashjoin = off;
SET enable_nestloop = off;

-- Should show Merge Join
EXPLAIN (COSTS OFF)
SELECT * FROM int_test i JOIN float_test f ON i.val = f.val;
```

## SQL Pattern for Btree Family Registration

```sql
-- Add operators to float4_ops btree family
ALTER OPERATOR FAMILY float4_ops USING btree ADD
  -- float4 x int2
  FUNCTION 1 (float4, int2) float4_cmp_int2(float4, int2),
  OPERATOR 1 < (float4, int2),
  OPERATOR 2 <= (float4, int2),
  OPERATOR 3 = (float4, int2),
  OPERATOR 4 >= (float4, int2),
  OPERATOR 5 > (float4, int2),
  
  -- int2 x float4 (reverse direction)
  FUNCTION 1 (int2, float4) int2_cmp_float4(int2, float4),
  OPERATOR 1 < (int2, float4),
  OPERATOR 2 <= (int2, float4),
  OPERATOR 3 = (int2, float4),
  OPERATOR 4 >= (int2, float4),
  OPERATOR 5 > (int2, float4);
```

## Event Trigger Cleanup Pattern

Add to `pg_num2int_direct_comp_cleanup()` ops array:

```sql
-- float4_ops btree cleanup
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 1 (float4, int2)',
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 2 (float4, int2)',
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 3 (float4, int2)',
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 4 (float4, int2)',
'ALTER OPERATOR FAMILY float4_ops USING btree DROP OPERATOR 5 (float4, int2)',
'ALTER OPERATOR FAMILY float4_ops USING btree DROP FUNCTION 1 (float4, int2)',
-- ... repeat for each type pair
```

## Common Issues

### "operator already exists in operator family"
- Event trigger cleanup didn't run on previous DROP
- Manual fix: Run the DROP statements from the cleanup function

### Merge join not selected
- Check `enable_mergejoin` is ON
- Verify operators are in btree families: query pg_amop
- Ensure both tables have btree indexes on join columns

### Test failures after changes
- Regenerate expected output: `make installcheck` then copy results to expected/
- Or update the test if behavior changed intentionally
