#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="$SCRIPT_DIR/devcontainer.json"
PORTABLE="$SCRIPT_DIR/devcontainer.portable.json"
LINUX_USB="$SCRIPT_DIR/devcontainer.usb-linux.json"

MODE="${1:-auto}"

if [[ ! -f "$PORTABLE" || ! -f "$LINUX_USB" ]]; then
  echo "Missing template(s). Expected:"
  echo "  - $PORTABLE"
  echo "  - $LINUX_USB"
  exit 1
fi

select_portable() {
  cp "$PORTABLE" "$TARGET"
  echo "Selected devcontainer profile: portable"
}

select_linux_usb() {
  cp "$LINUX_USB" "$TARGET"
  echo "Selected devcontainer profile: linux-usb"
}

case "$MODE" in
  auto)
    OS_NAME="$(uname -s || true)"
    if [[ "$OS_NAME" == "Linux" ]]; then
      select_linux_usb
    else
      select_portable
    fi
    ;;
  portable)
    select_portable
    ;;
  linux-usb)
    select_linux_usb
    ;;
  *)
    echo "Usage: $0 [auto|portable|linux-usb]"
    exit 2
    ;;
esac

echo "Wrote: $TARGET"
echo "Next step: Rebuild/Reopen Container in VS Code"
