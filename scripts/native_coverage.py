"""Add gcov coverage flags on Linux (CI) where GCC is available.

macOS Apple Clang does not ship libgcov, so these flags are skipped
locally to avoid linker errors. Coverage reports are generated in CI only.
"""
import platform
Import("env")

if platform.system() == "Linux":
    env.Append(
        CCFLAGS=["-fprofile-arcs", "-ftest-coverage"],
        LINKFLAGS=["-lgcov", "-fprofile-arcs", "-ftest-coverage"],
    )
