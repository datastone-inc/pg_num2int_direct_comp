---
name: doc-example-reviewer
description: Verify constitution compliance for PostgreSQL extension documentation testing requirements. Ensures every SQL example in user documentation has a corresponding regression test and vice versa.
---

# doc-example-reviewer Skill

## Purpose

Verify constitution compliance for PostgreSQL extension documentation testing requirements. Ensures every SQL example in user documentation has a corresponding regression test and vice versa.

## When to Use

- User says "review doc example coverage", "check doc test compliance", "verify constitution compliance"
- During code review before commits
- As part of CI/CD checks
- After running doc-example-tester to verify completeness

## Constitution Rule Being Enforced

From the project constitution:

```markdown
SQL code blocks (```sql) in user documentation MUST have corresponding regression
tests in `sql/doc_examples.sql`. Each test MUST include a comment referencing the
documentation location it validates.
```

## Core Workflow

### 1. Scan Documentation

Use bash to find all examples in documentation:

```bash
#!/bin/bash

# Find all markdown files in docs
DOC_FILES="README.md $(find doc -name '*.md' 2>/dev/null)"

# Extract examples with line numbers
for file in $DOC_FILES; do
  # Find SQL code blocks with -- Example: marker
  # Look for ```sql followed by -- Example: within first 3 lines
  awk '
    /^```sql/ { in_block=1; block_start=NR; lines_in_block=0; has_example=0; next }
    in_block && lines_in_block < 3 && /^-- Example:/ {
      has_example=1
      match($0, /^-- Example: *(.*)/, arr)
      description = arr[1]
    }
    in_block { lines_in_block++ }
    /^```$/ && in_block {
      if (has_example) {
        print FILENAME ":" block_start ":" description
      }
      in_block=0
    }
  ' "$file"
done > /tmp/doc_examples.txt
```

Output format: `filename:line_number:description`

### 2. Scan Test File

Parse `sql/doc_examples.sql` for test block headers:

```bash
#!/bin/bash

# Extract test block references
grep -n "^-- .*: .* (~line [0-9]\+)$" sql/doc_examples.sql | while IFS=: read line_num line_text; do
  # Parse: "-- README.md: Basic lookup (~line 120)"
  if [[ "$line_text" =~ ^--[[:space:]](.+):[[:space:]](.+)[[:space:]]\(~line[[:space:]]([0-9]+)\)$ ]]; then
    file="${BASH_REMATCH[1]}"
    desc="${BASH_REMATCH[2]}"
    ref_line="${BASH_REMATCH[3]}"
    echo "$file:$ref_line:$desc:$line_num"
  fi
done > /tmp/test_blocks.txt
```

Output format: `filename:referenced_line:description:test_file_line`

### 3. Cross-Reference and Report

Compare the two files to find issues:

```bash
#!/bin/bash

echo "❌ MISSING TESTS (constitution violation):"
echo

# Find examples in docs without tests
while IFS=: read doc_file doc_line doc_desc; do
  # Check if this example has a test
  if ! grep -q "^${doc_file}:${doc_line}:" /tmp/test_blocks.txt; then
    echo "$(( ++missing_count )). $doc_file line $doc_line: \"$doc_desc\""
    echo "   → No corresponding test in sql/doc_examples.sql"
    echo
  fi
done < /tmp/doc_examples.txt

echo "⚠️ ORPHANED TESTS (may need cleanup):"
echo

