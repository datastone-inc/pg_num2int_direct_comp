# Installation Guide

## Prerequisites

### Required Software

- **PostgreSQL**: Version 12 or later
- **Build Tools**: GCC or Clang with C99 support
- **PostgreSQL Development Headers**: 
  - Debian/Ubuntu: `postgresql-server-dev-<version>`
  - Red Hat/CentOS: `postgresql<version>-devel`
  - macOS (Homebrew): Included with `postgresql`
  - Windows: PostgreSQL installer includes development files

### Verify Prerequisites

```bash
# Check PostgreSQL version
psql --version

# Check pg_config (required for PGXS)
pg_config --version

# Verify development headers
pg_config --includedir-server
```

## Building from Source

### Clone Repository

```bash
git clone https://github.com/datastone-inc/pg_num2int_direct_comp.git
cd pg-num2int-direct-comp
```

### Build Extension

```bash
make
```

Expected output:
```
gcc -Wall -Wextra -Werror -std=gnu99 -fPIC ...
```

### Run Tests (Optional)

```bash
make installcheck
```

All tests should pass (100% pass rate required).

### Install Extension

```bash
sudo make install
```

This copies:
- `pg_num2int_direct_comp.so` → `$libdir`
- `pg_num2int_direct_comp--1.0.0.sql` → `$sharedir/extension`
- `pg_num2int_direct_comp.control` → `$sharedir/extension`

## Enabling in Database

### Create Extension

Connect to your database and run:

```sql
CREATE EXTENSION pg_num2int_direct_comp;
```

### Verify Installation

```sql
-- Check extension is loaded
\dx pg_num2int_direct_comp

-- List all extension objects (108 operators, 138 functions, etc.)
\dx+ pg_num2int_direct_comp

-- Test precision loss detection (stock PostgreSQL returns true for both)
SELECT 16777216::int4 = 16777217::float4;  -- true (float4 rounds to 16777216)
SELECT 16777217::int4 = 16777217::float4;  -- false (extension detects mismatch!)
```

See the [Quick Start examples](../README.md#quick-start) for more usage patterns.

## Troubleshooting

### Build Errors

**Error**: `pg_config: command not found`

**Solution**: Add PostgreSQL bin directory to PATH:
```bash
export PATH=/usr/pgsql-<version>/bin:$PATH  # Red Hat/CentOS
export PATH=/usr/lib/postgresql/<version>/bin:$PATH  # Debian/Ubuntu
```

**Error**: `postgres.h: No such file or directory`

**Solution**: Install development headers:
```bash
sudo apt install postgresql-server-dev-<version>  # Debian/Ubuntu
sudo yum install postgresql<version>-devel         # Red Hat/CentOS
```

### Installation Errors

**Error**: `permission denied` during `make install`

**Solution**: Use sudo or install to custom directory:
```bash
sudo make install
# OR
make install DESTDIR=/custom/path
```

### Runtime Errors

**Error**: `could not access file "$libdir/pg_num2int_direct_comp"`

**Solution**: Verify library is in correct location:
```bash
pg_config --pkglibdir
ls $(pg_config --pkglibdir)/pg_num2int_direct_comp.so
```

## Platform-Specific Notes

### Linux

Standard PGXS build works on all distributions.

### macOS

Homebrew PostgreSQL includes all required headers. Use:
```bash
make PG_CONFIG=/opt/homebrew/bin/pg_config  # Apple Silicon
make PG_CONFIG=/usr/local/bin/pg_config     # Intel
```

### Windows

Build with MinGW or MSVC:
```cmd
SET PATH=C:\Program Files\PostgreSQL\<version>\bin;%PATH%
make
```

## Uninstallation

```sql
-- In database
DROP EXTENSION pg_num2int_direct_comp;
```

```bash
# Remove files
sudo make uninstall
```

## Next Steps

- [Operator Reference](operator-reference.md) - Operator reference and usage guide
- [Development Guide](development.md) - Contributing and testing
