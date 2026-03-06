import hashlib
import json
import os
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
    monkeypatch.setattr(flash, "BUILD_BIN", str(tmp_path / "nonexistent.bin"))

    assert flash.find_firmware() == str(firmware)


def test_find_firmware_prompts_for_multiple_bins(monkeypatch, tmp_path):
    firmware_dir = tmp_path / "firmware"
    firmware_dir.mkdir()
    older = firmware_dir / "older.bin"
    newer = firmware_dir / "newer.bin"
    older.write_bytes(b"older")
    newer.write_bytes(b"newer")
    os.utime(older, (1_700_000_000, 1_700_000_000))
    os.utime(newer, (1_800_000_000, 1_800_000_000))

    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(firmware_dir))
    monkeypatch.setattr(flash, "BUILD_BIN", str(tmp_path / "nonexistent.bin"))
    monkeypatch.setattr("builtins.input", lambda _: "2")

    selected = Path(flash.find_firmware())
    assert selected.name == "older.bin"


def test_find_firmware_prefers_local_build(monkeypatch, tmp_path):
    build_bin = tmp_path / "firmware.bin"
    build_bin.write_bytes(b"local build")

    firmware_dir = tmp_path / "firmware"
    firmware_dir.mkdir()
    (firmware_dir / "release.bin").write_bytes(b"release")

    monkeypatch.setattr(flash, "BUILD_BIN", str(build_bin))
    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(firmware_dir))

    assert flash.find_firmware() == str(build_bin)


def test_find_local_build_returns_none_when_missing(monkeypatch, tmp_path):
    monkeypatch.setattr(flash, "BUILD_BIN", str(tmp_path / "nonexistent.bin"))
    assert flash.find_local_build() is None


def test_find_local_build_returns_path_when_exists(monkeypatch, tmp_path):
    build_bin = tmp_path / "firmware.bin"
    build_bin.write_bytes(b"test")
    monkeypatch.setattr(flash, "BUILD_BIN", str(build_bin))
    assert flash.find_local_build() == str(build_bin)


def test_verify_checksum_valid(tmp_path):

    firmware = tmp_path / "firmware.bin"
    firmware.write_bytes(b"test firmware content")
    sha = hashlib.sha256(b"test firmware content").hexdigest()

    checksums = tmp_path / "SHA256SUMS.txt"
    checksums.write_text(f"{sha}  firmware.bin\n")

    assert flash._verify_checksum(str(firmware), str(checksums)) is True


def test_verify_checksum_mismatch(tmp_path):
    firmware = tmp_path / "firmware.bin"
    firmware.write_bytes(b"test firmware content")

    checksums = tmp_path / "SHA256SUMS.txt"
    checksums.write_text(f"{'0' * 64}  firmware.bin\n")

    assert flash._verify_checksum(str(firmware), str(checksums)) is False


def test_verify_checksum_missing_entry(tmp_path):
    firmware = tmp_path / "firmware.bin"
    firmware.write_bytes(b"test")

    checksums = tmp_path / "SHA256SUMS.txt"
    checksums.write_text("abc123  other.bin\n")

    # Missing entry returns True (warns but doesn't block)
    assert flash._verify_checksum(str(firmware), str(checksums)) is True


def test_gh_repo_from_ssh_remote(monkeypatch):
    def fake_run(cmd, **kwargs):
        if "upstream" in cmd:
            result = SimpleNamespace(stdout="git@github.com:owner/repo.git\n", returncode=0)
            return result
        raise subprocess.CalledProcessError(1, cmd)

    import subprocess
    monkeypatch.setattr(subprocess, "run", fake_run)

    assert flash._gh_repo() == "owner/repo"


def test_gh_repo_from_https_remote(monkeypatch):
    def fake_run(cmd, **kwargs):
        if "upstream" in cmd:
            raise subprocess.CalledProcessError(1, cmd)
        if "origin" in cmd:
            result = SimpleNamespace(stdout="https://github.com/user/project.git\n", returncode=0)
            return result
        raise subprocess.CalledProcessError(1, cmd)

    import subprocess
    monkeypatch.setattr(subprocess, "run", fake_run)

    assert flash._gh_repo() == "user/project"


