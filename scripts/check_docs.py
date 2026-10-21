#!/usr/bin/env python3
"""Validate documentation links, anchors, and source-symbol references.

This repository cites a file and a symbol (for example
`[Game.cpp](Sources/Core/Game.cpp)` — `Game::update`) rather than a line
number. Line numbers rot silently when code moves; symbols can be checked.

Rules
-----
1. Every relative link resolves to a real file.
2. No ``#L<number>`` line anchors.
3. A Markdown heading fragment must match a real heading.
4. A link into C++/Objective-C++/script source must be followed by an
   inline-code symbol that occurs in the linked file.
5. Every QnA document is registered in ``Docs/QnA/README.md`` with a date, and
   every registered entry resolves to a real document.

Usage:
    python3 scripts/check_docs.py
    python3 scripts/check_docs.py --changed
    python3 scripts/check_docs.py --changed --base origin/main
    python3 scripts/check_docs.py --statuses

Standard library only; no third-party dependencies.
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CODE_PREFIXES = ("Sources/", "Checks/", "scripts/")
CODE_FILES = ("CMakeLists.txt", "Makefile")
CODE_EXTENSIONS = (".cpp", ".hpp", ".mm", ".h", ".sh", ".py", ".cmake")
SKIP_DIRS = {".git", ".build", "build", "dist", "Resources", "__pycache__", ".venv"}

LINK_RE = re.compile(r"(?<!!)\[([^\]]*)\]\(([^)\s]+)\)")
INLINE_CODE_RE = re.compile(r"`([^`]+)`")
LIST_OR_HEADING_RE = re.compile(r"^\s*(#{1,6}\s|[-*+]\s|\d+\.\s|\||```)")
DECL_RE = re.compile(r"^\s*(?:class|struct|enum|namespace|void|int|auto|float|bool)\s+([A-Za-z_][A-Za-z0-9_:]*)")

# The QnA index is a registry: every question document must be listed there, and
# every entry must be dated. Entry shape:
#     | [Question text](file.md) | YYYY-MM-DD |
QNA_DIR = "Docs/QnA"
QNA_INDEX = "Docs/QnA/README.md"
QNA_ENTRY_RE = re.compile(r"^\|\s*\[[^\]]+\]\(([^)\s]+)\)\s*\|\s*(\d{4}-\d{2}-\d{2})\s*\|\s*$")


class Doc:
    def __init__(self, path: str) -> None:
        self.path = path
        with open(os.path.join(ROOT, path), encoding="utf-8") as handle:
            self.lines = handle.read().splitlines()
        self.fenced = [False] * len(self.lines)
        in_fence = False
        for index, line in enumerate(self.lines):
            if line.lstrip().startswith("```"):
                self.fenced[index] = True
                in_fence = not in_fence
            else:
                self.fenced[index] = in_fence


def repo_docs() -> list[Doc]:
    docs = []
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = sorted(d for d in dirnames if d not in SKIP_DIRS and not d.startswith("."))
        for name in sorted(filenames):
            if name.endswith(".md"):
                docs.append(Doc(os.path.relpath(os.path.join(dirpath, name), ROOT)))
    return docs


def all_files() -> set[str]:
    files: set[str] = set()
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith(".")]
        for name in filenames:
            files.add(os.path.relpath(os.path.join(dirpath, name), ROOT))
    return files


def heading_slugs(doc: Doc) -> set[str]:
    slugs: set[str] = set()
    seen: dict[str, int] = {}
    in_fence = False
    for line in doc.lines:
        if line.lstrip().startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence or not line.startswith("#"):
            continue
        text = line.lstrip("#").strip()
        if not text:
            continue
        text = re.sub(r"`([^`]*)`", r"\1", text)
        text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
        text = text.lower()
        text = re.sub(r"[^\w\- ]", "", text)
        slug = text.replace(" ", "-")
        count = seen.get(slug, 0)
        seen[slug] = count + 1
        slugs.add(slug if count == 0 else f"{slug}-{count}")
    return slugs


def is_code_target(target: str) -> bool:
    if target.startswith(CODE_PREFIXES) or target in CODE_FILES:
        return True
    return target.endswith(CODE_EXTENSIONS)


def link_context(doc: Doc, index: int, end_column: int) -> str:
    parts = [doc.lines[index][end_column:]]
    cursor = index + 1
    while cursor < len(doc.lines):
        line = doc.lines[cursor]
        if not line.strip() or LIST_OR_HEADING_RE.match(line):
            break
        parts.append(line)
        cursor += 1
    return " ".join(parts)


def cited_symbol(text: str) -> str | None:
    for match in INLINE_CODE_RE.finditer(text):
        raw = match.group(1).strip()
        if raw.endswith((".md", ".plist", ".png", ".icns", ".dmg")) or "/" in raw:
            continue
        name = raw.split("(")[0].strip()
        name = name.split("::")[-1].split(".")[-1].strip()
        name = name.rstrip(".,;")
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name or ""):
            return name
    return None


def symbol_in_file(symbol: str, path: str) -> bool:
    try:
        with open(os.path.join(ROOT, path), encoding="utf-8") as handle:
            text = handle.read()
    except OSError:
        return False
    return re.search(rf"\b{re.escape(symbol)}\b", text) is not None


def validate_qna_registry() -> list[str]:
    """Every QnA document must appear in the index with a date, and vice versa."""
    if QNA_INDEX not in {doc.path for doc in repo_docs()}:
        return [f"{QNA_INDEX} does not exist"]
    index = Doc(QNA_INDEX)
    errors: list[str] = []
    registered: set[str] = set()
    for number, line in enumerate(index.lines, start=1):
        if not line.lstrip().startswith("|"):
            continue
        if "](http" in line or "---" in line or "(" not in line:
            # Foreign links and the table header are not QnA entries.
            continue
        match = QNA_ENTRY_RE.match(line.rstrip())
        if match is None:
            errors.append(
                f"{QNA_INDEX}:{number}: malformed QnA entry; expected "
                "'| [Question](file.md) | YYYY-MM-DD |'"
            )
            continue
        target, _ = match.group(1), match.group(2)
        if target.startswith(("http://", "https://", "#")):
            continue
        resolved = os.path.normpath(os.path.join(QNA_DIR, target))
        if resolved == QNA_INDEX:
            continue
        registered.add(resolved)

    for doc in repo_docs():
        if not doc.path.startswith(QNA_DIR + "/") or doc.path == QNA_INDEX:
            continue
        if doc.path not in registered:
            errors.append(
                f"{doc.path}: not registered in {QNA_INDEX}; add a dated table row "
                "so the QnA index stays complete"
            )
    return errors


def validate() -> tuple[list[str], dict[str, list[tuple[str, str]]]]:
    errors: list[str] = []
    references: dict[str, list[tuple[str, str]]] = {}
    known_files = all_files()
    slugs: dict[str, set[str]] = {}

    for doc in repo_docs():
        for index, line in enumerate(doc.lines):
            if doc.fenced[index]:
                continue
            for match in LINK_RE.finditer(line):
                label, target = match.group(1), match.group(2)
                if target.startswith(("http://", "https://", "mailto:", "#")):
                    continue
                path, _, fragment = target.partition("#")
                where = f"{doc.path}:{index + 1}"
                resolved = (
                    os.path.normpath(os.path.join(os.path.dirname(doc.path), path))
                    if path
                    else doc.path
                )
                if resolved not in known_files:
                    errors.append(f"{where}: link target does not exist: {target}")
                    continue

                if fragment:
                    if re.match(r"L\d+", fragment):
                        errors.append(
                            f"{where}: line anchor '#{fragment}' is not allowed; "
                            "link to the file and cite the symbol"
                        )
                        continue
                    if resolved.endswith(".md"):
                        if resolved not in slugs:
                            slugs[resolved] = heading_slugs(Doc(resolved))
                        if fragment not in slugs[resolved]:
                            errors.append(
                                f"{where}: no heading matches anchor '#{fragment}' in {resolved}"
                            )

                if not is_code_target(resolved):
                    continue
                if "docs-check: ignore" in line:
                    continue

                context = link_context(doc, index, match.end())
                symbol = cited_symbol(context)
                if symbol is None and label.strip():
                    symbol = cited_symbol(f"`{label}`") if "`" in label else None
                if symbol is None:
                    errors.append(
                        f"{where}: link to {resolved} must cite a symbol in backticks "
                        "(for example `Game::update`) so it can be verified"
                    )
                    continue
                if not symbol_in_file(symbol, resolved):
                    errors.append(
                        f"{where}: '{symbol}' does not occur in {resolved}; "
                        "the reference has drifted or the symbol was renamed"
                    )
                    continue
                references.setdefault(resolved, []).append((doc.path, symbol))
    return errors, references


def git(*args: str) -> str:
    result = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, check=False)
    return result.stdout


def changed_files(base: str) -> list[str]:
    files = [line for line in git("diff", "--name-only", base).splitlines() if line]
    files += [line for line in git("ls-files", "--others", "--exclude-standard").splitlines() if line]
    return sorted(set(files))


def removed_symbols(base: str) -> dict[str, set[str]]:
    removed: dict[str, set[str]] = {}
    current = None
    for line in git("diff", "-U0", base).splitlines():
        if line.startswith("+++ b/"):
            current = line[6:]
            continue
        if line.startswith("---") or not current:
            continue
        if line.startswith("-"):
            match = DECL_RE.match(line[1:])
            if match:
                removed.setdefault(current, set()).add(match.group(1))
    return removed


def report_changed(base: str, references: dict[str, list[tuple[str, str]]]) -> None:
    files = changed_files(base)
    if not files:
        print("No changed files.")
        return
    removed = removed_symbols(base)
    hits = 0
    print(f"Documentation impact of changes against {base}:")
    for path in files:
        docs = references.get(path)
        if not docs:
            continue
        hits += 1
        by_doc: dict[str, list[str]] = {}
        for doc, symbol in docs:
            by_doc.setdefault(doc, []).append(symbol)
        print(f"\n  {path}")
        for doc, symbols in sorted(by_doc.items()):
            print(f"    {doc}: " + ", ".join(f"`{symbol}`" for symbol in sorted(set(symbols))))
        gone = removed.get(path, set())
        cited = {symbol for _, symbol in docs}
        for symbol in sorted(gone & cited):
            print(f"    WARNING: the diff removes `{symbol}`, which this doc cites.")
    if hits == 0:
        print("  No documentation references the changed files.")
    else:
        print("\nRe-read the docs above before committing.")


def report_statuses() -> None:
    """Print the working-tree status of every tracked and untracked file.

    Agents use this to decide whether to commit and push, and to catch staged
    build products, secrets, or other generated artifacts before committing.
    """
    status = git("status", "--porcelain")
    entries = [line for line in status.splitlines() if line.strip()]
    if not entries:
        print("Working tree clean: nothing to commit.")
        return
    print("Working-tree status:")
    for line in entries:
        print(f"  {line}")
    staged = [line for line in entries if line[:2].strip() and line[0] not in " ?"]
    if staged:
        print("\nStaged changes present: commit them once all gates pass.")
    else:
        print("\nNothing is staged (changes may be untracked or unstaged).")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--changed", action="store_true")
    parser.add_argument("--base", default="HEAD")
    parser.add_argument(
        "--statuses",
        action="store_true",
        help="print git status --porcelain, for the commit-and-push decision",
    )
    args = parser.parse_args()

    if args.statuses:
        report_statuses()
        print()

    errors, references = validate()
    errors += validate_qna_registry()
    if errors:
        for error in errors:
            print(f"ERROR {error}")
        print(f"\nFAIL: {len(errors)} documentation problem(s)")
        return 1
    print(
        f"OK: {len(references)} code file(s) referenced by docs; "
        "all links, anchors, and cited symbols resolve"
    )
    if args.changed:
        print()
        report_changed(args.base, references)
    return 0


if __name__ == "__main__":
    sys.exit(main())
