#!/usr/bin/env python3
"""
OUI Spy Unified Blue -- Firmware Flasher

Interactive flasher for XIAO ESP32-S3 boards. Run with no arguments for
a guided menu, or use CLI flags for automation.

    python scripts/flash.py           # interactive menu
    python scripts/flash.py --release # download + flash latest release
    python scripts/flash.py --build   # flash local build output
    python scripts/flash.py --batch   # batch-flash production run

Works on macOS, Linux, and Windows.

Requirements:  uv sync  (or: pip install esptool pyserial questionary)
"""

import glob
import hashlib
import io
import os
import shutil
import subprocess
import sys
import time

import serial.tools.list_ports

# Force UTF-8 on Windows so box-drawing / special chars don't garble
if sys.platform == "win32":
    os.system("")  # noqa: S605 S607 -- static call to enable ANSI/VT100 on Win10+ terminals
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
        sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    else:
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
        sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# -- Config ----------------------------------------------------------------
APP_OFFSET    = "0x10000"
BAUD          = "921600"
CHIP          = "esp32s3"
PROJECT_ROOT  = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
FIRMWARE_DIR  = os.path.join(PROJECT_ROOT, "firmware")
BUILD_BIN     = os.path.join(PROJECT_ROOT, ".pio", "build", "seeed_xiao_esp32s3", "firmware.bin")
RELEASE_ASSET = "oui-spy-unified-blue-firmware.bin"
CHECKSUMS_ASSET = "SHA256SUMS.txt"

# Known USB VID:PID pairs for ESP32-S3 / common UART bridges
ESP_VIDS = {
    "303A",  # Espressif USB JTAG/serial
    "1A86",  # CH340/CH341
    "10C4",  # CP210x
    "0403",  # FTDI
}

BANNER = """
  +==========================================+
  |   OUI Spy Unified Blue -- Flasher        |
  +==========================================+"""


# -- GitHub release download -----------------------------------------------

def _gh_repo():
    """Detect the GitHub repo from git remotes (prefers upstream, falls back to origin)."""
    for remote in ("upstream", "origin"):
        try:
            url = subprocess.run(
                ["git", "remote", "get-url", remote],
                capture_output=True, text=True, check=True, cwd=PROJECT_ROOT,
            ).stdout.strip()
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue
        # ssh: git@github.com:owner/repo.git
        if "github.com:" in url:
            return url.split("github.com:")[-1].removesuffix(".git")
        # https: https://github.com/owner/repo.git
        if "github.com/" in url:
            return "/".join(url.split("github.com/")[-1].removesuffix(".git").split("/")[:2])
    return None


def _verify_checksum(bin_path, checksums_path):
    """Verify a firmware binary against a SHA256SUMS.txt file. Returns True if valid."""
    sha = hashlib.sha256(open(bin_path, "rb").read()).hexdigest()
    basename = os.path.basename(bin_path)
    for line in open(checksums_path):
        parts = line.strip().split()
        if len(parts) == 2 and parts[1].lstrip("*") == basename:
            if parts[0] == sha:
                print(f"  Checksum OK: {sha[:16]}...")
                return True
            print(f"  Checksum MISMATCH!")
            print(f"    Expected: {parts[0]}")
            print(f"    Got:      {sha}")
            return False
    print(f"  Warning: {basename} not found in checksums file, skipping verification")
    return True


