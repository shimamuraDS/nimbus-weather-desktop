#!/usr/bin/env python3
"""Build reproducible Nimbus Weather ZIP and MSI release assets.

The release tree is defined by CMake install rules. Qt's deployment API adds
the matching Qt/QML plugins and compiler runtime, then CPack creates both a
portable ZIP and a WiX 4 MSI from that same tree.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DIST = ROOT / "dist"
TOOLS = ROOT / ".tools"
WIX_VERSION = "4.0.4"
WIX_EXTENSION = f"WixToolset.UI.wixext/{WIX_VERSION}"
WIX_EXTENSION_LISTING = f"WixToolset.UI.wixext {WIX_VERSION}"

_VERSION_MATCH = re.search(
    r"project\(NimbusWeather\s+VERSION\s+([0-9]+(?:\.[0-9]+){1,3})",
    (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"),
)
if not _VERSION_MATCH:
    raise RuntimeError("Could not read the NimbusWeather version from CMakeLists.txt")
PROJECT_VERSION = _VERSION_MATCH.group(1)

VARIANTS = {
    "AI": True,
    "Standard": False,
}


def run(command: list[str], *, env: dict[str, str] | None = None) -> None:
    print("  $", subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, cwd=ROOT, env=env, check=True)


def find_qt_root(explicit: str | None) -> Path:
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit))
    if os.environ.get("QT_ROOT"):
        candidates.append(Path(os.environ["QT_ROOT"]))

    # Reuse the Qt selected by an existing release build when available.
    for cache in (ROOT / "build-audit-ai" / "CMakeCache.txt",
                  ROOT / "build-ai" / "CMakeCache.txt"):
        if cache.is_file():
            for line in cache.read_text(encoding="utf-8", errors="ignore").splitlines():
                if line.startswith("Qt6_DIR:PATH="):
                    qt6_dir = Path(line.split("=", 1)[1])
                    candidates.append(qt6_dir.parents[2])
                    break

    for candidate in candidates:
        candidate = candidate.expanduser().resolve()
        if (candidate / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file():
            return candidate

    raise RuntimeError(
        "Qt 6 was not found. Pass --qt-root or set QT_ROOT to the Qt kit "
        "directory, for example D:/Qt/6.8.3/mingw_64."
    )


def find_compiler(qt_root: Path, explicit: str | None) -> Path:
    if explicit:
        compiler = Path(explicit).expanduser().resolve()
        if compiler.is_file():
            return compiler
        raise RuntimeError(f"C++ compiler does not exist: {compiler}")

    # Prefer the compiler already paired with this exact Qt kit by CMake.
    for cache in (ROOT / "build-audit-ai" / "CMakeCache.txt",
                  ROOT / "build-ai" / "CMakeCache.txt"):
        if cache.is_file():
            cached_compiler = None
            cached_qt = None
            for line in cache.read_text(encoding="utf-8", errors="ignore").splitlines():
                if line.startswith("CMAKE_CXX_COMPILER:STRING="):
                    cached_compiler = Path(line.split("=", 1)[1])
                elif line.startswith("Qt6_DIR:PATH="):
                    cached_qt = Path(line.split("=", 1)[1]).parents[2]
            if (cached_compiler and cached_qt
                    and cached_qt.resolve() == qt_root.resolve()
                    and cached_compiler.is_file()):
                return cached_compiler.resolve()

    qt_install_root = qt_root.parent.parent
    candidates = list(qt_install_root.glob("Tools/mingw*_64/bin/g++.exe"))
    candidates.sort(
        key=lambda path: tuple(int(value) for value in re.findall(r"\d+", path.as_posix())),
        reverse=True,
    )
    if not candidates:
        raise RuntimeError("A matching Qt MinGW compiler was not found; pass --compiler.")
    return candidates[0].resolve()


def find_ninja(qt_root: Path, explicit: str | None) -> Path:
    if explicit:
        ninja = Path(explicit).expanduser().resolve()
        if ninja.is_file():
            return ninja
        raise RuntimeError(f"Ninja does not exist: {ninja}")

    from_path = shutil.which("ninja")
    if from_path:
        return Path(from_path).resolve()

    bundled = qt_root.parent.parent / "Tools" / "Ninja" / "ninja.exe"
    if bundled.is_file():
        return bundled.resolve()
    raise RuntimeError("Ninja was not found; pass --ninja.")


def wix_environment(wix_root: Path) -> dict[str, str]:
    env = os.environ.copy()
    env["WIX"] = str(wix_root)
    # Keep a version-isolated extension cache next to the pinned WiX binary.
    # Reusing the repository-level .wix directory would let WiX 7 extensions
    # shadow the required WiX 4 assemblies.
    env["WIX_EXTENSIONS"] = str(wix_root / "extension-cache")
    env["PATH"] = str(wix_root) + os.pathsep + env.get("PATH", "")
    return env


def ensure_wix4(bootstrap: bool) -> tuple[Path, dict[str, str]]:
    wix_root = (TOOLS / "wix4").resolve()
    wix_exe = wix_root / "wix.exe"
    env = wix_environment(wix_root)

    if not wix_exe.is_file():
        if not bootstrap:
            raise RuntimeError(
                f"WiX {WIX_VERSION} is missing at {wix_root}. "
                "Run without --no-bootstrap-wix to install the pinned local tool."
            )
        wix_root.mkdir(parents=True, exist_ok=True)
        run([
            "dotnet", "tool", "install", "--tool-path", str(wix_root),
            "wix", "--version", WIX_VERSION,
        ], env=env)

    listing = subprocess.run(
        [str(wix_exe), "extension", "list", "--global"],
        cwd=ROOT,
        env=env,
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    if WIX_EXTENSION_LISTING not in listing:
        run([str(wix_exe), "extension", "add", "--global", WIX_EXTENSION], env=env)

    return wix_root, env


def configure_and_build(
    variant: str,
    qt_root: Path,
    compiler: Path,
    ninja: Path,
    wix_root: Path,
    env: dict[str, str],
    run_tests: bool,
    build_msi: bool,
) -> list[Path]:
    with_llm = VARIANTS[variant]
    build_dir = ROOT / f"build-release-{variant.lower()}"
    stage_dir = DIST / "stage" / variant.lower()

    print(f"\n=== Nimbus Weather {variant} ===", flush=True)
    run([
        "cmake", "-Wno-dev", "-S", str(ROOT), "-B", str(build_dir), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DCMAKE_PREFIX_PATH={qt_root}",
        f"-DCMAKE_CXX_COMPILER={compiler}",
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DWITH_LLM={'ON' if with_llm else 'OFF'}",
        f"-DNIMBUS_WIX_ROOT={wix_root}",
    ], env=env)
    run(["cmake", "--build", str(build_dir), "--parallel"], env=env)

    if run_tests:
        run([
            "ctest", "--test-dir", str(build_dir),
            "--output-on-failure", "-C", "Release",
        ], env=env)

    if stage_dir.exists():
        shutil.rmtree(stage_dir)
    stage_dir.mkdir(parents=True, exist_ok=True)
    run([
        "cmake", "--install", str(build_dir),
        "--prefix", str(stage_dir), "--config", "Release",
    ], env=env)

    package_stem = f"NimbusWeather-{PROJECT_VERSION}-win64-{variant}"
    artifacts: list[Path] = []
    for suffix in (".zip", ".msi"):
        old_asset = DIST / f"{package_stem}{suffix}"
        if old_asset.is_file():
            old_asset.unlink()

    run([
        "cpack", "--config", str(build_dir / "CPackConfig.cmake"),
        "-G", "ZIP", "-B", str(DIST), "-C", "Release",
    ], env=env)
    artifacts.append(DIST / f"{package_stem}.zip")

    if build_msi:
        run([
            "cpack", "--config", str(build_dir / "CPackConfig.cmake"),
            "-G", "WIX", "-B", str(DIST), "-C", "Release",
        ], env=env)
        artifacts.append(DIST / f"{package_stem}.msi")

    missing = [str(path) for path in artifacts if not path.is_file()]
    if missing:
        raise RuntimeError("CPack did not create expected artifacts: " + ", ".join(missing))
    return artifacts


def write_checksums(artifacts: list[Path]) -> Path:
    checksum_path = DIST / "SHA256SUMS.txt"
    lines = []
    for artifact in sorted(artifacts, key=lambda path: path.name):
        digest = hashlib.sha256(artifact.read_bytes()).hexdigest()
        lines.append(f"{digest}  {artifact.name}")
    checksum_path.write_text("\n".join(lines) + "\n", encoding="ascii")
    return checksum_path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--variant", choices=("all", "ai", "standard"), default="all",
        help="edition to package (default: all)",
    )
    parser.add_argument("--qt-root", help="Qt kit root containing lib/cmake/Qt6")
    parser.add_argument("--compiler", help="path to the matching g++.exe")
    parser.add_argument("--ninja", help="path to ninja.exe")
    parser.add_argument("--skip-tests", action="store_true")
    parser.add_argument("--skip-msi", action="store_true")
    parser.add_argument("--no-bootstrap-wix", action="store_true")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        qt_root = find_qt_root(args.qt_root)
        compiler = find_compiler(qt_root, args.compiler)
        ninja = find_ninja(qt_root, args.ninja)

        if args.skip_msi:
            wix_root = (TOOLS / "wix4").resolve()
            env = os.environ.copy()
        else:
            wix_root, env = ensure_wix4(not args.no_bootstrap_wix)

        selected = list(VARIANTS)
        if args.variant != "all":
            selected = ["AI" if args.variant == "ai" else "Standard"]

        DIST.mkdir(parents=True, exist_ok=True)
        artifacts: list[Path] = []
        for variant in selected:
            artifacts.extend(configure_and_build(
                variant, qt_root, compiler, ninja, wix_root, env,
                not args.skip_tests, not args.skip_msi,
            ))

        checksum_path = write_checksums(artifacts)
        print("\nRelease assets:")
        for artifact in artifacts:
            print(f"  {artifact} ({artifact.stat().st_size / 1024 / 1024:.1f} MiB)")
        print(f"  {checksum_path}")
        return 0
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
