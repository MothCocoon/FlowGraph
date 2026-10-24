"""
KB consistency checker for the Flow knowledge base.

Pure Python, no Unreal/MCP dependency - runnable standalone even when the
editor/MCP server is down.

A class's documentation lives on the class itself (`FFlowAgentDoc`, read via the catalog), never as
a markdown file - there are no per-class node/addon articles to check. The filename/tag check below
applies uniformly to every article kind.

Checks, across one or more `agent-docs/` roots:
  1. Every `$KB:<domain>:<kind>:<id>` reference resolves to a file that
     declares that exact tag as its own (a "definition" - the tag appearing
     on its own line near the top of the file, as described in INDEX.md).
  2. Every article's declared tag id matches its filename, normalized
     (lowercased, `-`/`_` stripped) so `CourierTextFormat.md` /
     `courier-text-format` compare equal. The exemption is any file literally named
     `INDEX.md` (any casing) - every index in this KB deliberately decouples
     its filename from its semantic id (e.g. `flowgraph-index`) so every domain
     root can use the same filename.
  3. (Informational, non-failing) Orphan articles: a defined tag that no
     OTHER article references. Doesn't affect exit code - it's a discoverability
     signal (e.g. a pattern with no see-also pointing at it), not an error.
     `guide` kind is excluded (index and authoring entry points are expected to be
     link *targets* mentioned in prose without always being formally referenced).

With --catalog <export.json> (a FlowCatalogQuery UTF-8 export, e.g. from
ExportFlowCatalogToFile), two more checks run - both need live per-class data
that markdown alone cannot provide:
  4. Bidirectional pattern/concept linkage. A class's AgentDoc.Articles
     entry "pattern:<slug>" or "concept:<slug>" must resolve to a real
     Patterns/*.md or Concepts/*.md file declaring the matching
     $KB:<domain>:<kind>:<slug> tag - flagged as a DANGLING SLUG if not.
     Conversely, every Pattern/Concept article must be referenced by at least
     one class's Articles field - flagged as an EMPTY-MEMBERSHIP ARTICLE if
     not (a pattern nobody's doc points at is undiscoverable from a class).
  5. Stem-collision ladder rule: a group of same-kind classes sharing one
     derived stem where NONE of them kept the bare stem (every member fell
     back to its full class name) - the catalog could not break the tie, and
     neither should this tool guess which one is right. This checks catalog
     naming independently of markdown articles.

Usage:
    python kb_lint.py <root1> [<root2> ...] [--catalog <export.json>]

Example (from the plugin root):
    python agent-docs/Scripts/kb_lint.py agent-docs

Exit code is 0 if clean, 1 if any problem was found (suitable for CI/hooks).
"""

from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

TAG_DEFINITION_RE = re.compile(r"^\$KB:([A-Za-z0-9_-]+):([A-Za-z0-9_-]+):([A-Za-z0-9_-]+)\s*$", re.MULTILINE)
TAG_REFERENCE_RE = re.compile(r"\$KB:([A-Za-z0-9_-]+):([A-Za-z0-9_-]+):([A-Za-z0-9_-]+)")


def normalize_for_filename_match(value: str) -> str:
    """Lowercases and strips '-'/'_' so PascalCase/kebab-case/SCREAMING variants of the same word
    compare equal (e.g. 'CourierTextFormat' and 'courier-text-format' both become
    'couriertextformat')."""
    return value.lower().replace("-", "").replace("_", "")


@dataclass
class KBFile:
    path: Path
    text: str
    definitions: list[tuple[str, str, str]] = field(default_factory=list)
    references: list[tuple[str, str, str]] = field(default_factory=list)


@dataclass
class LintResult:
    dangling_references: list[tuple[Path, str]] = field(default_factory=list)
    filename_tag_mismatch: list[tuple[Path, str]] = field(default_factory=list)
    duplicate_definitions: list[tuple[str, list[Path]]] = field(default_factory=list)
    orphan_tags: list[tuple[Path, str]] = field(default_factory=list)
    # Populated only when --catalog is given.
    dangling_slugs: list[tuple[str, str]] = field(default_factory=list)  # (class_path, slug)
    empty_membership_articles: list[tuple[Path, str]] = field(default_factory=list)  # (article, slug)
    unresolved_stem_collisions: list[tuple[str, str, list[str]]] = field(default_factory=list)  # (kind, stem, class_paths)

    def is_clean(self) -> bool:
        return not (
            self.dangling_references
            or self.filename_tag_mismatch
            or self.duplicate_definitions
            or self.dangling_slugs
            or self.empty_membership_articles
            or self.unresolved_stem_collisions
        )


