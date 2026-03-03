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

Inside the container, you can use either raw PlatformIO commands (`pio ...`) or the project `Justfile` (`just build`, `just upload`, `just monitor`). `just` is preinstalled in this devcontainer image.

Quality checks in-container:

- `just lint`
- `just test`
- `just test-cpp`
- `just analyze-cpp`
- `just quality`

`just analyze-cpp` runs `cppcheck` only against maintained source directories (`src/main.cpp`, `src/hal`, `src/modules`, `src/web`).

The image also pre-installs PlatformIO packages declared in `platformio.ini` during image build for faster first-run builds.

## Recommended workflow by host OS

### Linux

- If you need direct flashing from container, copy the Linux template over the default:

```bash
bash .devcontainer/select-profile.sh linux-usb
```

- Rebuild/reopen container in VS Code
- Build/flash inside container:

```bash
pio run
pio run -t upload
pio device monitor
```

### macOS

Use the default `devcontainer.json` for build/dev. USB passthrough can be limited depending on Docker Desktop and host setup.

Recommended path:

- Build in container: `pio run`
- Flash from host: `python flash.py` or host `pio run -t upload`

### Windows (Docker Desktop + WSL2)

Use the default `devcontainer.json` for build/dev.

For serial flashing, USB forwarding to WSL can vary by setup (`usbipd-win` + WSL attach). If serial isn’t visible in-container, use host upload instead:

- Build in container: `pio run`
- Flash from host: `python flash.py` or host `pio run -t upload`

## Quick reset to portable default

If you switched templates and want to revert:

```bash
bash .devcontainer/select-profile.sh portable
```
