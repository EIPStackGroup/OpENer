# OpENer CI/CD Workflows

This document describes the continuous integration and analysis workflows for the OpENer project.

## Overview

The CI/CD pipeline is split into two workflows optimized for different purposes:

### 1. **CI Workflow** (`ci.yml`)
- **Purpose**: Fast feedback for daily development
- **Triggers**: Every push to `master`, every pull request
- **Duration**: ~5-10 minutes
- **Analysis Level**: Standard (fast) static analysis

### 2. **Exhaustive Analysis** (`exhaustive-analysis.yml`)
- **Purpose**: Thorough validation for releases
- **Triggers**:
  - Release branches (`release/**`)
  - Version tags (`v*`)
  - Nightly at 3 AM UTC
  - Manual workflow dispatch
- **Duration**: ~30-60 minutes
- **Analysis Level**: Exhaustive static analysis

## Workflow Details

### CI Workflow (Fast)

**Jobs:**
1. **lint** - Quick code quality checks
   - MegaLinter with standard cppcheck
   - Auto-applies formatting fixes on PRs
   - Suppresses `normalCheckLevelMaxBranches` info message

2. **build-test** - Build and test
   - Compiles with optimizations enabled
   - Runs full test suite with parallel execution
   - Generates code coverage reports
   - Posts coverage summary on PRs

**Best Practices Applied:**
- ✅ Parallel builds (`-j$(nproc)`)
- ✅ Parallel test execution
- ✅ Action version pinning with SHA (security)
- ✅ Automatic linter fixes
- ✅ Coverage reporting on PRs

### Exhaustive Analysis (Thorough)

**Jobs:**
1. **exhaustive-lint** - Deep static analysis
   - Full branch analysis with `--check-level=exhaustive`
   - Enables all cppcheck warnings (style, performance, portability)
   - Creates GitHub issue on nightly failures
   - Retains reports for 30 days

2. **build-release** - Release configuration testing
   - Strict compiler warnings (`-Wall -Wextra -Werror`)
   - Full optimization testing
   - Separate coverage report for releases

## Configuration Files

### `.mega-linter.yml`
Central MegaLinter configuration with:
- Linter inclusions/exclusions
- File filtering rules
- Report settings
- Parallel processing enabled

### When Does Each Workflow Run?

| Event | CI (Fast) | Exhaustive |
|-------|-----------|------------|
| Push to `master` | ✅ | ❌ |
| Pull request | ✅ | ❌ |
| Push to `release/*` | ❌ | ✅ |
| Version tag (`v*`) | ❌ | ✅ |
| Nightly (3 AM UTC) | ❌ | ✅ |
| Manual trigger | ❌ | ✅ |

## Benefits of This Approach

### For Developers
- ⚡ **Fast feedback** on PRs (5-10 min)
- 🔧 **Auto-fixes** for formatting issues
- 📊 **Immediate coverage** reports
- 🎯 **No waiting** for exhaustive checks during development

### For Releases
- 🔍 **Thorough validation** before releases
- 🛡️ **Deep branch analysis** finds edge cases
- 📈 **Long-term tracking** with nightly runs
- 🚨 **Automatic alerts** for regressions

### For Industrial/Safety Context
- ✅ OpENer is used in **manufacturing and automation**
- ✅ Exhaustive checks catch **subtle issues** in critical paths
- ✅ **Nightly monitoring** ensures code health
- ✅ **Release validation** provides confidence for production deployments

## Maintenance

### Updating Dependencies
All action versions are pinned to specific commits for security. To update:

```bash
# Check for updates
gh workflow view ci.yml

# Update specific action
# Replace SHA in workflows with new version
```

### Adjusting Analysis Depth

**To make standard checks more thorough:**
```yaml
# In ci.yml
CPPCHECK_ARGUMENTS: --inline-suppr --enable=warning
```

**To reduce exhaustive analysis:**
```yaml
# In exhaustive-analysis.yml
CPPCHECK_ARGUMENTS: --check-level=exhaustive --inline-suppr
# Remove: --enable=warning,style,performance,portability
```

### Troubleshooting

**If CI is too slow:**
- Reduce parallel jobs: `cmake --build ... -j2`
- Skip tests on draft PRs
- Reduce validation scope

**If exhaustive analysis fails nightly:**
- Check created GitHub issues
- Download artifacts for detailed reports
- Review cppcheck output for false positives

**If linter fixes cause conflicts:**
- Add `skip fix` to commit message
- Adjust `.mega-linter.yml` rules
- Disable specific fixers

## Migration from Old Workflow

The new structure maintains all functionality from `build.yml`:

| Old Feature | New Location | Changes |
|------------|--------------|---------|
| MegaLinter | `ci.yml` → lint | Split into standard/exhaustive |
| Build & Test | `ci.yml` → build-test | Added parallel execution |
| Coverage | `ci.yml` → build-test | Enhanced reporting |
| Auto-fixes | `ci.yml` → lint | Simplified logic |
| Release validation | NEW: `exhaustive-analysis.yml` | Added thorough checks |

## Manual Workflow Execution

```bash
# Trigger exhaustive analysis manually
gh workflow run exhaustive-analysis.yml

# View recent runs
gh run list --workflow=ci.yml

# Download artifacts
gh run download <run-id>
```

## Questions?

- **Why two workflows?** Balance between speed (dev) and thoroughness (release)
- **Why nightly exhaustive?** Catches regressions without slowing daily work
- **Can I run exhaustive on my PR?** Yes, via workflow dispatch, but not recommended for regular PRs
- **What if exhaustive finds issues?** Fix in a follow-up PR; doesn't block daily development

---

*Last updated: December 2025*