def scan_root(root: Path) -> list[KBFile]:
    files = []
    for path in sorted(root.rglob("*.md")):
        if "process" in path.relative_to(root).parts:
            # Optional process notes are not KB articles and may contain illustrative tags.
            continue
        text = path.read_text(encoding="utf-8")
        kb_file = KBFile(path=path, text=text)
        kb_file.definitions = [
            (m.group(1), m.group(2), m.group(3)) for m in TAG_DEFINITION_RE.finditer(text)
        ]
        kb_file.references = [
            (m.group(1), m.group(2), m.group(3)) for m in TAG_REFERENCE_RE.finditer(text)
        ]
        files.append(kb_file)
    return files


def tag_str(domain: str, kind: str, id_: str) -> str:
    return f"${{KB}}:{domain}:{kind}:{id_}".replace("${KB}", "$KB")


def lint(files: list[KBFile]) -> LintResult:
    result = LintResult()

    defined_tags: dict[tuple[str, str, str], list[Path]] = {}
    for kb_file in files:
        for tag in kb_file.definitions:
            defined_tags.setdefault(tag, []).append(kb_file.path)

    for tag, paths in defined_tags.items():
        if len(paths) > 1:
            result.duplicate_definitions.append((tag_str(*tag), paths))

    for kb_file in files:
        # Every reference (including a file's own definition line, since a
        # definition is also a valid reference to itself) must resolve.
        for ref in set(kb_file.references):
            if ref not in defined_tags:
                result.dangling_references.append((kb_file.path, tag_str(*ref)))

        for domain, kind, id_ in kb_file.definitions:
            # Every index in this KB is named INDEX.md by convention and carries a semantic id
            # (flowgraph-index, ...) instead - filename/id decoupling is the point,
            # not drift, so index files are exempt from this check entirely.
            if kb_file.path.stem.lower() == "index":
                continue
            if normalize_for_filename_match(kb_file.path.stem) != normalize_for_filename_match(id_):
                result.filename_tag_mismatch.append((kb_file.path, id_))

    # Informational: orphan tags (defined, but no OTHER file references them).
    referencing_files: dict[tuple[str, str, str], set[Path]] = {}
    for kb_file in files:
        for ref in set(kb_file.references):
            referencing_files.setdefault(ref, set()).add(kb_file.path)

    for tag, paths in defined_tags.items():
        if len(paths) != 1 or tag[1] == "guide":
            continue
        definer = paths[0]
        referencers = referencing_files.get(tag, set()) - {definer}
        if not referencers:
            result.orphan_tags.append((definer, tag_str(*tag)))

    return result


# --- Catalog-driven checks (require --catalog) ---------------------------------------------

SLUG_RE = re.compile(r"^(pattern|concept):([a-z0-9]+(?:-[a-z0-9]+)*)$")


def load_catalog_rows(catalog_path: Path) -> list[dict]:
    """Reads a FlowCatalogQuery export and returns every node and addon row, tagged with its kind."""
    with catalog_path.open(encoding="utf-8") as handle:
        payload = json.load(handle)

    rows: list[dict] = []
    for row in payload.get("nodes", []):
        row = dict(row)
        row["_kind"] = "node"
        rows.append(row)
    for row in payload.get("addons", []):
        row = dict(row)
        row["_kind"] = "addon"
        rows.append(row)
    return rows


def find_pattern_concept_definitions(files: list[KBFile]) -> dict[tuple[str, str], Path]:
    """Maps (kind, slug) -> defining file, for pattern/concept tags only.

    Bridges the two slug namespaces mechanically: an article
    declares $KB:<domain>:<kind>:<slug>, while a class's AgentDoc.Articles entry stores
    "<kind>:<slug>" with no domain. Resolution is always by the exact tag, never by filename.
    """
    definitions: dict[tuple[str, str], Path] = {}
    for kb_file in files:
        for _domain, kind, id_ in kb_file.definitions:
            if kind in ("pattern", "concept"):
                definitions[(kind, id_)] = kb_file.path
    return definitions


def lint_article_linkage(rows: list[dict], article_definitions: dict[tuple[str, str], Path]) -> tuple[list[tuple[str, str]], list[tuple[Path, str]]]:
    """Bidirectional linkage check: every class-side Articles slug resolves to a defined article, and
    every defined pattern/concept article has at least one class pointing back at it."""
    dangling: list[tuple[str, str]] = []
    referenced: set[tuple[str, str]] = set()

    for row in rows:
        class_path = row.get("class_path", "<unknown>")
        for slug in row.get("articles") or []:
            match = SLUG_RE.match(slug)
            if not match:
                # Malformed slugs are out of scope here; this check only cares about linkage.
                continue
            kind, id_ = match.group(1), match.group(2)
            if (kind, id_) not in article_definitions:
                dangling.append((class_path, slug))
            else:
                referenced.add((kind, id_))

    empty_membership = [
        (path, f"{kind}:{id_}")
        for (kind, id_), path in article_definitions.items()
        if (kind, id_) not in referenced
    ]
    return dangling, empty_membership


