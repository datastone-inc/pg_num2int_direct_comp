---
name: doc-example-tester
description: Generate and validate regression tests for PostgreSQL extension documentation examples.
---
# doc-example-tester Skill

## Purpose

Generate and validate regression tests for PostgreSQL extension documentation examples. Ensures all SQL examples in user documentation have corresponding tests in `sql/doc_examples.sql` and that test output matches documented results.

## When to Use

- User says "generate doc example tests", "update doc_examples.sql", "verify doc examples"
- When documentation SQL examples are added or modified
- To validate that documented output matches actual test results

## Core Workflow

### 1. Parse Documentation Examples

Scan README.md and doc/*.md for SQL blocks with this pattern:

```bash
#!/bin/bash

# Process each markdown file
process_markdown_file() {
  local file="$1"

  awk -v filename="$file" '
    BEGIN {
      in_sql=0
      has_example=0
      example_desc=""
      example_line=0
      sql_code=""
      expected_output=""
      without_extension=0
      output_started=0
      output_ended=0
    }

    # Start of SQL block
    /^```sql/ {
      in_sql=1
      sql_block_start=NR
      has_example=0
      example_desc=""
      sql_code=""
      expected_output=""
      without_extension=0
      output_started=0
      output_ended=0
      next
    }

    # Inside SQL block
    in_sql && /^-- Example:/ {
      has_example=1
      match($0, /^-- Example: *(.*)/, arr)
      example_desc = arr[1]
      example_line = sql_block_start
      next
    }

    in_sql && /^-- Without extension \(stock PostgreSQL\):/ {
      without_extension=1
      next
    }

    # Collect SQL code (non-comment lines, non-blank)
    in_sql && !/^--/ && !/^```/ && !/^[[:space:]]*$/ {
      if (sql_code != "") sql_code = sql_code "\n"
      sql_code = sql_code $0
      next
    }

    # Blank line ends expected output collection (comments after are ignored)
    in_sql && /^[[:space:]]*$/ {
      if (output_started) output_ended=1
      next
    }

    # Collect expected output (comment lines, before blank line)
    in_sql && /^--[^-]/ && !output_ended {
      output_started=1
      sub(/^-- ?/, "", $0)
      if (expected_output != "") expected_output = expected_output "\n"
      expected_output = expected_output $0
      next
    }

    # End of SQL block
    /^```$/ && in_sql {
      if (has_example) {
        # Output in structured format for processing
        print "FILE:" filename
        print "LINE:" example_line
        print "DESC:" example_desc
        print "WITH_WITHOUT:" without_extension
        print "SQL_START"
        print sql_code
        print "SQL_END"
        print "OUTPUT_START"
        print expected_output
        print "OUTPUT_END"
        print "---"
      }
      in_sql=0
      next
    }
  ' "$file"
}

# Process all markdown files
for file in README.md doc/*.md; do
  [[ -f "$file" ]] && process_markdown_file "$file"
done > /tmp/parsed_examples.txt
```

Output is structured text with markers for parsing in next step.

### 2. Generate/Update sql/doc_examples.sql

Parse the structured output and generate test blocks:

```bash
#!/bin/bash

# Detect extension name
detect_extension_name() {
  if [[ -f sql/doc_examples.sql ]]; then
    # Extract from existing tests
    grep "CREATE EXTENSION" sql/doc_examples.sql | head -1 | \
      sed 's/.*CREATE EXTENSION \([^ ;]*\).*/\1/'
  else
    # Find .control file
    local control_file=$(ls *.control 2>/dev/null | head -1)
    if [[ -n "$control_file" ]]; then
      basename "$control_file" .control
    else
      echo "UNKNOWN_EXTENSION"
    fi
  fi
}

EXT_NAME=$(detect_extension_name)