def download_release(tag=None):
    """Download a firmware release from GitHub. Returns path to .bin or None."""
    if not shutil.which("gh"):
        print("\n  'gh' CLI not found. Install it: https://cli.github.com/")
        return None

    repo = _gh_repo()
    if not repo:
        print("\n  Could not detect GitHub repo from git remotes.")
        return None

    os.makedirs(FIRMWARE_DIR, exist_ok=True)

    tag_label = tag or "latest"
    print(f"\n  Downloading {tag_label} release from {repo}...")

    # Build gh command
    gh_cmd = ["gh", "release", "download", "--repo", repo, "--dir", FIRMWARE_DIR,
              "--pattern", RELEASE_ASSET, "--pattern", CHECKSUMS_ASSET, "--clobber"]
    if tag:
        gh_cmd.append(tag)

    result = subprocess.run(gh_cmd)
    if result.returncode != 0:
        print(f"\n  Failed to download release. Is there a release tagged '{tag_label}'?")
        return None

    bin_path = os.path.join(FIRMWARE_DIR, RELEASE_ASSET)
    checksums_path = os.path.join(FIRMWARE_DIR, CHECKSUMS_ASSET)

    if not os.path.isfile(bin_path):
        print(f"\n  Download succeeded but {RELEASE_ASSET} not found.")
        return None

    # Verify checksum
    if os.path.isfile(checksums_path):
        if not _verify_checksum(bin_path, checksums_path):
            os.remove(bin_path)
            return None
        os.remove(checksums_path)
    else:
        print("  Warning: no checksums file in release, skipping verification")

    size_kb = os.path.getsize(bin_path) / 1024
    print(f"  Downloaded: {RELEASE_ASSET} ({size_kb:.0f} KB)")
    return bin_path


# -- Port detection --------------------------------------------------------

def find_port():
    """Auto-detect the ESP32 serial port (macOS, Linux, Windows)."""
    ports = serial.tools.list_ports.comports()
    candidates = []
    for p in ports:
        vid = f"{p.vid:04X}" if p.vid else ""
        if vid in ESP_VIDS:
            candidates.append(p)
        elif "esp" in (p.description or "").lower():
            candidates.append(p)
        # macOS
        elif "usbmodem" in (p.device or "").lower():
            candidates.append(p)
        elif "usbserial" in (p.device or "").lower():
            candidates.append(p)
        # Linux
        elif "ttyACM" in (p.device or ""):
            candidates.append(p)
        elif "ttyUSB" in (p.device or ""):
            candidates.append(p)
        # Windows — COM ports with a real description (skip built-in COM1)
        elif sys.platform == "win32" and (p.device or "").upper().startswith("COM"):
            port_num = p.device.upper().replace("COM", "")
            if port_num.isdigit() and int(port_num) > 1:
                candidates.append(p)

    if len(candidates) == 1:
        return candidates[0].device
    if len(candidates) > 1:
        print("\n  Multiple serial ports found:\n")
        for i, p in enumerate(candidates):
            desc = p.description or "unknown"
            vid = f"{p.vid:04X}" if p.vid else "----"
            print(f"    [{i + 1}] {p.device}  ({desc})  VID:{vid}")
        print()
        while True:
            try:
                choice = input("  Pick a port [1]: ").strip()
                idx = int(choice) - 1 if choice else 0
                if 0 <= idx < len(candidates):
                    return candidates[idx].device
            except (ValueError, IndexError):
                pass
            print("  Invalid choice, try again.")
    return None


def wait_for_port(timeout=30):
    """Wait for an ESP32 to appear on USB. Returns port or None."""
    print(f"\n  Waiting for ESP32 (plug in a board)...", end="", flush=True)
    start = time.time()
    last_dot = start
    while time.time() - start < timeout:
        port = find_port()
        if port:
            print(f" found!")
            return port
        if time.time() - last_dot >= 2:
            print(".", end="", flush=True)
            last_dot = time.time()
        time.sleep(0.5)
    print(" timeout.")
    return None


# -- Firmware discovery ----------------------------------------------------

def find_local_build():
    """Return the local PlatformIO build output if it exists."""
    if os.path.isfile(BUILD_BIN):
        return BUILD_BIN
    return None


