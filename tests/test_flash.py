from pathlib import Path
from types import SimpleNamespace

import pytest

import flash


def test_find_port_prefers_known_vid(monkeypatch):
    ports = [
        SimpleNamespace(device="/dev/ttyS0", vid=None, description="ttyS0"),
        SimpleNamespace(device="/dev/ttyUSB0", vid=0x303A, description="Espressif USB"),
    ]
    monkeypatch.setattr(flash.serial.tools.list_ports, "comports", lambda: ports)

    assert flash.find_port() == "/dev/ttyUSB0"


def test_find_firmware_uses_explicit_path(tmp_path):
    firmware = tmp_path / "firmware.bin"
    firmware.write_bytes(b"test")

    assert flash.find_firmware(str(firmware)) == str(firmware.resolve())


def test_find_firmware_missing_explicit_path_exits():
    with pytest.raises(SystemExit) as excinfo:
        flash.find_firmware("/no/such/file.bin")

    assert excinfo.value.code == 1


def test_find_firmware_single_bin_in_firmware_dir(monkeypatch, tmp_path):
    firmware_dir = tmp_path / "firmware"
    firmware_dir.mkdir()
    firmware = firmware_dir / "oui-spy.bin"
    firmware.write_bytes(b"test")

    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(firmware_dir))

    assert flash.find_firmware() == str(firmware)


def test_find_firmware_prompts_for_multiple_bins(monkeypatch, tmp_path):
    import os

    firmware_dir = tmp_path / "firmware"
    firmware_dir.mkdir()
    older = firmware_dir / "older.bin"
    newer = firmware_dir / "newer.bin"
    older.write_bytes(b"older")
    newer.write_bytes(b"newer")
    os.utime(older, (1_700_000_000, 1_700_000_000))
    os.utime(newer, (1_800_000_000, 1_800_000_000))

    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(firmware_dir))
    monkeypatch.setattr("builtins.input", lambda _: "2")

    selected = Path(flash.find_firmware())
    assert selected.name == "older.bin"
