#!/usr/bin/env python3
"""Compare every old W/S entry with the JSONC migration (requires pyjson5).

Usage: python3 tools/verify-class-skill-migration.py --legacy-ref <pre-migration commit>
"""

import argparse
from pathlib import Path
import subprocess

import pyjson5


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-ref", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    legacy = subprocess.check_output(
        ["git", "show", f"{args.legacy_ref}:lib/edit/ClassSkillDefinitions.txt"],
        cwd=root,
    ).decode("utf-8")
    expected = {}
    ids = []
    for line in legacy.splitlines():
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        tag, *values = text.split(":")
        values = list(map(int, values))
        if tag == "N":
            class_id = values[0]
            ids.append(class_id)
        elif tag in ("W", "S"):
            key = (class_id, tag, *values[:-2])
            if key in expected:
                raise ValueError(f"Duplicate legacy entry: {key}")
            expected[key] = tuple(values[-2:])
        else:
            raise ValueError(f"Unknown legacy directive: {text}")

    data = pyjson5.decode((root / "lib/edit/ClassSkillDefinitions.jsonc").read_text(encoding="utf-8"))
    if [entry["id"] for entry in data["classes"]] != ids:
        raise ValueError("Class IDs/order changed")
    actual = {}
    for entry in data["classes"]:
        class_id = entry["id"]
        for tval, name in enumerate(("BOW", "DIGGING", "HAFTED", "POLEARM", "SWORD")):
            weapon = entry["weapons"][name]
            for sval, pair in enumerate(zip(weapon["start_ranks"], weapon["max_ranks"], strict=True)):
                actual[(class_id, "W", tval, sval)] = pair
        for kind, name in enumerate(("MARTIAL_ARTS", "TWO_WEAPON", "RIDING", "SHIELD")):
            skill = entry["skills"][name]
            actual[(class_id, "S", kind)] = (skill["start_exp"], skill["max_exp"])
    differences = [key for key in expected.keys() | actual.keys() if expected.get(key) != actual.get(key)]
    if differences:
        raise ValueError(f"Changed/missing entries: {differences[:20]}")
    print(f"Identical: {len(ids)} classes, {len(expected)} weapon/skill entries")


if __name__ == "__main__":
    main()
