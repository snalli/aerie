# Docker CI Environment

Match the GitHub Actions CI environment locally using Docker. Test your code changes before pushing to ensure CI will pass.

## Quick Start

### Option 1: Using the Script (Recommended)

```bash
# Interactive shell in CI environment
bash scripts/docker-ci-run.sh bash

# Run specific checks
bash scripts/docker-ci-run.sh format-code
bash scripts/docker-ci-run.sh format-fix
bash scripts/docker-ci-run.sh analyze
bash scripts/docker-ci-run.sh quality
bash scripts/docker-ci-run.sh build
bash scripts/docker-ci-run.sh ci-full    # Run entire CI pipeline
```

### Option 2: Using Docker Compose

```bash
# Start interactive shell
docker-compose -f docker-compose.ci.yml run --rm ci bash

# Run specific command
docker-compose -f docker-compose.ci.yml run --rm ci bash scripts/format-code.sh --check
```

### Option 3: Using Docker Directly

```bash
# Build image
docker build -f Dockerfile.ci -t aerie-ci:latest .

# Run interactively
docker run -it -v $(pwd):/workspace aerie-ci:latest bash

# Run command
docker run --rm -v $(pwd):/workspace aerie-ci:latest \
  bash scripts/format-code.sh --check
```

## Environment Details

The Docker image matches GitHub Actions CI with:

| Tool | Version | Purpose |
|------|---------|---------|
| **clang-format** | 22.x | Code formatting |
| **cppcheck** | latest | Static analysis |
| **cmake** | latest | Build system |
| **build-essential** | latest | C++ compiler & tools |
| **Ubuntu** | 22.04 | Base OS |

## Common Workflows

### Before Committing

```bash
# Quick quality check
bash scripts/docker-ci-run.sh quality

# Or run full CI simulation
bash scripts/docker-ci-run.sh ci-full
```

### Fix Formatting Issues

```bash
# Inside Docker
bash scripts/docker-ci-run.sh format-fix

# Changes are applied to your local files (mounted volume)
# Then commit and push
```

### Build Project

```bash
bash scripts/docker-ci-run.sh build
```

### Interactive Development

```bash
# Start interactive shell
bash scripts/docker-ci-run.sh bash

# Inside container, run any commands:
$ bash scripts/format-code.sh --check
$ bash scripts/analyze-code.sh critical
$ cd libfs && cmake . && make
$ exit
```

## Troubleshooting

### Docker not installed
Install Docker Desktop: https://www.docker.com/products/docker-desktop

### Image build fails
```bash
# Rebuild image from scratch
docker image rm aerie-ci:latest
bash scripts/docker-ci-run.sh bash
```

### Volume permission issues (Linux)
```bash
# Run with user context
docker run -it --user $(id -u):$(id -g) \
  -v $(pwd):/workspace aerie-ci:latest bash
```

### Container exits immediately
Make sure you're in the project root:
```bash
cd /path/to/aerie
bash scripts/docker-ci-run.sh bash
```

## Why Use Docker?

| Benefit | Details |
|---------|---------|
| **Consistency** | Same environment as GitHub Actions CI |
| **Reliability** | No "works on my machine" issues |
| **Isolation** | Doesn't affect your system |
| **Reproducibility** | Anyone can run the same checks |
| **Testing** | Test CI changes before pushing |

## Files

- **`Dockerfile.ci`** - Docker image definition
- **`docker-compose.ci.yml`** - Docker Compose configuration
- **`scripts/docker-ci-run.sh`** - Helper script for common tasks

## Advanced Usage

### Mount specific directory
```bash
docker run -it -v /path/to/aerie:/workspace aerie-ci:latest bash
```

### Run with different entrypoint
```bash
docker run -it -v $(pwd):/workspace aerie-ci:latest bash -c "cd libfs && make"
```

### Shell into running container
```bash
# Terminal 1: Start container
docker run -it --name aerie-work -v $(pwd):/workspace aerie-ci:latest bash

# Terminal 2: Shell into it
docker exec -it aerie-work bash
```

### View image details
```bash
docker inspect aerie-ci:latest
docker image history aerie-ci:latest
```

## CI Pipeline Simulation

The `ci-full` command runs the complete GitHub Actions pipeline:

```bash
bash scripts/docker-ci-run.sh ci-full
```

This runs in order:
1. **Formatting Check** - `clang-format` validation
2. **Static Analysis** - `cppcheck` critical issues
3. **Build Test** - CMake configuration and build

If all pass, your code is ready to push! ✅

## Next Steps

1. **Install Docker** if you haven't already
2. **Test locally** before pushing:
   ```bash
   bash scripts/docker-ci-run.sh ci-full
   ```
3. **Fix any issues** that docker reports
4. **Push to GitHub** with confidence

---

**Status**: ✅ Ready to use  
**Last Updated**: 2026-05-31
