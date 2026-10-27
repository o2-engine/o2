"""Run the reliable image-to-ui workflow.

Use ``prepare`` before writing ui_structure.json and optional ``measure`` for
pixel-preserving local crops, then ``check`` for every
review iteration. ``target`` creates focused bbox images when the overview is
too crowded. Both validation paths refresh comparison.png and its same-pass
diagnostics. ``finalize`` verifies review coverage and accepted approximations.
The state file prevents outputs from being reused with different inputs and
makes interrupted runs safe to retry.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.metadata
import importlib.util
import json
import platform
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


SCRIPT_DIR = Path(__file__).resolve().parent
STATE_NAME = "workflow_state.json"
STATE_VERSION = 2
RENDER_DEPENDENCY_VERSION = 2
RELEVANT_ASSET_SUFFIXES = {".png", ".jpg", ".jpeg", ".ttf", ".otf", ".ttc"}
RELEVANT_META_SUFFIXES = (".png.meta", ".jpg.meta", ".jpeg.meta")
CRITICAL_SCRIPT_NAMES = (
    "workflow.py",
    "inventory_assets.py",
    "annotate_grid.py",
    "validate_structure.py",
    "backfill_anchors.py",
    "layout.py",
    "ui_components.py",
    "preview_components.py",
    "render_comparison.py",
    "annotate_element.py",
    "audit_render.py",
    "review_risk.py",
)
REVIEW_BINDING_ARTIFACTS = (
    "validation",
    "bbox",
    "bbox_legend",
    "comparison",
    "reconstruction",
    "render_trace",
    "assets_inventory",
    "review_risk",
    "risk_review",
    "risk_review_legend",
)
REVIEW_BINDING_PATTERN = re.compile(
    r"<!--\s*image-to-ui-review-binding:\s*"
    r"structure=([0-9a-f]{64})\s+artifacts=([0-9a-f]{64})\s*-->",
    flags=re.IGNORECASE,
)
REVIEW_BINDING_MARKER = re.compile(
    r"<!--\s*image-to-ui-review-binding:", flags=re.IGNORECASE
)


class WorkflowError(RuntimeError):
    pass


def resolved(path: str | Path) -> Path:
    return Path(path).expanduser().resolve()


def is_within(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
        return True
    except ValueError:
        return False


def require_file(path: Path, label: str) -> None:
    if not path.is_file():
        raise WorkflowError(f"{label} not found: {path}")


def require_dir(path: Path, label: str) -> None:
    if not path.is_dir():
        raise WorkflowError(f"{label} not found: {path}")


def check_dependencies() -> None:
    missing = [
        package
        for package, module in (("pillow", "PIL"), ("numpy", "numpy"))
        if importlib.util.find_spec(module) is None
    ]
    if missing:
        command = f'"{sys.executable}" -m pip install ' + " ".join(missing)
        raise WorkflowError(
            f"Missing Python dependencies: {', '.join(missing)}. Run: {command}"
        )


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def design_snapshot(path: Path) -> dict[str, Any]:
    stat = path.stat()
    return {
        "path": str(path),
        "size": stat.st_size,
        "mtime_ns": stat.st_mtime_ns,
        "sha256": sha256_file(path),
    }


def artifact_snapshot(path: Path) -> dict[str, Any]:
    stat = path.stat()
    return {
        "path": str(path),
        "size": stat.st_size,
        "mtime_ns": stat.st_mtime_ns,
        "sha256": sha256_file(path),
    }


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()


def read_json_file(path: Path, label: str) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise WorkflowError(f"Invalid {label}: {path}: {exc}") from exc
    if not isinstance(value, dict):
        raise WorkflowError(f"{label} root must be an object: {path}")
    return value


def relevant_asset_files(root: Path) -> list[Path]:
    return sorted(
        (
            path
            for path in root.rglob("*")
            if path.is_file()
            and (
                path.suffix.lower() in RELEVANT_ASSET_SUFFIXES
                or path.name.lower().endswith(RELEVANT_META_SUFFIXES)
            )
        ),
        key=lambda path: path.relative_to(root).as_posix().lower(),
    )


def assets_snapshot(root: Path) -> dict[str, Any]:
    digest = hashlib.sha256()
    files = relevant_asset_files(root)
    for path in files:
        relative = path.relative_to(root).as_posix()
        digest.update(relative.encode("utf-8"))
        digest.update(b"\0")
        digest.update(sha256_file(path).encode("ascii"))
        digest.update(b"\n")
    return {
        "path": str(root),
        "file_count": len(files),
        "signature": digest.hexdigest(),
    }


def input_snapshot(design: Path, assets: Path) -> dict[str, Any]:
    return {
        "design": design_snapshot(design),
        "assets": assets_snapshot(assets),
    }


def same_inputs(expected: dict[str, Any], current: dict[str, Any]) -> bool:
    return (
        expected.get("design", {}).get("path") == current["design"]["path"]
        and expected.get("design", {}).get("sha256")
        == current["design"]["sha256"]
        and expected.get("assets", {}).get("path") == current["assets"]["path"]
        and expected.get("assets", {}).get("signature")
        == current["assets"]["signature"]
    )


def write_json_atomic(path: Path, value: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.tmp")
    temporary.write_text(
        json.dumps(value, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    temporary.replace(path)


def read_state(output: Path) -> dict[str, Any]:
    state_path = output / STATE_NAME
    if not state_path.is_file():
        raise WorkflowError(
            f"Workflow state not found: {state_path}. Run prepare first."
        )
    try:
        state = json.loads(state_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise WorkflowError(f"Invalid workflow state: {state_path}: {exc}") from exc
    if state.get("version") != STATE_VERSION:
        raise WorkflowError(
            f"Unsupported workflow state version in {state_path}; rerun prepare."
        )
    return state


def run_script(name: str, *arguments: str) -> None:
    script = SCRIPT_DIR / name
    command = [sys.executable, "-B", str(script), *arguments]
    print(f"\n[{script.stem}]", flush=True)
    result = subprocess.run(command, check=False)
    if result.returncode:
        raise WorkflowError(
            f"{script.name} failed with exit code {result.returncode}"
        )


def unlink_if_exists(path: Path) -> None:
    if path.is_file():
        path.unlink()


def clean_prepare_artifacts(output: Path) -> None:
    unlink_if_exists(output / "design_grid.png")
    unlink_if_exists(output / "design_grid_metrics.json")
    for directory in (output / "measurements").glob("region_*"):
        if directory.is_dir():
            for name in ("design.png", "design_grid.png", "metrics.json"):
                unlink_if_exists(directory / name)
    assets_output = output / "assets"
    if assets_output.is_dir():
        unlink_if_exists(assets_output / "assets_inventory.json")
        for path in assets_output.glob("assets_contact_sheet*.png"):
            unlink_if_exists(path)


def clean_check_artifacts(output: Path) -> None:
    for directory in (output / "previews").glob("*"):
        if directory.is_dir():
            for name in ("reconstruction.png", "render_trace.json", "preview_report.json"):
                unlink_if_exists(directory / name)
    for name in (
        "validate_report.json",
        "all_elements.png",
        "all_elements_legend.json",
        "comparison.png",
        "reconstruction.png",
        "render_trace.json",
        "visual_audit.json",
        "review_risk.json",
        "risk_review.png",
        "risk_review_legend.json",
        "detail_comparison.png",
        "completion_report.json",
    ):
        unlink_if_exists(output / name)


def clean_target_artifacts(output: Path) -> None:
    for path in output.glob("target_*.png"):
        unlink_if_exists(path)
    for path in output.glob("target_*_legend.json"):
        unlink_if_exists(path)


def clean_finalize_artifacts(output: Path) -> None:
    unlink_if_exists(output / "detail_comparison.png")
    unlink_if_exists(output / "completion_report.json")


def prepare(args: argparse.Namespace) -> None:
    check_dependencies()
    design = resolved(args.design)
    assets = resolved(args.assets)
    output = resolved(args.output)
    require_file(design, "Design image")
    require_dir(assets, "Assets directory")
    if is_within(output, assets) or is_within(assets, output):
        raise WorkflowError(
            "Output and assets directories must not contain one another."
        )

    files = relevant_asset_files(assets)
    if not any(path.suffix.lower() == ".png" for path in files):
        raise WorkflowError(f"No PNG assets found under: {assets}")

    state_path = output / STATE_NAME
    evidence_generation = 0
    if output.exists() and not state_path.exists() and any(output.iterdir()):
        raise WorkflowError(
            f"Output directory is not empty and has no workflow state: {output}. "
            "Use a new task directory."
        )

    output.mkdir(parents=True, exist_ok=True)
    snapshot = input_snapshot(design, assets)
    if state_path.exists():
        previous = read_json_file(state_path, "workflow state")
        previous_design = previous.get("inputs", {}).get("design", {}).get("path")
        previous_assets = previous.get("inputs", {}).get("assets", {}).get("path")
        if previous_design != str(design) or previous_assets != str(assets):
            raise WorkflowError(
                f"Output belongs to different inputs: {output}. Use a new task directory."
            )
        previous_generation = previous.get("evidence_generation", 0)
        if (
            not isinstance(previous_generation, int)
            or isinstance(previous_generation, bool)
            or previous_generation < 0
        ):
            raise WorkflowError(
                "Workflow evidence generation is invalid; use a new task directory."
            )
        evidence_generation = previous_generation

    state: dict[str, Any] = {
        "version": STATE_VERSION,
        "status": "preparing",
        "inputs": snapshot,
        "output": str(output),
        "evidence_generation": evidence_generation,
    }
    write_json_atomic(state_path, state)
    clean_prepare_artifacts(output)
    clean_check_artifacts(output)
    clean_target_artifacts(output)

    try:
        run_script(
            "inventory_assets.py",
            "--assets",
            str(assets),
            "--output",
            str(output / "assets"),
        )
        run_script(
            "annotate_grid.py",
            "--design",
            str(design),
            "--output",
            str(output / "design_grid.png"),
            "--metrics",
            str(output / "design_grid_metrics.json"),
        )
    except WorkflowError as exc:
        state["status"] = "prepare_failed"
        state["error"] = str(exc)
        write_json_atomic(state_path, state)
        raise

    state["status"] = "prepared"
    state["artifacts"] = {
        "inventory": str(output / "assets" / "assets_inventory.json"),
        "grid": str(output / "design_grid.png"),
        "grid_metrics": str(output / "design_grid_metrics.json"),
    }
    write_json_atomic(state_path, state)
    print(f"\nPrepared task: {output}")
    print(f"Write structure: {output / 'ui_structure.json'}")


def measure(args: argparse.Namespace) -> None:
    """Measure prepared design pixels without requiring or changing a draft."""
    check_dependencies()
    output = resolved(args.output)
    state = read_state(output)
    if state.get("status") in {"preparing", "prepare_failed"}:
        raise WorkflowError("Prepare must succeed before measuring the design.")
    design_value = state.get("inputs", {}).get("design", {}).get("path")
    assets_value = state.get("inputs", {}).get("assets", {}).get("path")
    if not isinstance(design_value, str) or not isinstance(assets_value, str):
        raise WorkflowError("Prepared input paths are missing. Rerun prepare.")
    design, assets = resolved(design_value), resolved(assets_value)
    require_file(design, "Design image")
    require_dir(assets, "Assets directory")
    ensure_inputs_current(state, design, assets)
    region_name = "region_" + "_".join(str(value) for value in args.region)
    destination = output / "measurements" / region_name
    run_script(
        "measure_region.py", "--design", str(design), "--output", str(destination),
        "--region", *(str(value) for value in args.region),
        "--zoom", str(args.zoom), "--cell-size", str(args.cell_size),
    )
    ensure_inputs_current(state, design, assets)
    metrics_path = destination / "metrics.json"
    metrics = read_json_file(metrics_path, "measurement metrics")
    metrics["design_sha256"] = state["inputs"]["design"]["sha256"]
    write_json_atomic(metrics_path, metrics)
    print("Rulers use absolute design pixels. Read metrics.json for crop/zoom offsets.")
    print("Measurement does not change workflow status or replace check/target evidence.")


def checked_context(
    output_arg: str, structure_arg: str | None
) -> tuple[Path, Path, Path, Path, dict[str, Any]]:
    check_dependencies()
    output = resolved(output_arg)
    state = read_state(output)
    design_value = state.get("inputs", {}).get("design", {}).get("path")
    assets_value = state.get("inputs", {}).get("assets", {}).get("path")
    if not isinstance(design_value, str) or not isinstance(assets_value, str):
        state["status"] = "invalid_state"
        state["error"] = "Prepared input paths are missing from workflow state."
        write_json_atomic(output / STATE_NAME, state)
        raise WorkflowError(state["error"])
    design = resolved(design_value)
    assets = resolved(assets_value)
    if not design.is_file() or not assets.is_dir():
        state["status"] = "inputs_missing"
        state["error"] = "Prepared design or assets are no longer available."
        write_json_atomic(output / STATE_NAME, state)
        raise WorkflowError(state["error"])
    current = input_snapshot(design, assets)
    if not same_inputs(state.get("inputs", {}), current):
        state["status"] = "inputs_changed"
        state["error"] = (
            "Design or assets changed after prepare. Rerun prepare before continuing."
        )
        write_json_atomic(output / STATE_NAME, state)
        raise WorkflowError(state["error"])
    structure = resolved(structure_arg) if structure_arg else output / "ui_structure.json"
    if not structure.is_file():
        state["status"] = "structure_missing"
        state["error"] = f"Structure JSON not found: {structure}"
        write_json_atomic(output / STATE_NAME, state)
        raise WorkflowError(state["error"])
    checked_structure = state.get("structure", {})
    if checked_structure and checked_structure.get("sha256") != sha256_file(structure):
        state["status"] = "structure_changed"
        state["error"] = "Structure changed after the last full check."
        write_json_atomic(output / STATE_NAME, state)
    return output, design, assets, structure, state


def ensure_inputs_current(
    state: dict[str, Any], design: Path, assets: Path
) -> None:
    current = input_snapshot(design, assets)
    if not same_inputs(state.get("inputs", {}), current):
        raise WorkflowError(
            "Design or assets changed during the workflow command. Rerun prepare."
        )


def runtime_dependency_versions() -> dict[str, str]:
    try:
        pillow_version = importlib.metadata.version("Pillow")
        numpy_version = importlib.metadata.version("NumPy")
    except importlib.metadata.PackageNotFoundError as exc:
        raise WorkflowError(
            f"Render dependency version is unavailable: {exc.name}"
        ) from exc
    return {
        "Python": f"{platform.python_implementation()} {platform.python_version()}",
        "Pillow": pillow_version,
        "NumPy": numpy_version,
    }


def font_dependency_path(font: Any) -> Path | None:
    value = getattr(font, "path", None)
    if not isinstance(value, (str, Path)):
        return None
    path = resolved(value)
    return path if path.is_file() else None


def workflow_font_dependency_paths(output: Path) -> list[Path]:
    legend = read_json_file(
        output / "all_elements_legend.json", "all-elements legend"
    )
    design_size = legend.get("design_size")
    if (
        not isinstance(design_size, list)
        or len(design_size) != 2
        or any(
            not isinstance(value, int) or isinstance(value, bool) or value <= 0
            for value in design_size
        )
    ):
        raise WorkflowError(
            "All-elements legend design_size is missing or invalid; run check again."
        )
    shortest_side = min(design_size)
    providers = (
        (
            "annotate_element.py",
            {max(12, shortest_side // 90), max(18, shortest_side // 60)},
        ),
        ("audit_render.py", {12, 15, 18}),
        ("review_risk.py", {12, 15}),
    )
    paths: set[Path] = set()
    for script_name, sizes in providers:
        script = SCRIPT_DIR / script_name
        spec = importlib.util.spec_from_file_location(
            f"_image_to_ui_font_{script.stem}", script
        )
        if spec is None or spec.loader is None:
            raise WorkflowError(
                f"Cannot inspect font dependency provider: {script}"
            )
        module = importlib.util.module_from_spec(spec)
        try:
            spec.loader.exec_module(module)
            find_font = getattr(module, "find_font")
            fonts = (find_font(size) for size in sorted(sizes))
            for font in fonts:
                path = font_dependency_path(font)
                if path is not None:
                    paths.add(path)
        except Exception as exc:
            raise WorkflowError(
                f"Cannot inspect font dependency provider {script.name}: {exc}"
            ) from exc
    return sorted(paths, key=lambda path: str(path).casefold())


def render_dependency_snapshots(output: Path) -> dict[str, Any]:
    trace = read_json_file(output / "render_trace.json", "render trace")
    files: dict[str, dict[str, Any]] = {}
    for name in CRITICAL_SCRIPT_NAMES:
        script = SCRIPT_DIR / name
        require_file(script, f"Critical workflow script {name}")
        files[str(script)] = artifact_snapshot(script)
    for item in trace.get("elements", []):
        if not isinstance(item, dict):
            continue
        text_info = item.get("text")
        if not isinstance(text_info, dict):
            continue
        font_info = text_info.get("font")
        if not isinstance(font_info, dict):
            continue
        resolved_font = font_info.get("resolved")
        if not isinstance(resolved_font, str) or not resolved_font:
            continue
        font_path = resolved(resolved_font)
        require_file(font_path, "Resolved render font")
        files[str(font_path)] = artifact_snapshot(font_path)
    for font_path in workflow_font_dependency_paths(output):
        files[str(font_path)] = artifact_snapshot(font_path)
    return {
        "version": RENDER_DEPENDENCY_VERSION,
        "files": files,
        "packages": runtime_dependency_versions(),
    }


def verify_render_dependencies(output: Path, state: dict[str, Any]) -> None:
    expected = state.get("check", {}).get("render_dependencies")
    if (
        not isinstance(expected, dict)
        or expected.get("version") != RENDER_DEPENDENCY_VERSION
        or not isinstance(expected.get("files"), dict)
        or not isinstance(expected.get("packages"), dict)
    ):
        raise WorkflowError(
            "Render dependency snapshot is obsolete or incomplete; run check again."
        )
    current = render_dependency_snapshots(output)
    expected_packages = expected["packages"]
    current_packages = current["packages"]
    changed_versions = sorted(
        name
        for name in set(expected_packages) | set(current_packages)
        if expected_packages.get(name) != current_packages.get(name)
    )
    if changed_versions:
        details = ", ".join(
            f"{name} (checked {expected_packages.get(name)!r}, "
            f"current {current_packages.get(name)!r})"
            for name in changed_versions
        )
        raise WorkflowError(
            "Render dependency versions changed after check: "
            f"{details}. Run check again."
        )

    expected_files = expected["files"]
    current_files = current["files"]
    changed_files = sorted(
        path_value
        for path_value in set(expected_files) | set(current_files)
        if not isinstance(expected_files.get(path_value), dict)
        or not isinstance(current_files.get(path_value), dict)
        or expected_files[path_value].get("sha256")
        != current_files[path_value].get("sha256")
    )
    if changed_files:
        display_names = [
            Path(path_value).name
            if is_within(resolved(path_value), SCRIPT_DIR)
            else path_value
            for path_value in changed_files
        ]
        raise WorkflowError(
            "Render dependency files changed after check: "
            + ", ".join(display_names)
            + ". Run check again."
        )


def validate(
    output: Path,
    design: Path,
    assets: Path,
    structure: Path,
    allow_warnings: bool,
) -> None:
    arguments = [
        "--structure",
        str(structure),
        "--design",
        str(design),
        "--assets",
        str(assets),
        "--report",
        str(output / "validate_report.json"),
    ]
    if not allow_warnings:
        arguments.append("--warnings-as-errors")
    run_script("validate_structure.py", *arguments)


def validate_and_render(
    output: Path,
    design: Path,
    assets: Path,
    structure: Path,
    allow_warnings: bool,
    transparent_bg: bool,
) -> None:
    comparison = output / "comparison.png"
    reconstruction = output / "reconstruction.png"
    trace = output / "render_trace.json"
    for path in (comparison, reconstruction, trace, output / "visual_audit.json"):
        unlink_if_exists(path)
    validate(output, design, assets, structure, allow_warnings)
    arguments = [
        "--design",
        str(design),
        "--structure",
        str(structure),
        "--assets",
        str(assets),
        "--output",
        str(comparison),
        "--reconstruction",
        str(reconstruction),
        "--trace",
        str(trace),
    ]
    if transparent_bg:
        arguments.append("--transparent-bg")
    run_script("render_comparison.py", *arguments)


def visual_audit(
    output: Path,
    design: Path,
    *,
    evidence: bool = False,
    fail_on_error: bool = False,
) -> None:
    arguments = [
        "--trace",
        str(output / "render_trace.json"),
        "--inventory",
        str(output / "assets" / "assets_inventory.json"),
        "--design",
        str(design),
        "--reconstruction",
        str(output / "reconstruction.png"),
        "--output",
        str(output / "visual_audit.json"),
    ]
    if evidence:
        arguments.extend(["--evidence", str(output / "detail_comparison.png")])
    if fail_on_error:
        arguments.append("--fail-on-error")
    run_script("audit_render.py", *arguments)


def review_risk(output: Path, design: Path) -> None:
    run_script(
        "review_risk.py",
        "--trace",
        str(output / "render_trace.json"),
        "--design",
        str(design),
        "--reconstruction",
        str(output / "reconstruction.png"),
        "--output",
        str(output / "review_risk.json"),
        "--evidence",
        str(output / "risk_review.png"),
        "--legend",
        str(output / "risk_review_legend.json"),
    )


def check_artifact_snapshots(output: Path) -> dict[str, dict[str, Any]]:
    paths = {
        "validation": output / "validate_report.json",
        "bbox": output / "all_elements.png",
        "bbox_legend": output / "all_elements_legend.json",
        "comparison": output / "comparison.png",
        "reconstruction": output / "reconstruction.png",
        "render_trace": output / "render_trace.json",
        "visual_audit": output / "visual_audit.json",
        "assets_inventory": output / "assets" / "assets_inventory.json",
        "review_risk": output / "review_risk.json",
        "risk_review": output / "risk_review.png",
        "risk_review_legend": output / "risk_review_legend.json",
    }
    for name, path in paths.items():
        require_file(path, f"Check artifact {name}")
    return {name: artifact_snapshot(path) for name, path in paths.items()}


def check(args: argparse.Namespace) -> None:
    output, design, assets, structure, state = checked_context(
        args.output, args.structure
    )
    previous_structure_hash = state.get("structure", {}).get("sha256")
    previous_revision = int(state.get("structure_revision", 0) or 0)
    clean_check_artifacts(output)
    clean_target_artifacts(output)
    state["status"] = "checking"
    state.pop("error", None)
    state.pop("completion", None)
    state.pop("completed_at", None)
    state.pop("finalized_at", None)
    state.pop("review_binding", None)
    state.pop("targets", None)
    write_json_atomic(output / STATE_NAME, state)

    try:
        validate_and_render(
            output,
            design,
            assets,
            structure,
            args.allow_warnings,
            args.transparent_bg,
        )
        run_script(
            "annotate_element.py",
            "--design",
            str(design),
            "--structure",
            str(structure),
            "--all-elements",
            "--output",
            str(output / "all_elements.png"),
            "--legend",
            str(output / "all_elements_legend.json"),
        )
        visual_audit(output, design, fail_on_error=True)
        review_risk(output, design)
        ensure_inputs_current(state, design, assets)
    except WorkflowError as exc:
        state["status"] = "check_failed"
        state["error"] = str(exc)
        write_json_atomic(output / STATE_NAME, state)
        raise

    structure_info = design_snapshot(structure)
    if previous_revision <= 0:
        structure_revision = 1
    elif previous_structure_hash != structure_info["sha256"]:
        structure_revision = previous_revision + 1
    else:
        structure_revision = previous_revision

    state["status"] = "checked"
    state.pop("error", None)
    state["structure"] = structure_info
    state["structure_revision"] = structure_revision
    state["check_count"] = int(state.get("check_count", 0) or 0) + 1
    state["evidence_generation"] = int(
        state.get("evidence_generation", 0) or 0
    ) + 1
    state["checked_at"] = utc_now()
    state["check"] = {
        "allow_warnings": bool(args.allow_warnings),
        "transparent_bg": bool(args.transparent_bg),
        "artifacts": {
            "validation": str(output / "validate_report.json"),
            "bbox": str(output / "all_elements.png"),
            "bbox_legend": str(output / "all_elements_legend.json"),
            "comparison": str(output / "comparison.png"),
            "reconstruction": str(output / "reconstruction.png"),
            "render_trace": str(output / "render_trace.json"),
            "visual_audit": str(output / "visual_audit.json"),
            "assets_inventory": str(
                output / "assets" / "assets_inventory.json"
            ),
            "review_risk": str(output / "review_risk.json"),
            "risk_review": str(output / "risk_review.png"),
            "risk_review_legend": str(output / "risk_review_legend.json"),
        },
        "artifact_snapshots": check_artifact_snapshots(output),
        "render_dependencies": render_dependency_snapshots(output),
    }
    write_json_atomic(output / STATE_NAME, state)
    print(f"\nCheck passed: {output}")
    print(f"Review binding: {review_binding_comment(state)}")


def safe_slug(value: str) -> str:
    slug = re.sub(r"[^A-Za-z0-9._-]+", "_", value).strip("._-")
    return (slug or "element")[-120:]


def preview(args: argparse.Namespace) -> None:
    output, design, assets, structure, state = checked_context(args.output, args.structure)
    if state.get("status") not in {"checked", "completed"}:
        raise WorkflowError("Run a successful full check before component previews.")
    verify_check_artifacts(output, state)
    verify_render_dependencies(output, state)
    arguments = [
        "--structure", str(structure), "--design", str(design), "--assets", str(assets),
        "--inventory", str(output / "assets" / "assets_inventory.json"),
        "--output", str(output / "previews" / safe_slug(args.name)),
    ]
    if args.size:
        arguments.extend(["--size", *(str(value) for value in args.size)])
    for option, values in (("--state", args.state), ("--progress", args.progress), ("--scroll", args.scroll)):
        for value in values or []:
            arguments.extend([option, value])
    if state.get("check", {}).get("transparent_bg"):
        arguments.append("--transparent-bg")
    run_script("preview_components.py", *arguments)
    ensure_inputs_current(state, design, assets)
    print("Native comparison and review binding are unchanged; inspect this scenario separately.")


def target(args: argparse.Namespace) -> None:
    output, design, assets, structure, state = checked_context(
        args.output, args.structure
    )
    if state.get("status") not in {"checked", "completed"}:
        raise WorkflowError("Run a successful full check before target review.")
    if state.get("structure", {}).get("sha256") != sha256_file(structure):
        raise WorkflowError("Structure changed after the full check; run check again.")
    verified_artifacts = verify_check_artifacts(output, state)
    verify_render_dependencies(output, state)
    transparent_bg = bool(
        args.transparent_bg
        or state.get("check", {}).get("transparent_bg", False)
    )
    clean_finalize_artifacts(output)
    state["status"] = "targeting"
    state.pop("error", None)
    state.pop("completion", None)
    state.pop("completed_at", None)
    state.pop("finalized_at", None)
    state.pop("review_binding", None)
    write_json_atomic(output / STATE_NAME, state)
    generated_targets: list[Path] = []
    try:
        validate_and_render(
            output,
            design,
            assets,
            structure,
            bool(args.allow_warnings or state.get("check", {}).get("allow_warnings")),
            transparent_bg,
        )
        used: set[str] = set()
        for index, element_path in enumerate(args.element_path, start=1):
            slug = safe_slug(element_path)
            if slug in used:
                slug = f"{slug}_{index}"
            used.add(slug)
            image_path = output / f"target_{slug}.png"
            legend_path = output / f"target_{slug}_legend.json"
            unlink_if_exists(image_path)
            unlink_if_exists(legend_path)
            run_script(
                "annotate_element.py",
                "--design",
                str(design),
                "--structure",
                str(structure),
                "--element-path",
                element_path,
                "--output",
                str(image_path),
                "--legend",
                str(legend_path),
            )
            generated_targets.extend((image_path, legend_path))
        visual_audit(output, design, fail_on_error=True)
        review_risk(output, design)
        ensure_inputs_current(state, design, assets)
        current_artifacts = check_artifact_snapshots(output)
        replaced_preserved = [
            name
            for name in ("bbox", "bbox_legend", "assets_inventory")
            if current_artifacts[name]["sha256"]
            != verified_artifacts[name]["sha256"]
        ]
        if replaced_preserved:
            raise WorkflowError(
                "Preserved check artifacts changed during target: "
                + ", ".join(replaced_preserved)
                + ". They were not registered; run check again."
            )
        verify_render_dependencies(output, state)
    except WorkflowError as exc:
        state["status"] = "target_failed"
        state["error"] = str(exc)
        write_json_atomic(output / STATE_NAME, state)
        raise

    state["status"] = "checked"
    state["target_count"] = int(state.get("target_count", 0) or 0) + len(
        args.element_path
    )
    state["evidence_generation"] = int(
        state.get("evidence_generation", 0) or 0
    ) + 1
    state["targeted_at"] = utc_now()
    state["check"]["artifact_snapshots"] = current_artifacts
    targets_state = state.get("targets")
    if not isinstance(targets_state, dict):
        targets_state = {
            "structure_sha256": state["structure"]["sha256"],
            "artifacts": {},
        }
    target_artifacts = targets_state.setdefault("artifacts", {})
    for path in generated_targets:
        target_artifacts[path.name] = artifact_snapshot(path)
    targets_state["structure_sha256"] = state["structure"]["sha256"]
    state["targets"] = targets_state
    write_json_atomic(output / STATE_NAME, state)
    print(f"\nTarget checks passed: {len(args.element_path)}")
    print(f"Refreshed comparison: {output / 'comparison.png'}")
    print(f"Review binding: {review_binding_comment(state)}")


def verify_check_artifacts(
    output: Path, state: dict[str, Any]
) -> dict[str, dict[str, Any]]:
    expected = state.get("check", {}).get("artifact_snapshots")
    if not isinstance(expected, dict):
        raise WorkflowError("Check artifact hashes are missing; run check again.")
    current = check_artifact_snapshots(output)
    missing = [
        name
        for name in current
        if not isinstance(expected.get(name), dict)
        or not isinstance(expected[name].get("sha256"), str)
    ]
    if missing:
        raise WorkflowError(
            "Check artifact hashes are incomplete: "
            + ", ".join(missing)
            + ". Run check again."
        )
    stale = [
        name for name, snapshot in current.items()
        if expected.get(name, {}).get("sha256") != snapshot["sha256"]
    ]
    if stale:
        raise WorkflowError(
            "Check artifacts changed outside the workflow: "
            + ", ".join(stale)
            + ". Run check again."
        )
    return current


def expected_review_binding(state: dict[str, Any]) -> dict[str, str]:
    structure_sha = state.get("structure", {}).get("sha256")
    snapshots = state.get("check", {}).get("artifact_snapshots")
    evidence_generation = state.get("evidence_generation")
    if (
        not isinstance(structure_sha, str)
        or not isinstance(snapshots, dict)
        or not isinstance(evidence_generation, int)
        or isinstance(evidence_generation, bool)
        or evidence_generation < 1
    ):
        raise WorkflowError("Review binding inputs are missing; run check again.")
    missing = [
        name
        for name in REVIEW_BINDING_ARTIFACTS
        if not isinstance(snapshots.get(name), dict)
        or not isinstance(snapshots[name].get("sha256"), str)
    ]
    if missing:
        raise WorkflowError(
            "Review binding artifact hashes are missing: "
            + ", ".join(missing)
            + ". Run check again."
        )
    check_hashes = {
        name: snapshots[name]["sha256"]
        for name in REVIEW_BINDING_ARTIFACTS
    }
    target_hashes: dict[str, str] = {}
    targets = state.get("targets")
    if targets is not None:
        target_snapshots = (
            targets.get("artifacts") if isinstance(targets, dict) else None
        )
        if not isinstance(target_snapshots, dict):
            raise WorkflowError(
                "Review binding target artifact hashes are missing; run target again."
            )
        missing_targets = [
            name
            for name, snapshot in target_snapshots.items()
            if not isinstance(name, str)
            or not isinstance(snapshot, dict)
            or not isinstance(snapshot.get("sha256"), str)
        ]
        if missing_targets:
            raise WorkflowError(
                "Review binding target artifact hashes are incomplete; run target "
                "again."
            )
        target_hashes = {
            name: snapshot["sha256"]
            for name, snapshot in target_snapshots.items()
        }
    artifacts_sha = hashlib.sha256(
        json.dumps(
            {
                "generation": evidence_generation,
                "check": check_hashes,
                "targets": target_hashes,
            },
            sort_keys=True,
            separators=(",", ":"),
        ).encode("utf-8")
    ).hexdigest()
    return {
        "structure_sha256": structure_sha.lower(),
        "artifacts_sha256": artifacts_sha,
    }


def review_binding_comment(state: dict[str, Any]) -> str:
    binding = expected_review_binding(state)
    return (
        "<!-- image-to-ui-review-binding: "
        f"structure={binding['structure_sha256']} "
        f"artifacts={binding['artifacts_sha256']} -->"
    )


def validate_review_binding(
    review_text: str, state: dict[str, Any]
) -> tuple[bool, str | None]:
    expected = expected_review_binding(state)
    expected_comment = review_binding_comment(state)
    matches = list(REVIEW_BINDING_PATTERN.finditer(review_text))
    marker_count = len(REVIEW_BINDING_MARKER.findall(review_text))
    if marker_count == 0:
        return False, (
            "Alignment review is missing a hash binding. After inspecting the "
            f"current artifacts, add this exact comment: {expected_comment}"
        )
    if len(matches) != 1 or marker_count != 1:
        return False, (
            "Alignment review hash binding is malformed or duplicated. Replace "
            f"it with this exact comment: {expected_comment}"
        )
    actual = {
        "structure_sha256": matches[0].group(1).lower(),
        "artifacts_sha256": matches[0].group(2).lower(),
    }
    mismatches = [
        label
        for label, key in (
            ("structure", "structure_sha256"),
            ("evidence artifacts", "artifacts_sha256"),
        )
        if actual[key] != expected[key]
    ]
    if mismatches:
        return False, (
            "Alignment review hash binding does not match the current "
            + " and ".join(mismatches)
            + ". Reinspect all_elements.png and comparison.png, then replace "
            f"the binding with: {expected_comment}"
        )
    return True, None


def parse_review_rows(review_text: str) -> tuple[dict[str, dict[str, Any]], list[str]]:
    rows: dict[str, dict[str, Any]] = {}
    duplicates: list[str] = []
    for line in review_text.splitlines():
        stripped = line.strip()
        if not stripped.startswith("|") or not stripped.endswith("|"):
            continue
        cells = [cell.strip() for cell in stripped.strip("|").split("|")]
        if len(cells) < 2:
            continue
        path_match = re.fullmatch(r"`([^`]+)`", cells[0])
        if not path_match:
            continue
        path = path_match.group(1)
        if path in rows:
            duplicates.append(path)
            continue
        rows[path] = {
            "status": cells[1].strip().lower(),
            "detail": cells[2].strip() if len(cells) > 2 else "",
            "recheck": cells[3].strip() if len(cells) > 3 else "",
        }
    return rows, duplicates


def review_evidence_paths(output: Path, value: str) -> tuple[list[Path], list[str]]:
    paths: list[Path] = []
    invalid: list[str] = []
    for token in re.findall(r"`([^`]+\.png)`", value, flags=re.IGNORECASE):
        candidate = Path(token)
        if candidate.is_absolute():
            resolved_candidate = candidate.resolve()
        else:
            resolved_candidate = (output / candidate).resolve()
        if not is_within(resolved_candidate, output):
            invalid.append(token)
            continue
        if (
            resolved_candidate.parent == output
            and not resolved_candidate.is_file()
        ):
            matches = [
                path.resolve()
                for path in output.iterdir()
                if path.is_file()
                and path.name.casefold() == resolved_candidate.name.casefold()
            ]
            if len(matches) == 1:
                resolved_candidate = matches[0]
            elif len(matches) > 1:
                invalid.append(f"{token} (ambiguous filename casing)")
                continue
        paths.append(resolved_candidate)
    return paths, invalid


def snapshot_for_name(
    snapshots: dict[str, Any], name: str
) -> dict[str, Any] | None:
    matches = [
        value
        for key, value in snapshots.items()
        if isinstance(key, str)
        and key.casefold() == name.casefold()
        and isinstance(value, dict)
    ]
    return matches[0] if len(matches) == 1 else None


def snapshot_matches_file(snapshot: Any, path: Path) -> bool:
    return (
        isinstance(snapshot, dict)
        and isinstance(snapshot.get("sha256"), str)
        and path.is_file()
        and snapshot["sha256"] == sha256_file(path)
    )


def target_legend_covers_path(legend: dict[str, Any], review_path: str) -> bool:
    covered = {
        item.get("path")
        for item in legend.get("targets", [])
        if isinstance(item, dict) and isinstance(item.get("path"), str)
    }
    element_path = legend.get("element_path")
    if isinstance(element_path, str):
        covered.add(element_path)
    return review_path in covered


def review_risk_by_path(output: Path) -> tuple[dict[str, dict[str, Any]], dict[str, Any]]:
    report = read_json_file(output / "review_risk.json", "review risk report")
    items = {
        item["path"]: item
        for item in report.get("items", [])
        if isinstance(item, dict) and isinstance(item.get("path"), str)
    }
    return items, report


def review_coverage(
    output: Path, state: dict[str, Any]
) -> tuple[dict[str, Any], list[str]]:
    legend_path = output / "all_elements_legend.json"
    review_path = output / "alignment_review.md"
    require_file(legend_path, "All-elements legend")
    legend = read_json_file(legend_path, "all-elements legend")
    expected = [
        item.get("path")
        for item in legend.get("targets", [])
        if isinstance(item, dict) and isinstance(item.get("path"), str)
    ]
    blockers: list[str] = []
    if not review_path.is_file():
        review_text = ""
        blockers.append(f"Alignment review not found: {review_path}")
    else:
        review_text = review_path.read_text(encoding="utf-8")
    rows, duplicates = parse_review_rows(review_text)
    reviewed = [path for path in expected if path in rows]
    missing = [path for path in expected if path not in rows]
    if not expected:
        blockers.append(
            "All-elements legend has no review targets; a non-empty UI cannot be "
            "completed from an empty structure."
        )
    if missing:
        blockers.append(
            f"Alignment review is missing {len(missing)} of {len(expected)} element paths."
        )
    if duplicates:
        blockers.append(
            "Alignment review has duplicate element rows: "
            + ", ".join(sorted(set(duplicates)))
        )

    allowed_statuses = {"aligned", "adjusted", "skipped"}
    invalid_statuses: list[str] = []
    missing_reasons: list[str] = []
    missing_evidence: list[str] = []
    invalid_evidence: list[str] = []
    evidence_files: set[str] = set()
    check_snapshots = state.get("check", {}).get("artifact_snapshots", {})
    risk_items, risk_report = review_risk_by_path(output)
    high_risk_paths = {
        path for path in expected
        if risk_items.get(path, {}).get("risk_level") == "high"
    }
    focused_high_risk_paths: set[str] = set()
    missing_high_risk_evidence: list[str] = []
    overview_artifacts = {
        "all_elements.png": "bbox",
        "comparison.png": "comparison",
    }
    for path in reviewed:
        row = rows[path]
        status = row["status"]
        if status not in allowed_statuses:
            invalid_statuses.append(f"{path}={status or '<empty>'}")
            continue
        if status == "skipped":
            detail = row["detail"]
            if len(detail.strip(" .-")) < 8:
                missing_reasons.append(path)
            continue
        evidence, invalid = review_evidence_paths(output, row["recheck"])
        invalid_evidence.extend(f"{path}: {item}" for item in invalid)
        if not evidence:
            missing_evidence.append(path)
            if path in high_risk_paths:
                missing_high_risk_evidence.append(path)
            continue
        has_focused_evidence = False
        for evidence_path in evidence:
            if not evidence_path.is_file():
                invalid_evidence.append(f"{path}: missing {evidence_path.name}")
                continue
            if evidence_path.parent != output:
                invalid_evidence.append(
                    f"{path}: {evidence_path.name} must be directly under {output}"
                )
                continue
            evidence_name = evidence_path.name.casefold()
            overview_key = overview_artifacts.get(evidence_name)
            if overview_key is not None:
                if not snapshot_matches_file(
                    check_snapshots.get(overview_key), evidence_path
                ):
                    invalid_evidence.append(
                        f"{path}: {evidence_path.name} does not match the current "
                        "check artifact hash"
                    )
                    continue
            elif evidence_name == "risk_review.png":
                risk_legend_path = output / "risk_review_legend.json"
                if (
                    not snapshot_matches_file(
                        check_snapshots.get("risk_review"), evidence_path
                    )
                    or not snapshot_matches_file(
                        check_snapshots.get("risk_review_legend"),
                        risk_legend_path,
                    )
                ):
                    invalid_evidence.append(
                        f"{path}: risk_review.png or its legend does not match "
                        "the current check artifact hash"
                    )
                    continue
                risk_legend = read_json_file(
                    risk_legend_path, "review risk evidence legend"
                )
                if not target_legend_covers_path(risk_legend, path):
                    invalid_evidence.append(
                        "risk_review_legend.json does not cover review path "
                        f"{path}; run a targeted review for that path"
                    )
                    continue
                has_focused_evidence = True
            elif evidence_name.startswith("target_"):
                target_legend_path = evidence_path.with_name(
                    f"{evidence_path.stem}_legend.json"
                )
                if not target_legend_path.is_file():
                    invalid_evidence.append(
                        f"{path}: missing {target_legend_path.name} for targeted evidence"
                    )
                    continue
                target_state = state.get("targets") or {}
                target_snapshots = target_state.get("artifacts") or {}
                expected_target = snapshot_for_name(
                    target_snapshots, evidence_path.name
                )
                expected_legend = snapshot_for_name(
                    target_snapshots, target_legend_path.name
                )
                if (
                    target_state.get("structure_sha256")
                    != state.get("structure", {}).get("sha256")
                    or not snapshot_matches_file(expected_target, evidence_path)
                    or not snapshot_matches_file(
                        expected_legend, target_legend_path
                    )
                ):
                    invalid_evidence.append(
                        f"{path}: targeted evidence is stale or was not generated "
                        "by workflow target"
                    )
                    continue
                target_legend = read_json_file(
                    target_legend_path, "target evidence legend"
                )
                if not target_legend_covers_path(target_legend, path):
                    invalid_evidence.append(
                        f"{target_legend_path.name} does not cover review path {path}; "
                        "legend.element_path or targets[].path must match"
                    )
                    continue
                has_focused_evidence = True
            else:
                invalid_evidence.append(
                    f"{path}: {evidence_path.name} is not an allowed recheck PNG; "
                    "use current all_elements.png, comparison.png, risk_review.png, "
                    "or a target_*.png registered by workflow target"
                )
                continue
            evidence_files.add(str(evidence_path))
        if path in high_risk_paths:
            if has_focused_evidence:
                focused_high_risk_paths.add(path)
            else:
                missing_high_risk_evidence.append(path)

    if invalid_statuses:
        blockers.append(
            "Alignment review has unresolved/invalid statuses: "
            + ", ".join(invalid_statuses)
        )
    if missing_reasons:
        blockers.append(
            "Skipped review rows require a concrete reason: "
            + ", ".join(missing_reasons)
        )
    if missing_evidence:
        blockers.append(
            "Aligned/adjusted review rows require a recheck PNG: "
            + ", ".join(missing_evidence)
        )
    if invalid_evidence:
        blockers.append(
            "Alignment review references invalid evidence: "
            + "; ".join(invalid_evidence)
        )
    if missing_high_risk_evidence:
        blockers.append(
            "High-risk review rows require current focused evidence "
            "(risk_review.png or a covering target_*.png): "
            + ", ".join(missing_high_risk_evidence)
        )

    review_fresh = False
    if review_path.is_file():
        review_fresh, binding_error = validate_review_binding(review_text, state)
        if binding_error:
            blockers.append(binding_error)

    status_counts = {
        status: sum(1 for path in reviewed if rows[path]["status"] == status)
        for status in sorted(allowed_statuses)
    }
    return {
        "required": len(expected),
        "reviewed": len(reviewed),
        "complete": not blockers,
        "missing": missing,
        "status_counts": status_counts,
        "evidence_count": len(evidence_files),
        "evidence": sorted(evidence_files),
        "risk_strategy": risk_report.get("strategy"),
        "risk_level_counts": (
            risk_report.get("summary", {}).get("risk_level_counts", {})
        ),
        "high_risk_required": sum(
            1 for path in high_risk_paths
            if rows.get(path, {}).get("status") != "skipped"
        ),
        "high_risk_focused": len(focused_high_risk_paths),
        "missing_high_risk_evidence": missing_high_risk_evidence,
        "fresh": review_fresh,
        "review": str(review_path),
        "expected_binding": review_binding_comment(state),
    }, blockers


def approximation_gate(
    structure: dict[str, Any], valid_paths: set[str]
) -> tuple[dict[str, Any], list[str]]:
    metadata = structure.get("metadata") or {}
    raw = metadata.get("approximations", []) if isinstance(metadata, dict) else []
    blockers: list[str] = []
    if not isinstance(raw, list):
        return {
            "total": 0,
            "accepted": 0,
            "items": [],
        }, ["metadata.approximations must be a list."]

    items = []
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            blockers.append(f"metadata.approximations[{index}] must be an object.")
            continue
        path = item.get("path")
        reason = item.get("reason")
        accepted = item.get("accepted") is True
        if not isinstance(path, str) or not path:
            blockers.append(f"metadata.approximations[{index}].path is required.")
        elif path not in valid_paths:
            blockers.append(f"Approximation path is not in the review legend: {path}")
        if not isinstance(reason, str) or not reason.strip():
            blockers.append(f"metadata.approximations[{index}].reason is required.")
        if not accepted:
            blockers.append(f"Approximation is not accepted: {path or index}")
        items.append({
            "path": path,
            "kind": item.get("kind", "other"),
            "reason": reason,
            "accepted": accepted,
        })
    return {
        "total": len(items),
        "accepted": sum(item["accepted"] for item in items),
        "items": items,
    }, blockers


def audit_warning_gate(
    structure: dict[str, Any], audit: dict[str, Any]
) -> tuple[dict[str, Any], list[str]]:
    metadata = structure.get("metadata") or {}
    raw = metadata.get("auditWaivers", []) if isinstance(metadata, dict) else []
    blockers: list[str] = []
    if not isinstance(raw, list):
        return {"required": 0, "accepted": 0, "items": []}, [
            "metadata.auditWaivers must be a list."
        ]

    waivers: dict[tuple[str, str], dict[str, Any]] = {}
    for index, item in enumerate(raw):
        if not isinstance(item, dict):
            blockers.append(f"metadata.auditWaivers[{index}] must be an object.")
            continue
        path = item.get("path")
        code = item.get("code")
        reason = item.get("reason")
        accepted = item.get("accepted") is True
        if not isinstance(path, str) or not path:
            blockers.append(f"metadata.auditWaivers[{index}].path is required.")
            continue
        if not isinstance(code, str) or not code:
            blockers.append(f"metadata.auditWaivers[{index}].code is required.")
            continue
        if not isinstance(reason, str) or len(reason.strip()) < 8:
            blockers.append(
                f"metadata.auditWaivers[{index}].reason must explain the warning."
            )
        if not accepted:
            blockers.append(f"Audit waiver is not accepted: {path} ({code})")
        key = (path, code)
        if key in waivers:
            blockers.append(f"Duplicate audit waiver: {path} ({code})")
        waivers[key] = {
            "path": path,
            "code": code,
            "reason": reason,
            "accepted": accepted,
        }

    warnings = [
        item for item in audit.get("issues", [])
        if isinstance(item, dict) and item.get("severity") == "warning"
    ]
    required_keys = {
        (str(item.get("path", "")), str(item.get("code", "")))
        for item in warnings
    }
    missing = sorted(required_keys - set(waivers))
    stale = sorted(set(waivers) - required_keys)
    if missing:
        blockers.append(
            "Unaccepted visual-audit warnings: "
            + ", ".join(f"{path} ({code})" for path, code in missing)
        )
    if stale:
        blockers.append(
            "Stale visual-audit waivers no longer match a warning: "
            + ", ".join(f"{path} ({code})" for path, code in stale)
        )
    accepted_items = [waivers[key] for key in sorted(required_keys & set(waivers))]
    return {
        "required": len(required_keys),
        "accepted": sum(item["accepted"] for item in accepted_items),
        "items": accepted_items,
    }, blockers


def finalize(args: argparse.Namespace) -> None:
    output = resolved(args.output)
    state = read_state(output)
    previous_status = state.get("status")
    clean_finalize_artifacts(output)
    state["status"] = "finalizing"
    state.pop("error", None)
    state.pop("completion", None)
    state.pop("completed_at", None)
    state.pop("finalized_at", None)
    state.pop("review_binding", None)
    write_json_atomic(output / STATE_NAME, state)

    try:
        output, design, assets, structure_path, state = checked_context(
            args.output, args.structure
        )
        if previous_status not in {"checked", "completed", "finalize_failed"}:
            raise WorkflowError("Run a successful full check before finalize.")
        if state.get("structure", {}).get("sha256") != sha256_file(structure_path):
            raise WorkflowError(
                "Structure changed after the full check; run check again."
            )
        verify_check_artifacts(output, state)
        verify_render_dependencies(output, state)
        visual_audit(output, design, evidence=True)
        ensure_inputs_current(state, design, assets)
        coverage, blockers = review_coverage(output, state)
        structure = read_json_file(structure_path, "structure JSON")
        valid_paths = set()
        legend = read_json_file(
            output / "all_elements_legend.json", "all-elements legend"
        )
        for item in legend.get("targets", []):
            if isinstance(item, dict) and isinstance(item.get("path"), str):
                valid_paths.add(item["path"])
        approximations, approximation_blockers = approximation_gate(
            structure, valid_paths
        )
        blockers.extend(approximation_blockers)
        validation = read_json_file(
            output / "validate_report.json", "validation report"
        )
        audit = read_json_file(output / "visual_audit.json", "visual audit")
        risk = read_json_file(output / "review_risk.json", "review risk report")
        audit_waivers, audit_waiver_blockers = audit_warning_gate(
            structure, audit
        )
        blockers.extend(audit_waiver_blockers)
        if not validation.get("valid", False):
            blockers.append("Structure validation report is not valid.")
        warning_count = int(validation.get("warning_count", 0) or 0)
        if warning_count:
            blockers.append(
                f"Structure validation still has {warning_count} warning(s); "
                "finalize requires a warning-free check."
            )
        if not audit.get("valid", False):
            blockers.append(
                f"Visual audit has {audit.get('error_count', '?')} blocking errors."
            )

        current_artifacts = check_artifact_snapshots(output)
        current_artifacts["detail_comparison"] = artifact_snapshot(
            output / "detail_comparison.png"
        )
        review_path = output / "alignment_review.md"
        if review_path.is_file():
            current_artifacts["alignment_review"] = artifact_snapshot(review_path)
        completed_at = utc_now()
        report = {
            "complete": not blockers,
            "completed_at": completed_at if not blockers else None,
            "structure": design_snapshot(structure_path),
            "iterations": {
                "check_count": int(state.get("check_count", 0) or 0),
                "structure_revision": int(state.get("structure_revision", 0) or 0),
                "target_count": int(state.get("target_count", 0) or 0),
            },
            "review_coverage": coverage,
            "validation": {
                "valid": bool(validation.get("valid")),
                "error_count": validation.get("error_count", 0),
                "warning_count": validation.get("warning_count", 0),
            },
            "visual_audit": {
                "valid": bool(audit.get("valid")),
                "error_count": audit.get("error_count", 0),
                "warning_count": audit.get("warning_count", 0),
                "elapsed_ms": audit.get("elapsed_ms"),
            },
            "review_risk": {
                "strategy": risk.get("strategy"),
                "thresholds": risk.get("thresholds", {}),
                "summary": risk.get("summary", {}),
                "evidence": risk.get("evidence", {}),
            },
            "approximations": approximations,
            "audit_waivers": audit_waivers,
            "artifact_snapshots": current_artifacts,
            "blockers": blockers,
        }
        report_path = output / "completion_report.json"
        write_json_atomic(report_path, report)
    except WorkflowError as exc:
        state["status"] = "finalize_failed"
        state["error"] = str(exc)
        write_json_atomic(output / STATE_NAME, state)
        raise

    state["check"]["artifact_snapshots"] = check_artifact_snapshots(output)
    state["finalize_count"] = int(state.get("finalize_count", 0) or 0) + 1
    state["finalize_attempted_at"] = utc_now()
    state["completion"] = artifact_snapshot(output / "completion_report.json")
    if blockers:
        state["status"] = "finalize_failed"
        state["error"] = blockers[0]
        write_json_atomic(output / STATE_NAME, state)
        raise WorkflowError(
            f"Finalize blocked by {len(blockers)} issue(s). See "
            f"{output / 'completion_report.json'}"
        )

    state["status"] = "completed"
    state["completed_at"] = completed_at
    state["finalized_at"] = completed_at
    state["review_binding"] = expected_review_binding(state)
    state["review_binding"]["review_sha256"] = sha256_file(
        output / "alignment_review.md"
    )
    state.pop("error", None)
    write_json_atomic(output / STATE_NAME, state)
    print(f"\nTask completed: {output}")
    print(f"Completion report: {output / 'completion_report.json'}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    prepare_parser = commands.add_parser(
        "prepare", help="Inventory assets and annotate the design grid"
    )
    prepare_parser.add_argument("--design", required=True)
    prepare_parser.add_argument("--assets", required=True)
    prepare_parser.add_argument("--output", required=True)
    prepare_parser.set_defaults(handler=prepare)

    measure_parser = commands.add_parser(
        "measure", help="Crop a selected design region with absolute pixel rulers"
    )
    measure_parser.add_argument("--output", required=True)
    measure_parser.add_argument("--region", type=int, nargs=4, required=True,
                                metavar=("X", "Y", "WIDTH", "HEIGHT"))
    measure_parser.add_argument("--zoom", type=int, default=3)
    measure_parser.add_argument("--cell-size", type=int, default=10)
    measure_parser.set_defaults(handler=measure)

    check_parser = commands.add_parser(
        "check", help="Validate, annotate all bboxes, and render a comparison"
    )
    check_parser.add_argument("--output", required=True)
    check_parser.add_argument(
        "--structure", help="Defaults to <output>/ui_structure.json"
    )
    check_parser.add_argument("--transparent-bg", action="store_true")
    check_parser.add_argument(
        "--allow-warnings",
        action="store_true",
        help="Accept validation warnings after they have been reviewed",
    )
    check_parser.set_defaults(handler=check)

    preview_parser = commands.add_parser(
        "preview", help="Render a checked structure at another size/state/progress/scroll position"
    )
    preview_parser.add_argument("--output", required=True)
    preview_parser.add_argument("--structure")
    preview_parser.add_argument("--name", required=True, help="Scenario output folder name")
    preview_parser.add_argument("--size", nargs=2, type=int, metavar=("WIDTH", "HEIGHT"))
    preview_parser.add_argument("--state", action="append", help="PATH=VARIANT, repeatable")
    preview_parser.add_argument("--progress", action="append", help="PATH=VALUE, repeatable")
    preview_parser.add_argument("--scroll", action="append", help="PATH=X,Y, repeatable")
    preview_parser.set_defaults(handler=preview)

    target_parser = commands.add_parser(
        "target", help="Validate, refresh comparison, and annotate focused bboxes"
    )
    target_parser.add_argument("--output", required=True)
    target_parser.add_argument(
        "--structure", help="Defaults to <output>/ui_structure.json"
    )
    target_parser.add_argument(
        "--element-path", action="append", required=True
    )
    target_parser.add_argument(
        "--transparent-bg",
        action="store_true",
        help="Use a transparent reconstruction background; otherwise reuse "
             "the latest check mode",
    )
    target_parser.add_argument("--allow-warnings", action="store_true")
    target_parser.set_defaults(handler=target)

    finalize_parser = commands.add_parser(
        "finalize", help="Verify review coverage, audit details, and seal outputs"
    )
    finalize_parser.add_argument("--output", required=True)
    finalize_parser.add_argument(
        "--structure", help="Defaults to <output>/ui_structure.json"
    )
    finalize_parser.set_defaults(handler=finalize)
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    try:
        args.handler(args)
    except WorkflowError as exc:
        print(f"Workflow error: {exc}", file=sys.stderr)
        raise SystemExit(1) from exc


if __name__ == "__main__":
    main()