def find_firmware(path_arg=None):
    """Locate the .bin file to flash.

    Search order: explicit path -> local build -> firmware/ dir -> CWD.
    """
    # Explicit path from CLI
    if path_arg:
        if os.path.isfile(path_arg):
            return os.path.abspath(path_arg)
        print(f"\n  File not found: {path_arg}")
        sys.exit(1)

    # Local PlatformIO build output
    build = find_local_build()
    if build:
        return build

    # Look in firmware/ folder
    if os.path.isdir(FIRMWARE_DIR):
        bins = sorted(glob.glob(os.path.join(FIRMWARE_DIR, "*.bin")), key=os.path.getmtime, reverse=True)
        if len(bins) == 1:
            return bins[0]
        if len(bins) > 1:
            print("\n  Multiple .bin files found:\n")
            for i, b in enumerate(bins):
                size = os.path.getsize(b) / 1024
                print(f"    [{i + 1}] {os.path.basename(b)}  ({size:.0f} KB)")
            print()
            while True:
                try:
                    choice = input("  Pick a firmware [1]: ").strip()
                    idx = int(choice) - 1 if choice else 0
                    if 0 <= idx < len(bins):
                        return bins[idx]
                except (ValueError, IndexError):
                    pass
                print("  Invalid choice, try again.")

    # Check current directory
    bins = sorted(glob.glob("*.bin"), key=os.path.getmtime, reverse=True)
    if bins:
        return os.path.abspath(bins[0])

    return None


# -- Flash / erase ---------------------------------------------------------

def flash_one(port, firmware, do_erase=False, board_num=None):
    """Flash a single board. Returns True on success."""
    size_kb = os.path.getsize(firmware) / 1024
    label = f"  Board #{board_num}" if board_num else "  Target"
    print(f"""
{label}
  Port:       {port}
  Firmware:   {os.path.basename(firmware)}  ({size_kb:.0f} KB)
  Chip:       {CHIP}
  Offset:     {APP_OFFSET}
  Baud:       {BAUD}
""")

    if do_erase:
        erase(port)

    print("  Flashing...\n")

    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", CHIP,
        "--port", port,
        "--baud", BAUD,
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "-z",
        "--flash_mode", "qio",
        "--flash_freq", "80m",
        "--flash_size", "detect",
        APP_OFFSET, firmware,
    ]

    try:
        result = subprocess.run(cmd)
        if result.returncode == 0:
            print("\n  Done! Device will reboot into the new firmware.")
            return True
        else:
            print(f"\n  esptool exited with code {result.returncode}")
            return False
    except FileNotFoundError:
        print("  esptool not found. Install it:\n")
        print("    uv sync\n")
        sys.exit(1)


def erase(port):
    """Full flash erase."""
    print(f"  Erasing flash on {port}...\n")
    cmd = [
        sys.executable, "-m", "esptool",
        "--chip", CHIP,
        "--port", port,
        "--baud", BAUD,
        "erase_flash",
    ]
    subprocess.run(cmd)
    print()


# -- Batch mode ------------------------------------------------------------

def batch_mode(firmware, do_erase=False):
    """Flash multiple boards one after another."""
    print(BANNER)
    size_kb = os.path.getsize(firmware) / 1024
    print(f"""
  BATCH MODE -- flash boards one after another
  Firmware:   {os.path.basename(firmware)}  ({size_kb:.0f} KB)
  Erase:      {"YES" if do_erase else "no"}

  Plug in a board, flash, swap, repeat.
  Press Ctrl+C when done.
""")

    board_num = 0
    success_count = 0
    fail_count = 0

    while True:
        board_num += 1

        # Wait for a board to appear
        port = wait_for_port(timeout=300)  # 5 min timeout
        if not port:
            print("\n  No board detected. Still waiting? Plug one in and try again.")
            try:
                input("  Press Enter to retry, Ctrl+C to quit: ")
            except KeyboardInterrupt:
                break
            continue

        # Give the port a moment to stabilize (Windows especially needs this)
        time.sleep(1)

        ok = flash_one(port, firmware, do_erase=do_erase, board_num=board_num)
        if ok:
            success_count += 1
        else:
            fail_count += 1

        print(f"\n  -- Score: {success_count} flashed, {fail_count} failed --")

        try:
            resp = input("\n  Swap board and press Enter for next (q to quit): ").strip().lower()
            if resp in ("q", "quit", "exit"):
                break
        except (KeyboardInterrupt, EOFError):
            break

        # Wait for the old port to disappear and new one to appear
        print("  Waiting for board swap...", end="", flush=True)
        time.sleep(2)  # give time for USB disconnect
        print(" ready.")

    print(f"""
  +==========================================+
  |   Batch complete                         |
  +==========================================+
  |   Flashed:  {success_count:<5}                        |
  |   Failed:   {fail_count:<5}                        |
  +==========================================+
""")