# Generate test file
generate_test_file() {
  cat <<'HEADER'
-- ============================================================================
-- Auto-generated documentation example tests
-- DO NOT EDIT - regenerate with doc-example-tester skill
-- ============================================================================

HEADER

  # Parse the structured examples
  local current_section=""
  local file="" line="" desc="" with_without="" sql_code="" expected_output=""

  while IFS= read -r line_text; do
    case "$line_text" in
      FILE:*)
        file="${line_text#FILE:}"
        ;;
      LINE:*)
        line="${line_text#LINE:}"
        ;;
      DESC:*)
        desc="${line_text#DESC:}"
        ;;
      WITH_WITHOUT:*)
        with_without="${line_text#WITH_WITHOUT:}"
        ;;
      SQL_START)
        sql_code=""
        current_section="sql"
        ;;
      SQL_END)
        current_section=""
        ;;
      OUTPUT_START)
        expected_output=""
        current_section="output"
        ;;
      OUTPUT_END)
        current_section=""
        ;;
      ---)
        # End of example - generate test block
        generate_test_block "$file" "$line" "$desc" "$with_without" "$sql_code" "$expected_output"
        file="" line="" desc="" with_without="" sql_code="" expected_output=""
        ;;
      *)
        if [[ "$current_section" == "sql" ]]; then
          sql_code="${sql_code}${line_text}"$'\n'
        elif [[ "$current_section" == "output" ]]; then
          expected_output="${expected_output}${line_text}"$'\n'
        fi
        ;;
    esac
  done < /tmp/parsed_examples.txt
}

generate_test_block() {
  local file="$1" line="$2" desc="$3" with_without="$4" sql="$5" output="$6"

  cat <<EOF

-- ============================================================================
-- ${file}: ${desc} (~line ${line})
-- ============================================================================

EOF

  # Extract setup SQL (CREATE/INSERT statements)
  local setup_sql=$(echo "$sql" | grep -E "^(CREATE|INSERT|ALTER|DROP)" || true)

  if [[ -n "$setup_sql" ]]; then
    echo "-- Setup"
    echo "$setup_sql"
    echo
  fi

  # Generate with/without blocks if needed
  if [[ "$with_without" == "1" ]]; then
    echo "-- Without extension (verifies documented stock PostgreSQL behavior)"
    echo "\\echo '--- Without extension ---'"
    echo "DROP EXTENSION IF EXISTS ${EXT_NAME} CASCADE;"
    echo "$sql"
    echo
    echo "-- With extension (behavior not documented - for completeness only)"
    echo "\\echo '--- With extension ---'"
    echo "CREATE EXTENSION IF NOT EXISTS ${EXT_NAME};"
    echo "$sql"
    echo
  else
    echo "-- With extension"
    echo "\\echo '--- With extension ---'"
    echo "CREATE EXTENSION IF NOT EXISTS ${EXT_NAME};"
    echo "$sql"
    echo
  fi

  # Generate cleanup
  if [[ -n "$setup_sql" ]]; then
    echo "-- Cleanup"
    echo "$setup_sql" | grep "CREATE TABLE" | \
      sed 's/CREATE TABLE \([^ (]*\).*/DROP TABLE IF EXISTS \1 CASCADE;/'
    echo "$setup_sql" | grep "CREATE FOREIGN TABLE" | \
      sed 's/CREATE FOREIGN TABLE \([^ (]*\).*/DROP FOREIGN TABLE IF EXISTS \1 CASCADE;/'
    echo
  fi
}

generate_test_file > sql/doc_examples.sql
```

**IMPORTANT**:

- Preserve existing manual edits by checking git diff before overwriting
- Update line numbers in existing test headers if they've drifted
- Use unique table names (prefix with `doc_ex_`) to avoid conflicts

### 3. Run Regression Tests

Execute:

```bash
cd <extension_root>
make installcheck REGRESS=doc_examples
```

### 4. Validate Results

Parse `results/doc_examples.out` and compare against expected output from docs:

```bash
#!/bin/bash