# Find tests without corresponding doc examples
while IFS=: read test_file test_ref_line test_desc test_line_num; do
  # Check if doc still has this example (with ±20 line fuzzy match)
  found=0
  while IFS=: read doc_file doc_line doc_desc; do
    if [[ "$doc_file" == "$test_file" ]]; then
      line_diff=$((doc_line - test_ref_line))
      line_diff=${line_diff#-}  # absolute value
      if [[ $line_diff -le 5 ]]; then
        found=1
        break
      elif [[ $line_diff -le 20 ]]; then
        # Stale reference
        echo "⚠️ STALE: Test at line $test_line_num claims $test_file:$test_ref_line"
        echo "   → Actual example at line $doc_line (off by $line_diff lines)"
        echo
        found=1
        break
      fi
    fi
  done < /tmp/doc_examples.txt

  if [[ $found -eq 0 ]]; then
    echo "$(( ++orphaned_count )). sql/doc_examples.sql line $test_line_num: References \"$test_file line $test_ref_line\""
    echo "   → No example found at that location"
    echo
  fi
done < /tmp/test_blocks.txt
```

Generate compliance report with three sections:

#### Missing Tests (Constitution Violations)

Examples in docs without tests:

```text
❌ MISSING TESTS (constitution violation):

1. doc/features.md line 67: "Advanced filtering"
   → No corresponding test in sql/doc_examples.sql

2. README.md line 340: "Transaction handling"
   → No corresponding test in sql/doc_examples.sql

Action: Run doc-example-tester to generate missing tests
```

#### Orphaned Tests (Maintenance Issues)

Tests without corresponding doc examples:

```text
⚠️ ORPHANED TESTS (may need cleanup):

1. sql/doc_examples.sql line 245: References "README.md line 890"
   → No example found at that location
   → Possible causes: example removed, line numbers shifted significantly

2. sql/doc_examples.sql line 312: References "doc/old_api.md line 12"
   → File doc/old_api.md does not exist

Action: Review and remove obsolete tests, or update references
```

#### Stale Line References (Needs Update)

Tests where line numbers are significantly off:

```text
⚠️ STALE LINE REFERENCES (need update):

1. sql/doc_examples.sql line 45: Claims "README.md line 120"
   → Actual example found at line 145 (off by 25 lines)

2. sql/doc_examples.sql line 78: Claims "doc/features.md line 67"
   → Actual example found at line 90 (off by 23 lines)

Action: Run doc-example-tester to update line number references
```

### 4. Summary Statistics

```text
CONSTITUTION COMPLIANCE SUMMARY
===============================
✓ Examples with tests: 12 / 15 (80%)
✗ Missing tests: 3
⚠️ Orphaned tests: 2
⚠️ Stale references: 2

STATUS: ❌ NOT COMPLIANT
Action: Fix missing tests before commit
```

## Configuration

### Thresholds

- **Stale line threshold**: 20 lines (default)
  - If actual line differs by >20 from referenced line → flag as stale
  - Configurable: ask user on first use

- **File patterns to scan**:
  - Include: README.md, doc/*.md
  - Exclude: .github/, internal/, test/
  - Configurable via .doc-review-config.json (optional)

### Strictness Levels

Ask user on first invocation:

- **Strict** (default): Any missing test = failure
- **Warning**: Report missing tests but don't fail
- **Advisory**: Just show stats, no enforcement

## Implementation Details

### Example Marker Detection

Parse SQL blocks in markdown:

~~~markdown
```sql
-- Example: Description here
-- [Without extension (stock PostgreSQL):]
SELECT ...;
```

~~~

Requirements:

- Must have `-- Example:` as first or second line
- Must have actual SQL code (not just comments)
- Line count: at least 3 lines (marker + code + result)

### Test Header Parsing

Regular expression for test headers:

```regex
^-- =+$
^-- (.+?): (.+?) \(~line (\d+)\)$
^-- =+$
```

Capture groups:

1. File path
2. Description
3. Line number

### Line Number Fuzzy Matching

When checking if test references valid example:

1. Look for exact match at referenced line ±5
2. If not found, scan ±20 lines
3. If still not found → orphaned test
4. If found but >20 lines away → stale reference

### Description Matching

Don't require exact description match between doc and test:

- Match by file + line number (primary)
- Description is informational only
- Warn if descriptions differ significantly but locations match

## Error Handling

### File Not Found

- If sql/doc_examples.sql missing → report "No test file found. Run doc-example-tester first."
- If referenced doc file missing → orphaned test

### Parse Errors

- Malformed test headers → report and skip that test
- Malformed markdown → report line number and skip that example

### Ambiguous Cases

- Multiple examples at similar line numbers → use closest match
- Example moved between files → detect by description similarity and report

## Examples

### Example 1: Clean Repository

```text
Scanning documentation examples...
  ✓ README.md: 8 examples found
  ✓ doc/features.md: 4 examples found

Scanning test file...
  ✓ sql/doc_examples.sql: 12 test blocks found

Cross-referencing...
  ✓ All 12 examples have corresponding tests
  ✓ All 12 tests reference valid examples
  ✓ All line numbers accurate (within 5 lines)

CONSTITUTION COMPLIANCE SUMMARY
===============================
✓ Examples with tests: 12 / 12 (100%)
✗ Missing tests: 0
⚠️ Orphaned tests: 0
⚠️ Stale references: 0

STATUS: ✅ FULLY COMPLIANT
```

### Example 2: Issues Found

```text
Scanning documentation examples...
  ✓ README.md: 10 examples found
  ⚠️ doc/api.md: 3 examples found, 1 missing test

Scanning test file...
  ✓ sql/doc_examples.sql: 15 test blocks found
  ⚠️ 3 tests reference non-existent examples

Cross-referencing...

❌ MISSING TESTS (constitution violation):

1. doc/api.md line 234: "Custom aggregate functions"
   Example added in recent commit but no test created

⚠️ ORPHANED TESTS:

1. sql/doc_examples.sql line 145: References "README.md line 456"
   Example removed in commit a3f9c21

2. sql/doc_examples.sql line 189: References "doc/old_features.md line 89"
   File renamed to doc/features.md

3. sql/doc_examples.sql line 234: References "README.md line 890"
   Example moved to doc/api.md line 123

⚠️ STALE LINE REFERENCES:

1. sql/doc_examples.sql line 45: Claims "README.md line 120"
   Actual location: README.md line 148 (off by 28 lines)
   Likely cause: Large section added above this example

CONSTITUTION COMPLIANCE SUMMARY
===============================
✓ Examples with tests: 12 / 13 (92%)
✗ Missing tests: 1
⚠️ Orphaned tests: 3
⚠️ Stale references: 1

STATUS: ❌ NOT COMPLIANT

RECOMMENDED ACTIONS:
1. Run: doc-example-tester to generate missing test
2. Review orphaned tests and remove or update references
3. Run: doc-example-tester to update stale line numbers
```

## User Interaction

### Initial Invocation

When user says "review doc example coverage":

1. "Scanning for documentation examples..."
2. Show progress for each file
3. Show summary with issues
4. If issues found: "Would you like me to run doc-example-tester to fix these?"

### Integration with Git Workflow

Suggest adding to pre-commit hook:

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Check if any .md files changed
if git diff --cached --name-only | grep -q '\.md$'; then
  echo "Documentation changed, checking example coverage..."
  # Run reviewer (would call Claude with this skill)
  if [ $? -ne 0 ]; then
    echo "❌ Doc example coverage check failed"
    echo "Run 'doc-example-tester' to fix issues"
    exit 1
  fi
fi
```

### CI/CD Integration

Suggest GitHub Action:

```yaml
name: Doc Example Coverage
on: [pull_request]
jobs:
  check-coverage:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Check doc example coverage
        run: |
          # Would call Claude with doc-example-reviewer skill
          # Exit 1 if not compliant
```

## Output Formats

### Console (default)

Colorized, human-readable report as shown in examples above

### JSON (for CI/CD)

```json
{
  "compliant": false,
  "total_examples": 13,
  "tested_examples": 12,
  "missing_tests": [
    {
      "file": "doc/api.md",
      "line": 234,
      "description": "Custom aggregate functions"
    }
  ],
  "orphaned_tests": [
    {
      "test_file_line": 145,
      "references": "README.md line 456",
      "reason": "example_not_found"
    }
  ],
  "stale_references": [
    {
      "test_file_line": 45,
      "claimed_location": "README.md line 120",
      "actual_location": "README.md line 148",
      "drift": 28
    }
  ]
}
```

### Markdown (for PR comments)

````markdown
## Documentation Example Coverage Report

### ❌ Status: NOT COMPLIANT

**Summary:**
- ✓ Examples with tests: 12 / 13 (92%)
- ✗ Missing tests: 1
- ⚠️ Orphaned tests: 3
- ⚠️ Stale references: 1

### Missing Tests (Constitution Violation)

| File | Line | Description |
|------|------|-------------|
| doc/api.md | 234 | Custom aggregate functions |

### Recommended Actions

1. Run `doc-example-tester` to generate missing tests
2. Review and update orphaned test references
3. Commit updated `sql/doc_examples.sql`
````

## Success Criteria

The skill succeeds when it:

1. Accurately detects all `-- Example:` blocks in documentation
2. Correctly parses all test block headers in sql/doc_examples.sql
3. Reports missing, orphaned, and stale tests with actionable information
4. Provides clear pass/fail status for constitution compliance
5. Integrates smoothly with doc-example-tester for remediation

## Relationship to doc-example-tester

These skills work together:

1. **doc-example-reviewer** identifies issues (this skill)
2. **doc-example-tester** fixes issues (generator/validator)

Typical workflow:

```bash
# Before commit
doc-example-reviewer   # Check compliance
doc-example-tester     # Fix issues
doc-example-reviewer   # Verify fixed
git commit
```

The reviewer is fast (just file parsing) while the tester is slower (runs actual tests).