# -- Flash helpers for specific sources ------------------------------------

def flash_firmware(firmware, do_erase=False):
    """Find port and flash a given firmware binary."""
    print(BANNER)
    port = find_port()
    if not port:
        print("\n  No ESP32 detected. Is the board plugged in?")
        print("  Make sure drivers are installed (CH340/CP210x).\n")
        sys.exit(1)

    print(f"  Found: {port}")

    confirm = input("\n  Flash? [Y/n]: ").strip().lower()
    if confirm and confirm != "y":
        print("  Aborted.")
        sys.exit(0)

    ok = flash_one(port, firmware, do_erase=do_erase)
    if not ok:
        sys.exit(1)


def do_flash_release(tag=None, do_erase=False):
    """Download a release and flash it."""
    firmware = download_release(tag)
    if not firmware:
        sys.exit(1)
    flash_firmware(firmware, do_erase=do_erase)


def do_flash_build(do_erase=False):
    """Flash the local PlatformIO build output."""
    build = find_local_build()
    if not build:
        print(f"\n  No local build found at:")
        print(f"    {BUILD_BIN}")
        print(f"\n  Build first with: just build  (or: just docker-build)\n")
        sys.exit(1)
    size_kb = os.path.getsize(build) / 1024
    mtime = time.strftime("%Y-%m-%d %H:%M", time.localtime(os.path.getmtime(build)))
    print(f"\n  Local build: {os.path.basename(build)} ({size_kb:.0f} KB, built {mtime})")
    flash_firmware(build, do_erase=do_erase)


# -- Interactive menu ------------------------------------------------------

def interactive_menu():
    """Show an interactive menu when no arguments are provided."""
    import questionary

    print(BANNER)

    # Gather info for the menu
    build = find_local_build()
    build_label = None
    if build:
        size_kb = os.path.getsize(build) / 1024
        mtime = time.strftime("%Y-%m-%d %H:%M", time.localtime(os.path.getmtime(build)))
        build_label = f"Flash local build ({size_kb:.0f} KB, built {mtime})"

    firmware_bins = []
    if os.path.isdir(FIRMWARE_DIR):
        firmware_bins = sorted(glob.glob(os.path.join(FIRMWARE_DIR, "*.bin")),
                               key=os.path.getmtime, reverse=True)

    choices = []
    choice_map = {}

    if build_label:
        choices.append(questionary.Choice(build_label, value="build"))
        choice_map["build"] = build

    label = "Flash latest GitHub release"
    choices.append(questionary.Choice(label, value="release"))

    if firmware_bins:
        for b in firmware_bins:
            size = os.path.getsize(b) / 1024
            name = os.path.basename(b)
            label = f"Flash firmware/{name} ({size:.0f} KB)"
            key = f"file:{b}"
            choices.append(questionary.Choice(label, value=key))
            choice_map[key] = b

    choices.append(questionary.Choice("Batch flash (production run)", value="batch"))
    choices.append(questionary.Choice("Erase flash only", value="erase"))
    choices.append(questionary.Separator())
    choices.append(questionary.Choice("Quit", value="quit"))

    print()
    action = questionary.select(
        "What would you like to do?",
        choices=choices,
    ).ask()

    if action is None or action == "quit":
        return

    do_erase = False
    if action not in ("erase",):
        do_erase = questionary.confirm("Erase flash before writing?", default=False).ask()
        if do_erase is None:
            return

    if action == "build":
        flash_firmware(choice_map["build"], do_erase=do_erase)
    elif action == "release":
        tag = questionary.text(
            "Release tag (leave blank for latest):",
            default="",
        ).ask()
        if tag is None:
            return
        do_flash_release(tag=tag or None, do_erase=do_erase)
    elif action.startswith("file:"):
        flash_firmware(choice_map[action], do_erase=do_erase)
    elif action == "batch":
        firmware = find_firmware()
        if not firmware:
            print(f"\n  No .bin file found to batch-flash.")
            print(f"  Build first or download a release.\n")
            sys.exit(1)
        batch_mode(firmware, do_erase=do_erase)
    elif action == "erase":
        port = find_port()
        if not port:
            print("\n  No ESP32 detected.")
            sys.exit(1)
        erase(port)


