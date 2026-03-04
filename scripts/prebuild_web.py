"""PlatformIO pre-build hook: regenerate dashboard_gz.h if web sources changed."""
import os
import subprocess
import sys

Import("env")  # noqa: F821 — PlatformIO injects this

WEB_DIR = os.path.join(env.subst("$PROJECT_DIR"), "web")
HEADER = os.path.join(env.subst("$PROJECT_DIR"), "src", "web", "dashboard_gz.h")
SRC_DIR = os.path.join(WEB_DIR, "src")


def newest_mtime(directory):
    """Return the newest mtime of any file under directory."""
    newest = 0
    for root, _dirs, files in os.walk(directory):
        for f in files:
            t = os.path.getmtime(os.path.join(root, f))
            if t > newest:
                newest = t
    return newest


CONFIG_FILES = [
    os.path.join(WEB_DIR, "vite.config.ts"),
    os.path.join(WEB_DIR, "package.json"),
    os.path.join(WEB_DIR, "package-lock.json"),
    os.path.join(WEB_DIR, "tsconfig.json"),
    os.path.join(WEB_DIR, "index.html"),
]


def should_rebuild():
    if not os.path.exists(HEADER):
        return True
    if not os.path.isdir(SRC_DIR):
        return False  # no web sources, use committed header
    header_time = os.path.getmtime(HEADER)
    if newest_mtime(SRC_DIR) > header_time:
        return True
    for f in CONFIG_FILES:
        if os.path.exists(f) and os.path.getmtime(f) > header_time:
            return True
    return False


def has_node():
    try:
        subprocess.run(
            ["node", "--version"],
            capture_output=True,
            check=True,
        )
        return True
    except (FileNotFoundError, subprocess.CalledProcessError):
        return False


if should_rebuild():
    if has_node():
        print("[prebuild_web] Web sources changed, rebuilding dashboard...")
        node_modules = os.path.join(WEB_DIR, "node_modules")
        if not os.path.isdir(node_modules):
            subprocess.run(
                ["npm", "ci"],
                cwd=WEB_DIR,
                check=True,
            )
        subprocess.run(
            ["npm", "run", "build"],
            cwd=WEB_DIR,
            check=True,
        )
        print("[prebuild_web] dashboard_gz.h regenerated")
    else:
        if os.path.exists(HEADER):
            print("[prebuild_web] Node.js not found, using committed header")
        else:
            print(
                "[prebuild_web] ERROR: No Node.js and no committed header!",
                file=sys.stderr,
            )
            env.Exit(1)
else:
    print("[prebuild_web] dashboard_gz.h is up to date")
