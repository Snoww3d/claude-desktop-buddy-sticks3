#!/usr/bin/env python3
"""
Flash a prepped character pack via USB (pio run -t uploadfs).
Faster than the BLE drop target when you're iterating on a character.

Usage:
  python3 tools/flash_character.py <character-dir> [--env <pio-env>]

Examples:
  python3 tools/flash_character.py characters/bufo
  python3 tools/flash_character.py characters/bufo --env m5stick-s3

If --env is omitted, uses platformio.ini's default_envs. Pass --env
explicitly when the default env doesn't match the board you have plugged
in (e.g. the default is m5stickc-plus but you're flashing a StickS3).
"""
import argparse, json, sys, shutil, subprocess
from pathlib import Path

PROJECT = Path(__file__).resolve().parent.parent
DATA    = PROJECT / "data" / "characters"
CAP     = 1_800_000


def flash(src: Path, env: str | None) -> None:
    if not (src / "manifest.json").exists():
        sys.exit(f"no manifest.json in {src} — run tools/prep_character.py first")
    name = json.loads((src / "manifest.json").read_text())["name"]

    total = sum(f.stat().st_size for f in src.iterdir() if f.is_file())
    if total > CAP:
        sys.exit(f"{total:,} bytes — over the {CAP:,} LittleFS cap")

    # uploadfs flashes everything under data/; the firmware only reads one
    # character at a time, so a stale sibling just wastes partition space.
    if DATA.exists():
        shutil.rmtree(DATA)
    dst = DATA / name
    shutil.copytree(src, dst)
    print(f"staged {name}: {total:,} bytes -> {dst}")

    cmd = ["pio", "run", "-t", "uploadfs"]
    if env:
        cmd.extend(["-e", env])
    subprocess.run(cmd, cwd=PROJECT, check=True)
    print(f"\nflashed. on the stick: hold A -> settings -> species -> GIF")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("character", help="path to character pack directory")
    parser.add_argument("--env", help="PlatformIO env (e.g. m5stick-s3, m5stickc-plus)")
    args = parser.parse_args()
    flash(Path(args.character).resolve(), args.env)