def derive_raw_stem(class_name: str) -> str:
    """
    Reimplements UFlowCatalogQuery::MakeClassStem, independent of collision resolution.

    Strips a trailing generated-class suffix (_C) and then one leading family-prefix segment
    (the first '_'-delimited token), collapsing a doubled prefix (FlowNode_FlowNode_Thing -> Thing).
    Needed to detect an unresolved collision: the export only shows the resolved `name`, not what the
    stem would have been before the tie-break ran.
    """
    name = class_name
    if name.endswith("_C"):
        name = name[: -len("_C")]

    parts = name.split("_", 1)
    if len(parts) == 2 and parts[0]:
        name = parts[1]
        # Collapse a doubled prefix (FlowNode_FlowNode_Thing -> after one strip: FlowNode_Thing).
        parts2 = name.split("_", 1)
        if len(parts2) == 2 and parts2[0] == parts[0]:
            name = parts2[1]

    return name


def lint_stem_collisions(rows: list[dict]) -> list[tuple[str, str, list[str]]]:
    """
    Detect a group of same-kind classes sharing a derived stem where the catalog's ladder could not
    pick a winner. A correctly-resolved collision always has exactly one member whose reported `name`
    is the short stem; an unresolved tie has every member reporting its full class name instead,
    because the catalog could not break the tie either.
    """
    groups: dict[tuple[str, str], list[dict]] = {}
    for row in rows:
        class_path = row.get("class_path", "")
        class_name = class_path.rsplit(".", 1)[-1]
        raw_stem = derive_raw_stem(class_name)
        groups.setdefault((row["_kind"], raw_stem), []).append(row)

    unresolved: list[tuple[str, str, list[str]]] = []
    for (kind, stem), members in groups.items():
        if len(members) < 2:
            continue
        if any(m.get("name") == stem for m in members):
            continue  # Correctly resolved: one member kept the bare stem.
        unresolved.append((kind, stem, [m.get("class_path", "") for m in members]))

    return unresolved


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 1

    catalog_arg: str | None = None
    root_args: list[str] = []
    args = iter(argv)
    for arg in args:
        if arg == "--catalog":
            catalog_arg = next(args, None)
            if catalog_arg is None:
                print("ERROR: --catalog requires a path argument")
                return 1
        else:
            root_args.append(arg)

    if not root_args:
        print(__doc__)
        return 1

    all_files: list[KBFile] = []
    for root_arg in root_args:
        root = Path(root_arg)
        if not root.exists():
            print(f"ERROR: root does not exist: {root}")
            return 1
        all_files.extend(scan_root(root))

    result = lint(all_files)

    if catalog_arg:
        catalog_path = Path(catalog_arg)
        if not catalog_path.is_file():
            print(f"ERROR: --catalog path does not exist: {catalog_path}")
            return 1
        rows = load_catalog_rows(catalog_path)
        article_definitions = find_pattern_concept_definitions(all_files)
        result.dangling_slugs, result.empty_membership_articles = lint_article_linkage(rows, article_definitions)
        result.unresolved_stem_collisions = lint_stem_collisions(rows)

    if result.is_clean():
        print(f"OK - {len(all_files)} article(s) scanned, no issues found.")
        if result.orphan_tags:
            print(f"\nOrphan articles ({len(result.orphan_tags)}, informational - not a failure):")
            for path, tag in result.orphan_tags:
                print(f"  {path}: defines {tag}, but no other article references it")
        return 0

    if result.dangling_references:
        print(f"\nDangling references ({len(result.dangling_references)}):")
        for path, tag in result.dangling_references:
            print(f"  {path}: references {tag}, but no article defines it")

    if result.filename_tag_mismatch:
        print(f"\nFilename/tag id mismatch ({len(result.filename_tag_mismatch)}):")
        for path, id_ in result.filename_tag_mismatch:
            print(f"  {path}: declares id '{id_}' but filename stem is '{path.stem}'")

    if result.duplicate_definitions:
        print(f"\nDuplicate tag definitions ({len(result.duplicate_definitions)}):")
        for tag, paths in result.duplicate_definitions:
            print(f"  {tag}: defined in {len(paths)} files: {[str(p) for p in paths]}")

    if result.dangling_slugs:
        print(f"\nDangling article slugs ({len(result.dangling_slugs)}):")
        for class_path, slug in result.dangling_slugs:
            print(f"  {class_path}: Articles carries '{slug}', but no Patterns/Concepts file defines it")

    if result.empty_membership_articles:
        print(f"\nEmpty-membership articles ({len(result.empty_membership_articles)}):")
        for path, slug in result.empty_membership_articles:
            print(f"  {path}: defines '{slug}', but no class's Articles field references it")

    if result.unresolved_stem_collisions:
        print(f"\nUnresolved stem collisions - ladder rule 4 ({len(result.unresolved_stem_collisions)}):")
        for kind, stem, class_paths in result.unresolved_stem_collisions:
            print(f"  {kind} stem '{stem}': {class_paths} - consolidate one away, do not rename")

    print()
    return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