# Extract sections from test output
parse_test_output() {
  local output_file="results/doc_examples.out"

  awk '
    /^--- Without extension ---/ { section="without"; next }
    /^--- With extension ---/ { section="with"; next }
    /^-- =+$/ { in_header=1; next }
    in_header && /^-- / {
      match($0, /^-- (.+): (.+) \(~line ([0-9]+)\)/, arr)
      file = arr[1]
      desc = arr[2]
      line = arr[3]
      in_header=0
      next
    }
    in_header && /^-- =+$/ {
      in_header=0
      print "BLOCK:" file ":" line ":" desc
      next
    }
    section != "" && /^[A-Z]/ {
      # Start of query output (column headers)
      output = $0
      collecting=1
      next
    }
    collecting && /^\(/ {
      # Row count line - end of output
      output = output "\n" $0
      print "OUTPUT:" section
      print output
      print "END_OUTPUT"
      collecting=0
      section=""
      output=""
      next
    }
    collecting {
      output = output "\n" $0
    }
  ' "$output_file" > /tmp/actual_output.txt
}

# Compare with expected output from docs
compare_outputs() {
  # Re-parse docs to get expected output
  # (reuse parse logic from step 1)

  # For each block, compare actual vs expected
  local mismatches=0

  while IFS= read -r line; do
    case "$line" in
      BLOCK:*)
        local block_info="${line#BLOCK:}"
        IFS=: read -r file line desc <<< "$block_info"
        ;;
      OUTPUT:*)
        local section="${line#OUTPUT:}"
        local actual=""
        while IFS= read -r line && [[ "$line" != "END_OUTPUT" ]]; do
          actual="${actual}${line}"$'\n'
        done

        # Get expected output for this block from /tmp/parsed_examples.txt
        local expected=$(extract_expected_output "$file" "$line")

        if [[ "$actual" != "$expected" ]]; then
          echo "✗ MISMATCH: $file line $line ($desc) [$section]"
          echo
          echo "Expected:"
          echo "$expected" | sed 's/^/  /'
          echo
          echo "Actual:"
          echo "$actual" | sed 's/^/  /'
          echo
          ((mismatches++))
        fi
        ;;
    esac
  done < /tmp/actual_output.txt

  if [[ $mismatches -eq 0 ]]; then
    echo "✓ All test outputs match documented examples"
  else
    echo "✗ Found $mismatches output mismatch(es)"
    return 1
  fi
}

extract_expected_output() {
  local file="$1" line="$2"
  # Extract from parsed examples
  awk -v file="$file" -v line="$line" '
    /^FILE:/ && $0 == "FILE:" file { in_file=1 }
    in_file && /^LINE:/ && $0 == "LINE:" line { in_block=1 }
    in_block && /^OUTPUT_START/ { collecting=1; next }
    in_block && /^OUTPUT_END/ { exit }
    collecting { print }
  ' /tmp/parsed_examples.txt
}

parse_test_output
compare_outputs
```

### 5. Report

Provide summary:

```text
✓ 12 examples tested successfully
✗ 2 mismatches found:
  - README.md line 230: Output differs (see diff below)
  - doc/features.md line 45: Missing expected output comments
⚠ 3 new test blocks generated
⚠ 5 line number references updated
```

For mismatches, show side-by-side diff:

```text
Expected (from docs):     | Actual (from test):
 col1 | col2              |  col1 | col2
------+------             | ------+------
    1 | foo               |     1 | bar
                          |     2 | baz
