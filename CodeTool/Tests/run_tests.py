#!/usr/bin/env python3
"""
CodeTool golden tests.

Each case is a directory under Cases/:
    Cases/<name>/input/     sources fed to the tool (copied to a temp dir first)
    Cases/<name>/expected/  the same files after the tool processed them, plus
                            the generated <project>ReflectionRegister.cpp
    Cases/<name>/options    optional, one CLI flag per line (e.g. -exclude_mode)
    Cases/<name>/known_bad  optional, notes about outputs that are currently
                            wrong; the case still runs, mismatches are reported
                            as failures only when the file content changes

Usage:
    run_tests.py                 run all cases
    run_tests.py <name> ...      run selected cases
    run_tests.py --update        rewrite expected/ from the current tool output
"""

import difflib
import os
import shutil
import subprocess
import sys
import tempfile

TESTS_DIR = os.path.dirname(os.path.abspath(__file__))
CASES_DIR = os.path.join(TESTS_DIR, "Cases")
DEFAULT_BINARY = os.path.join(TESTS_DIR, "..", "Bin", "o2CodeTool")

PROJECT_NAME = "TestProject"


def collect_files(root):
    result = {}
    for dirpath, _, filenames in os.walk(root):
        for name in filenames:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, root)
            with open(full, "r", encoding="utf-8", errors="replace") as f:
                result[rel] = f.read()
    return result


def run_case(binary, case_dir, update):
    name = os.path.basename(case_dir)
    input_dir = os.path.join(case_dir, "input")
    expected_dir = os.path.join(case_dir, "expected")

    options_path = os.path.join(case_dir, "options")
    options = []
    if os.path.exists(options_path):
        with open(options_path) as f:
            options = [line.strip() for line in f if line.strip()]

    with tempfile.TemporaryDirectory() as work_dir:
        sources_dir = os.path.join(work_dir, "Sources")
        shutil.copytree(input_dir, sources_dir)

        # The o2 framework cache provides the base types (o2::IObject, ISerializable):
        # without it the tool cannot tell reflectable classes from plain ones
        parents = os.path.join(TESTS_DIR, "..", "..", "Framework", "Sources", "o2", "CodeToolCache.xml")

        command = [binary, "-project", PROJECT_NAME, "-sources", sources_dir]
        if os.path.exists(parents):
            command += ["-parent_projects", parents]
        command += options
        process = subprocess.run(command, capture_output=True, text=True)

        if process.returncode != 0:
            return name, False, [f"tool exited with code {process.returncode}\n{process.stderr}"]

        # The cache file is timing dependent, it is not part of the golden data
        cache_path = os.path.join(sources_dir, "CodeToolCache.xml")
        if os.path.exists(cache_path):
            os.remove(cache_path)

        actual = collect_files(sources_dir)

        if update:
            if os.path.exists(expected_dir):
                shutil.rmtree(expected_dir)
            shutil.copytree(sources_dir, expected_dir)
            return name, True, ["updated"]

        if not os.path.exists(expected_dir):
            return name, False, ["no expected/ directory, run with --update first"]

        expected = collect_files(expected_dir)

        problems = []
        for rel in sorted(set(expected) | set(actual)):
            if rel not in actual:
                problems.append(f"missing output file: {rel}")
            elif rel not in expected:
                problems.append(f"unexpected output file: {rel}")
            elif expected[rel] != actual[rel]:
                diff = difflib.unified_diff(
                    expected[rel].splitlines(keepends=True),
                    actual[rel].splitlines(keepends=True),
                    fromfile=f"expected/{rel}", tofile=f"actual/{rel}")
                problems.append("".join(diff))

        return name, not problems, problems


def main():
    args = [a for a in sys.argv[1:]]
    update = "--update" in args
    if update:
        args.remove("--update")

    binary = os.environ.get("O2_CODETOOL", DEFAULT_BINARY)
    if not os.path.exists(binary):
        print(f"CodeTool binary not found: {binary}")
        return 2

    all_cases = sorted(
        os.path.join(CASES_DIR, name) for name in os.listdir(CASES_DIR)
        if os.path.isdir(os.path.join(CASES_DIR, name)))

    if args:
        all_cases = [c for c in all_cases if os.path.basename(c) in args]

    failed = []
    for case_dir in all_cases:
        name, ok, problems = run_case(binary, case_dir, update)
        status = "ok" if ok else "FAILED"
        print(f"[{status}] {name}")
        if not ok:
            failed.append(name)
            for problem in problems:
                print(problem)

    print()
    print(f"{len(all_cases) - len(failed)}/{len(all_cases)} cases passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