class FakeResponse:
    def __init__(self, data):
        self._data = data
    def read(self):
        return self._data
    def __enter__(self):
        return self
    def __exit__(self, *a):
        pass


def test_fetch_release_meta_latest(monkeypatch):
    release_data = {"tag_name": "v1.0", "assets": []}

    def fake_urlopen(req):
        assert "/releases/latest" in req.full_url
        return FakeResponse(json.dumps(release_data).encode())

    monkeypatch.setattr(flash.urllib.request, "urlopen", fake_urlopen)

    result = flash._fetch_release_meta("owner/repo")
    assert result["tag_name"] == "v1.0"


def test_fetch_release_meta_by_tag(monkeypatch):
    release_data = {"tag_name": "v2.0", "assets": []}

    def fake_urlopen(req):
        assert "/releases/tags/v2.0" in req.full_url
        return FakeResponse(json.dumps(release_data).encode())

    monkeypatch.setattr(flash.urllib.request, "urlopen", fake_urlopen)

    result = flash._fetch_release_meta("owner/repo", tag="v2.0")
    assert result["tag_name"] == "v2.0"


def test_fetch_release_meta_404(monkeypatch):
    import urllib.error

    def fake_urlopen(req):
        raise urllib.error.HTTPError(req.full_url, 404, "Not Found", {}, None)

    monkeypatch.setattr(flash.urllib.request, "urlopen", fake_urlopen)

    assert flash._fetch_release_meta("owner/repo", tag="v99") is None


def test_download_release_verifies_checksum(monkeypatch, tmp_path):
    firmware_content = b"real firmware bytes"
    sha = hashlib.sha256(firmware_content).hexdigest()
    checksums_content = f"{sha}  {flash.RELEASE_ASSET}\n".encode()

    release_data = {
        "tag_name": "v1.0",
        "assets": [
            {"name": flash.RELEASE_ASSET, "browser_download_url": "https://example.com/fw.bin"},
            {"name": flash.CHECKSUMS_ASSET, "browser_download_url": "https://example.com/sums.txt"},
        ],
    }

    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(tmp_path))
    monkeypatch.setattr(flash, "_gh_repo", lambda: "owner/repo")
    monkeypatch.setattr(flash, "_fetch_release_meta", lambda repo, tag=None: release_data)

    download_calls = []

    def fake_download(url, dest):
        download_calls.append(url)
        if url.endswith("fw.bin"):
            Path(dest).write_bytes(firmware_content)
        elif url.endswith("sums.txt"):
            Path(dest).write_bytes(checksums_content)
        return True

    monkeypatch.setattr(flash, "_download_file", fake_download)

    result = flash.download_release(tag="v1.0")
    assert result == str(tmp_path / flash.RELEASE_ASSET)
    assert len(download_calls) == 2
    # Checksums file should be cleaned up
    assert not (tmp_path / flash.CHECKSUMS_ASSET).exists()


def test_download_release_rejects_bad_checksum(monkeypatch, tmp_path):
    firmware_content = b"real firmware bytes"
    checksums_content = f"{'0' * 64}  {flash.RELEASE_ASSET}\n".encode()

    release_data = {
        "tag_name": "v1.0",
        "assets": [
            {"name": flash.RELEASE_ASSET, "browser_download_url": "https://example.com/fw.bin"},
            {"name": flash.CHECKSUMS_ASSET, "browser_download_url": "https://example.com/sums.txt"},
        ],
    }

    monkeypatch.setattr(flash, "FIRMWARE_DIR", str(tmp_path))
    monkeypatch.setattr(flash, "_gh_repo", lambda: "owner/repo")
    monkeypatch.setattr(flash, "_fetch_release_meta", lambda repo, tag=None: release_data)

    def fake_download(url, dest):
        if url.endswith("fw.bin"):
            Path(dest).write_bytes(firmware_content)
        elif url.endswith("sums.txt"):
            Path(dest).write_bytes(checksums_content)
        return True

    monkeypatch.setattr(flash, "_download_file", fake_download)

    result = flash.download_release(tag="v1.0")
    assert result is None
    # Bad firmware should be deleted
    assert not (tmp_path / flash.RELEASE_ASSET).exists()
