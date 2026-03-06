# Devcontainer Profiles

This folder includes one default profile and one USB-enabled template:

- `devcontainer.json` — active profile used by VS Code Dev Containers
- `devcontainer.portable.json` — portable default for all hosts (best for build/test)
- `devcontainer.usb-linux.json` — Linux host template with USB passthrough for direct `pio run -t upload`

## One-command selector

Pick the best profile for your host automatically:

```bash
bash .devcontainer/select-profile.sh
```

Manual override:

```bash
bash .devcontainer/select-profile.sh portable
bash .devcontainer/select-profile.sh linux-usb
```

Then run **Dev Containers: Rebuild and Reopen in Container**.

## Published image

The devcontainer image is published to GitHub Container Registry on every push to `master` that changes devcontainer-related files. Local builds use `cacheFrom` to pull cached layers, making rebuilds fast.

```text
ghcr.io/dougborg/oui-spy-unified-blue/devcontainer:latest
```

## What's pre-installed

The Docker image includes:

- PlatformIO CLI with all project platform/library packages
- Node.js 22 + web dashboard npm dependencies
- Python dev tools (`pre-commit`, `pytest`, `gcovr`)
- Pre-commit hook environments (clang-format, markdownlint-cli2, Biome, pre-commit-hooks) — cached in the image so hooks run instantly without reinstalling
- `clang-format`, `cppcheck` for C++ formatting and static analysis
- `just` task runner

## In-container commands

Build and flash:

```bash
pio run                  # build firmware
pio run -t upload        # flash (Linux USB only)
pio device monitor       # serial monitor
```

Quality checks:

```bash
just lint                # pre-commit (formatting, linting)
just test                # Python tests
just test-cpp            # native C++ tests (53 tests)
just coverage-cpp        # tests + coverage report (35% threshold)
just analyze-cpp         # cppcheck static analysis
just web-lint            # Biome linter on web source
just web-test            # Vitest web tests (18 tests)
just quality             # all of the above
```

## Docker targets (no local env needed)

Every build/test/lint target has a `docker-` counterpart that runs inside the
published devcontainer image. Only Docker is required on the host.

```bash
just docker-setup        # first-time: install pio packages + web deps
just docker-build        # build firmware
just docker-build-all    # build web + firmware
just docker-test         # Python tests
just docker-test-cpp     # native C++ tests
just docker-coverage-cpp # tests + coverage report
just docker-lint         # pre-commit (formatting, linting)
just docker-analyze-cpp  # cppcheck static analysis
just docker-web-lint     # Biome linter
just docker-web-test     # Vitest web tests
just docker-web-typecheck # TypeScript type checks
just docker-web-build    # build web dashboard
just docker-quality      # all quality checks
```

Flash/upload/monitor are not available via Docker (they need host USB access).

## Recommended workflow by host OS

### Linux

If you need direct flashing from container:

```bash
bash .devcontainer/select-profile.sh linux-usb
```

Rebuild/reopen container, then build and flash inside:

```bash
pio run
pio run -t upload
pio device monitor
```

### macOS

Use the default `devcontainer.json` for build/dev. USB passthrough can be limited depending on Docker Desktop setup.

Recommended path:

- Build in container: `pio run` or from host: `just docker-build`
- Flash from host: `just flash` or `pio run -t upload`

### Windows (Docker Desktop + WSL2)

Use the default `devcontainer.json` for build/dev.

For serial flashing, USB forwarding to WSL can vary by setup (`usbipd-win` + WSL attach). If serial isn't visible in-container, use host upload instead:

- Build in container: `pio run` or from host: `just docker-build`
- Flash from host: `just flash` or `pio run -t upload`

## Quick reset to portable default

```bash
bash .devcontainer/select-profile.sh portable
```
