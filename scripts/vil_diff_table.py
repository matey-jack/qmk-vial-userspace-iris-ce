#!/usr/bin/env python3
"""Print a Markdown table of the JSON differences between two revisions of
each *.vil file under init_vial/.

Usage:
    vil_diff_table.py <base-rev> [<head-rev>]

<head-rev> defaults to HEAD. Revisions are resolved with `git show`, so they
can be any commit-ish (SHA, branch, tag). Prints nothing (and exits 0) if no
init_vial/**/*.vil file differs between the two revisions.
"""
import json
import subprocess
import sys


def git(*args):
    return subprocess.run(
        ["git", *args], capture_output=True, text=True, check=True
    ).stdout


def changed_vil_paths(base, head):
    out = git("diff", "--name-only", base, head, "--", "init_vial")
    return sorted(p for p in out.splitlines() if p.endswith(".vil"))


def read_json_at(rev, path):
    result = subprocess.run(
        ["git", "show", f"{rev}:{path}"], capture_output=True, text=True
    )
    if result.returncode != 0:
        return None
    return json.loads(result.stdout)


def flatten(value):
    if isinstance(value, list):
        out = []
        for item in value:
            out.extend(flatten(item))
        return out
    if isinstance(value, dict):
        out = []
        for item in value.values():
            out.extend(flatten(item))
        return out
    return [value]


def describe(value):
    if isinstance(value, list):
        flat = flatten(value)
        if flat and all(v == flat[0] for v in flat):
            return f"*{len(flat)}-entry, all `{flat[0]}`*"
        return f"*list of {len(value)} items*"
    if isinstance(value, dict):
        return f"*object with {len(value)} keys*"
    return f"`{value}`"


def path_join(base, key):
    if isinstance(key, int):
        return f"{base}[{key}]"
    if key.isidentifier():
        return f"{base}.{key}"
    return f'{base}["{key}"]'


def diff_json(before, after, path, rows):
    if isinstance(before, dict) and isinstance(after, dict):
        keys = list(before.keys())
        keys += [k for k in after.keys() if k not in before]
        for k in keys:
            diff_json(before.get(k), after.get(k), path_join(path, k), rows)
        return
    if isinstance(before, list) and isinstance(after, list):
        for i in range(max(len(before), len(after))):
            b = before[i] if i < len(before) else before
            a = after[i] if i < len(after) else after
            if i >= len(before):
                rows.append((path_join(path, i), "*(none)*", describe(a)))
            elif i >= len(after):
                rows.append((path_join(path, i), describe(b), "*(none)*"))
            else:
                diff_json(before[i], after[i], path_join(path, i), rows)
        return
    if before != after:
        before_cell = describe(before) if before is not None else "*(none)*"
        after_cell = describe(after) if after is not None else "*(none)*"
        rows.append((path, before_cell, after_cell))


def render_table(rows):
    lines = ["| JSONPath | Before | After |", "|---|---|---|"]
    for path, before, after in rows:
        lines.append(f"| `{path}` | {before} | {after} |")
    return "\n".join(lines)


def main():
    if not 1 <= len(sys.argv) - 1 <= 2:
        print(__doc__, file=sys.stderr)
        sys.exit(2)
    base = sys.argv[1]
    head = sys.argv[2] if len(sys.argv) > 2 else "HEAD"

    sections = []
    for path in changed_vil_paths(base, head):
        before = read_json_at(base, path)
        after = read_json_at(head, path)
        rows = []
        diff_json(before, after, "$", rows)
        if not rows:
            continue
        sections.append(f"### `{path}`\n\n{render_table(rows)}")

    if sections:
        print("\n\n".join(sections))


if __name__ == "__main__":
    main()
