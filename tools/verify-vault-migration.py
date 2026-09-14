#!/usr/bin/env python3
"""Verify Vault JSONC migration against a legacy git revision (requires pyjson5).

Use --write once to mechanically convert the legacy definitions. The comparison
uses the exact concatenated D bytes consumed by the old reader, not visual rows.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess

import pyjson5


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--legacy-ref", required=True)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    legacy = subprocess.check_output(
        ["git", "show", f"{args.legacy_ref}:lib/edit/VaultDefinitions.txt"], cwd=root
    ).decode("utf-8")
    records = []
    comments = []
    pending = []
    for line in legacy.splitlines():
        if line.startswith("#"):
            pending.append(line[1:].strip())
        elif line.startswith("N:"):
            _, index, name = line.split(":", 2)
            record = {"id": int(index), "name": name, "layout": []}
            records.append(record)
            comments.append(pending)
            pending = []
        elif line.startswith("X:"):
            record.update(zip(("type", "rating", "height", "width"), map(int, line[2:].split(":")), strict=True))
        elif line.startswith("D:"):
            record["layout"].append(line[2:])
        elif line and not line.startswith("V:"):
            raise ValueError(f"Unknown directive: {line}")

    expected = []
    for record in records:
        flat = "".join(record["layout"])
        if len(flat.encode("ascii")) != record["height"] * record["width"]:
            raise ValueError(f"Invalid total layout size: {record['id']}")
        expected.append({**record, "layout": flat})
        record["layout"] = [flat[i:i + record["width"]] for i in range(0, len(flat), record["width"])]

    target = root / "lib/edit/VaultDefinitions.jsonc"
    if args.write:
        lines = ["// Vault definitions. See VaultDefinitions.md; spaces in layout are significant.", '{', '  "vaults": [']
        for index, (record, notes) in enumerate(zip(records, comments, strict=True)):
            # The old file header describes the removed raw/text format. Keep
            # record annotations, but replace that header with current documentation.
            if index:
                lines.extend("    // " + note for note in notes)
            if record["id"] == 114:
                lines.append("    // Legacy ID 114: reflowed to declared 17x12; concatenated layout is unchanged.")
            ordered = {key: record[key] for key in ("id", "name", "type", "rating", "height", "width", "layout")}
            block = json.dumps(ordered, ensure_ascii=False, indent=2).splitlines()
            lines.extend("    " + line for line in block)
            if index + 1 != len(records):
                lines[-1] += ","
        lines.extend(["  ]", "}", ""])
        # Format before replacing the file: a missing/failing formatter must not
        # leave an unformatted definition behind. Verification alone needs no Node.
        prettier = shutil.which("prettier")
        npx = shutil.which("npx")
        if prettier:
            command = [prettier]
        elif npx:
            command = [npx, "--no-install", "prettier"]
        else:
            raise RuntimeError("--write requires Prettier (install with npm install --global prettier)")
        formatted = subprocess.check_output(
            [*command, "--stdin-filepath", str(target)],
            input="\n".join(lines).encode("utf-8"), cwd=root,
        ).decode("utf-8")
        target.write_text(formatted, encoding="utf-8", newline="\n")

    data = pyjson5.decode(target.read_text(encoding="utf-8"))
    actual = [{**r, "layout": "".join(r["layout"])} for r in data["vaults"]]
    if actual != expected:
        raise ValueError("Vault IDs/order, metadata or layout bytes changed")
    for r in data["vaults"]:
        if len(r["layout"]) != r["height"] or any(len(row.encode("ascii")) != r["width"] for row in r["layout"]):
            raise ValueError(f"Invalid row dimensions: {r['id']}")
    print(f"Identical: {len(expected)} vaults, {sum(len(r['layout']) for r in expected)} layout bytes")


if __name__ == "__main__":
    main()