# -- CLI entry point -------------------------------------------------------

def _check_esptool():
    """Verify esptool is installed."""
    try:
        subprocess.run([sys.executable, "-m", "esptool", "version"],
                       capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("\n  esptool not found. Install it:\n")
        print("    uv sync\n")
        sys.exit(1)


def main():
    args = sys.argv[1:]

    # No arguments -> interactive menu
    if not args:
        _check_esptool()
        interactive_menu()
        return

    # Parse flags
    do_erase = "--erase" in args
    do_batch = "--batch" in args
    do_release = "--release" in args
    do_build = "--build" in args
    bin_path = None
    release_tag = None

    i = 0
    while i < len(args):
        a = args[i]
        if a in ("--erase", "--batch", "--build"):
            pass
        elif a == "--release":
            # Next arg might be a tag (if it doesn't start with --)
            if i + 1 < len(args) and not args[i + 1].startswith("--"):
                release_tag = args[i + 1]
                i += 1
        elif a in ("-h", "--help"):
            print("""
  Usage:  python scripts/flash.py [options] [firmware.bin]

  Modes:
    (no args)          Interactive menu
    --release [TAG]    Download + flash a GitHub release (default: latest)
    --build            Flash local PlatformIO build output
    --batch            Batch mode: flash multiple boards sequentially
    firmware.bin       Flash a specific .bin file

  Options:
    --erase            Erase entire flash before writing

  Examples:
    python scripts/flash.py                  # interactive menu
    python scripts/flash.py --release        # flash latest release
    python scripts/flash.py --release v1.0   # flash specific release
    python scripts/flash.py --build          # flash local build
    python scripts/flash.py --batch --erase  # batch flash with erase
    python scripts/flash.py firmware.bin     # flash specific file
""")
            sys.exit(0)
        else:
            bin_path = a
        i += 1

    _check_esptool()

    if do_release:
        do_flash_release(tag=release_tag, do_erase=do_erase)
        return

    if do_build:
        do_flash_build(do_erase=do_erase)
        return

    # Find firmware (explicit path, local build, firmware/ dir, or CWD)
    firmware = find_firmware(bin_path)
    if not firmware:
        print(f"\n  No .bin file found.")
        print(f"  Build first:       just build  (or: just docker-build)")
        print(f"  Download release:  just flash-release")
        print(f"  Or pass directly:  python scripts/flash.py firmware.bin\n")
        sys.exit(1)

    if do_batch:
        batch_mode(firmware, do_erase=do_erase)
        return

    # Single flash
    flash_firmware(firmware, do_erase=do_erase)

    # Offer to flash another
    try:
        resp = input("\n  Flash another board? [y/N]: ").strip().lower()
        if resp == "y":
            batch_mode(firmware, do_erase=do_erase)
    except (KeyboardInterrupt, EOFError):
        pass

    print()


if __name__ == "__main__":
    main()
