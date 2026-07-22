import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def git_value(root, *arguments):
    try:
        return subprocess.check_output(
            ["git", "-C", str(root), *arguments],
            text=True,
            stderr=subprocess.DEVNULL,
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def rom_hashes(rom_path):
    rom = bytearray(rom_path.read_bytes())
    exact_hash = sha256(rom)

    # The checksum tool rewrites these four bytes after linking. Ignore them
    # so the content hash describes the ROM payload rather than its checksum.
    if len(rom) >= 0x10000:
        rom[0xffdc:0xffe0] = b"\0\0\0\0"

    return exact_hash, sha256(rom)


def load_history(history_path, flavor):
    records = []
    if not history_path.exists():
        return records, {}, 1

    for line in history_path.read_text(encoding="utf-8").splitlines():
        if not line:
            continue
        record = json.loads(line)
        if record["flavor"] == flavor:
            records.append(record)

    next_number = 1
    numbers_by_content = {}
    for record in records:
        content_hash = record["content_sha256"]
        if content_hash not in numbers_by_content:
            numbers_by_content[content_hash] = record.get("build_number", next_number)
            next_number = max(next_number, numbers_by_content[content_hash] + 1)

    return records, numbers_by_content, next_number


def main():
    parser = argparse.ArgumentParser(description="Record and compare a ROM build.")
    parser.add_argument("rom", type=Path)
    parser.add_argument("flavor")
    parser.add_argument("--history", type=Path, default=Path(".build-history/builds.jsonl"))
    parser.add_argument("--check", action="store_true", help="Compare the ROM and working tree with its manifest.")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    rom_path = root / args.rom
    history_path = root / args.history
    exact_hash, content_hash = rom_hashes(rom_path)
    manifest_path = root / (str(args.rom) + ".build.json")
    history_records, numbers_by_content, next_number = load_history(history_path, args.flavor)

    if args.check:
        if not manifest_path.exists():
            raise SystemExit(f"No build manifest found: {manifest_path}")

        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        current_status_hash = sha256(git_value(root, "status", "--porcelain=v1").encode("utf-8"))
        rom_changed = content_hash != manifest["content_sha256"]
        source_changed = current_status_hash != manifest["git_status_sha256"]
        print(f"{args.flavor}: ROM material content changed: {'yes' if rom_changed else 'no'}")
        print(f"{args.flavor}: build number: {manifest.get('build_number', 'unknown')}")
        print(f"{args.flavor}: source working tree changed: {'yes' if source_changed else 'no'}")
        if exact_hash != manifest["rom_sha256"]:
            print(f"{args.flavor}: exact ROM checksum changed: yes")
        return

    previous_match = next(
        (record for record in history_records if record["content_sha256"] == content_hash),
        None,
    )
    build_number = numbers_by_content.get(content_hash, next_number)

    record = {
        "built_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "flavor": args.flavor,
        "build_number": build_number,
        "rom": str(args.rom),
        "rom_sha256": exact_hash,
        "content_sha256": content_hash,
        "git_commit": git_value(root, "rev-parse", "HEAD"),
        "git_status_sha256": sha256(git_value(root, "status", "--porcelain=v1").encode("utf-8")),
        "matches_previous_build": previous_match is not None,
    }
    if previous_match is not None:
        record["previous_build_at_utc"] = previous_match["built_at_utc"]
        record["previous_git_commit"] = previous_match["git_commit"]

    history_path.parent.mkdir(parents=True, exist_ok=True)
    with history_path.open("a", encoding="utf-8", newline="\n") as history_file:
        history_file.write(json.dumps(record, separators=(",", ":")) + "\n")

    manifest_path.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")

    if previous_match is None:
        print(f"{args.flavor}: build {build_number}, new content {content_hash[:12]}")
    else:
        print(
            f"{args.flavor}: build {build_number}, content matches previous build "
            f"from {previous_match['built_at_utc']} ({content_hash[:12]})"
        )


if __name__ == "__main__":
    main()