```

## Key Implementation Details

### Extension Name Detection

- Check for `CREATE EXTENSION <name>` in existing tests
- Parse from `<extname>.control` file
- Ask user if ambiguous

### Setup SQL Extraction

- Extract CREATE/INSERT statements from example
- If example references existing tables, note in test comment: `-- Assumes: table xyz exists`
- Include DROP IF EXISTS in cleanup

### Expected Output Format

Expected output in docs should be exact psql format:

```sql
SELECT * FROM foo;
--  id | name
-- ----+------
--   1 | bar
-- (1 row)
```

Extract by:

1. Finding lines starting with `--` after SQL statement
2. Strip leading `--` and one space
3. Preserve alignment, dashes, row count

### Line Number Accuracy

- When updating references, use actual current line number ±0
- Round to nearest 5 for cleaner diffs (optional: ask user preference)

### Test Isolation

Each test block should be fully self-contained:

- Create all required objects
- Run tests
- Drop all objects in cleanup section
- Use unique object names if needed (e.g., `doc_example_1_products`)

## Error Handling

### Documentation Parse Errors

- If `-- Example:` found but no SQL block follows → warn and skip
- If SQL block has no expected output comments → warn but generate test anyway
- If markdown has malformed SQL fence → report specific line

### Test Generation Errors

- If can't determine extension name → ask user
- If setup SQL is ambiguous → include comment asking user to verify
- If example uses undefined objects → note in test comment

### Test Execution Errors

- If `make installcheck` fails → report stderr
- If results file missing → check if test ran at all
- If unexpected errors in output → report and ask user to review

## Examples

### Example 1: Simple Example Without "Without Extension"

**Documentation (README.md line 120):**

```sql
-- Example: Basic lookup
CREATE TABLE products (id int, name text);
INSERT INTO products VALUES (1, 'Widget');
SELECT * FROM products;
--  id |  name
-- ----+--------
--   1 | Widget
-- (1 row)
```

**Generated test:**

```sql
-- ============================================================================
-- README.md: Basic lookup (~line 120)
-- ============================================================================
CREATE TABLE doc_ex_products (id int, name text);
INSERT INTO doc_ex_products VALUES (1, 'Widget');

\echo '--- With extension ---'
CREATE EXTENSION myext;
SELECT * FROM doc_ex_products;

DROP TABLE doc_ex_products CASCADE;
```

### Example 2: With/Without Extension

**Documentation (README.md line 250):**

```sql
-- Example: FDW behavior comparison
-- Without extension (stock PostgreSQL):
CREATE FOREIGN TABLE remote_users (id int, name text)
  SERVER myserver;
SELECT * FROM remote_users WHERE id = 1;
-- ERROR: WHERE clause not pushed down
```

**Generated test:**

```sql
-- ============================================================================
-- README.md: FDW behavior comparison (~line 250)
-- ============================================================================
CREATE SERVER doc_ex_srv FOREIGN DATA WRAPPER postgres_fdw;
CREATE FOREIGN TABLE doc_ex_remote_users (id int, name text)
  SERVER doc_ex_srv;

\echo '--- Without extension ---'
DROP EXTENSION IF EXISTS myext CASCADE;
SELECT * FROM doc_ex_remote_users WHERE id = 1;

\echo '--- With extension ---'
CREATE EXTENSION myext;
SELECT * FROM doc_ex_remote_users WHERE id = 1;

DROP FOREIGN TABLE doc_ex_remote_users CASCADE;
DROP SERVER doc_ex_srv CASCADE;
```

## User Interaction

### Initial Invocation

When user says "generate doc example tests":

1. Ask: "Should I scan all markdown files (README.md, doc/*.md) or specific files?"
2. If first run: "What's your extension name?"
3. Show preview of what will be generated
4. Ask: "Proceed with generation and test run?"

### After Generation

1. Show summary of changes
2. If mismatches found: "Would you like me to update the documentation with actual test output?"
3. If new tests generated: "Review sql/doc_examples.sql and run 'make installcheck' to verify"

### Iterative Use

When doc_examples.sql already exists:

- "Found existing tests. I'll update line numbers and add missing tests."
- Preserve manual edits to tests (anything between test blocks)
- Warn if removing tests: "Test for 'old example' at line X not found in current docs. Keep it?"

## Configuration

Optional user preferences (ask on first use):

- Extension name (default: parse from .control file)
- Test object prefix (default: `doc_ex_`)
- Line number rounding (exact vs nearest 5)
- Auto-update docs with actual output (default: ask)

## Integration with pg-extension-review

This skill complements pg-extension-review by:

- Enforcing the constitution rule about doc example testing
- Being invoked automatically by pg-extension-review when doc changes detected
- Sharing the same understanding of repository structure

## Success Criteria

The skill succeeds when:

1. All `-- Example:` blocks in docs have tests in sql/doc_examples.sql
2. Test output matches documented output exactly
3. Line number references are accurate
4. Tests run cleanly with `make installcheck`
5. User can easily verify constitution compliance