from __future__ import annotations

import copy
import json
import os
import subprocess
import sys
import tempfile
import unittest
from io import BytesIO
from pathlib import Path

from PIL import Image


WORKFLOW = Path(__file__).resolve().parents[1] / "scripts" / "workflow.py"
sys.path.insert(0, str(WORKFLOW.parent))
import annotate_element  # noqa: E402
import audit_render  # noqa: E402
import workflow as workflow_module  # noqa: E402


DEFAULT_ANCHOR = {"horizontal": "left", "vertical": "top"}


def add_default_anchors(element: dict) -> None:
    element.setdefault("anchor", dict(DEFAULT_ANCHOR))
    for child in element.get("children") or []:
        if isinstance(child, dict):
            add_default_anchors(child)


class WorkflowIntegrationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temporary = tempfile.TemporaryDirectory()
        self.root = Path(self.temporary.name)
        self.design = self.root / "design.png"
        self.assets = self.root / "assets"
        self.output = self.root / "output"
        self.assets.mkdir()
        Image.new("RGB", (64, 64), "black").save(self.design)
        Image.new("RGBA", (8, 8), "white").save(self.assets / "probe.png")

    def tearDown(self) -> None:
        self.temporary.cleanup()

    def run_workflow(self, *arguments: str, expect_success: bool = True) -> subprocess.CompletedProcess[str]:
        result = subprocess.run(
            [sys.executable, "-B", str(WORKFLOW), *arguments],
            text=True,
            capture_output=True,
            check=False,
        )
        if expect_success and result.returncode:
            self.fail(f"workflow failed:\n{result.stdout}\n{result.stderr}")
        return result

    def prepare(self) -> None:
        self.run_workflow(
            "prepare",
            "--design", str(self.design),
            "--assets", str(self.assets),
            "--output", str(self.output),
        )

    def write_structure(self, children: list[dict]) -> None:
        structure = {
            "canvas": {"width": 64, "height": 64},
            "metadata": {"approximations": [], "auditWaivers": []},
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": copy.deepcopy(children),
            },
        }
        add_default_anchors(structure["root"])
        (self.output / "ui_structure.json").write_text(
            json.dumps(structure), encoding="utf-8"
        )

    def read_state(self) -> dict:
        return json.loads(
            (self.output / "workflow_state.json").read_text(encoding="utf-8")
        )

    def write_state(self, state: dict) -> None:
        (self.output / "workflow_state.json").write_text(
            json.dumps(state, indent=2) + "\n", encoding="utf-8"
        )

    def review_binding_comment(self) -> str:
        return workflow_module.review_binding_comment(self.read_state())

    def write_review(
        self,
        rows: list[tuple[str, str, str, str]],
        *,
        include_binding: bool = True,
    ) -> None:
        lines = []
        if include_binding:
            lines.extend((self.review_binding_comment(), ""))
        lines.extend((
            "| Element path | Status | Review / change | Recheck |",
            "| --- | --- | --- | --- |",
        ))
        lines.extend(
            f"| `{path}` | {status} | {detail} | `{evidence}` |"
            for path, status, detail, evidence in rows
        )
        (self.output / "alignment_review.md").write_text(
            "\n".join(lines) + "\n", encoding="utf-8"
        )

    def write_valid_review(
        self,
        path: str = "root/probe",
        evidence: str | None = None,
    ) -> None:
        if evidence is None:
            risk = json.loads(
                (self.output / "review_risk.json").read_text(encoding="utf-8")
            )
            item = next(
                (
                    item for item in risk.get("items", [])
                    if item.get("path") == path
                ),
                {},
            )
            evidence = (
                "risk_review.png"
                if item.get("risk_level") == "high"
                and item.get("risk_review_covered") is True
                else "all_elements.png"
            )
        self.write_review([(
            path,
            "aligned",
            "Current output inspected.",
            evidence,
        )])

    def test_measure_before_authoring_preserves_prepared_state(self) -> None:
        self.prepare()
        before = (self.output / "workflow_state.json").read_bytes()
        self.run_workflow(
            "measure", "--output", str(self.output),
            "--region", "13", "17", "21", "19", "--zoom", "3",
        )
        region = self.output / "measurements" / "region_13_17_21_19"
        metrics = json.loads((region / "metrics.json").read_text(encoding="utf-8"))
        with Image.open(region / "design.png") as crop:
            self.assertEqual(crop.size, (63, 57))
        self.assertEqual(metrics["region"], {"x": 13, "y": 17, "width": 21, "height": 19})
        self.assertEqual(metrics["design_sha256"], self.read_state()["inputs"]["design"]["sha256"])
        self.assertEqual((self.output / "workflow_state.json").read_bytes(), before)
        self.assertFalse((self.output / "ui_structure.json").exists())

    def test_measure_rejects_inputs_changed_after_prepare(self) -> None:
        self.prepare()
        Image.new("RGB", (64, 64), "red").save(self.design)
        result = self.run_workflow(
            "measure", "--output", str(self.output),
            "--region", "0", "0", "10", "10", expect_success=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Rerun prepare", result.stderr)
        self.assertFalse((self.output / "measurements").exists())

    def test_measure_preserves_checked_render_and_review_binding(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect", "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20}, "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        before = self.review_binding_comment()
        self.run_workflow(
            "measure", "--output", str(self.output),
            "--region", "5", "5", "30", "30",
        )
        self.assertEqual(self.review_binding_comment(), before)
        self.run_workflow("finalize", "--output", str(self.output))
        self.assertEqual(self.read_state()["status"], "completed")

    def test_component_preview_preserves_native_evidence_and_rejects_bad_states(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "container", "name": "meter",
            "position": {"x": 10, "y": 10}, "size": {"width": 44, "height": 10},
            "responsive": {
                "min": {"x": 0, "y": 0}, "max": {"x": 1, "y": 0},
                "offsetMin": {"x": 10, "y": 10}, "offsetMax": {"x": -10, "y": 20},
            },
            "progress": {"fill": "fill", "value": 1, "direction": "left-to-right"},
            "state": {"current": "normal", "variants": {
                "normal": {}, "selected": {"track": {"color": "#FF0000"}},
            }},
            "children": [
                {
                    "type": "rect", "name": name, "color": color,
                    "size": {"width": 44, "height": 10},
                    "responsive": {
                        "min": {"x": 0, "y": 0}, "max": {"x": 1, "y": 1},
                        "offsetMin": {"x": 0, "y": 0}, "offsetMax": {"x": 0, "y": 0},
                    },
                }
                for name, color in (("track", "#303030"), ("fill", "#00FF00"))
            ],
        }])
        self.run_workflow("check", "--output", str(self.output))
        original = (self.output / "ui_structure.json").read_bytes()
        baseline = (self.output / "reconstruction.png").read_bytes()
        binding = self.review_binding_comment()
        self.run_workflow(
            "preview", "--output", str(self.output), "--name", "wide-half",
            "--size", "100", "80", "--progress", "root/meter=0.5",
            "--state", "root/meter=selected",
        )
        preview = self.output / "previews" / "wide-half"
        with Image.open(preview / "reconstruction.png") as image:
            self.assertEqual(image.size, (100, 80))
            self.assertEqual(image.getpixel((49, 15)), (0, 255, 0, 255))
            self.assertEqual(image.getpixel((50, 15)), (255, 0, 0, 255))
        report = json.loads((preview / "preview_report.json").read_text(encoding="utf-8"))
        self.assertTrue(report["valid"])
        self.assertEqual((self.output / "ui_structure.json").read_bytes(), original)
        self.assertEqual((self.output / "reconstruction.png").read_bytes(), baseline)
        self.assertEqual(self.review_binding_comment(), binding)
        result = self.run_workflow(
            "preview", "--output", str(self.output), "--name", "wide-half",
            "--state", "root/meter=missing", expect_success=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((preview / "reconstruction.png").exists())
        self.assertFalse((preview / "preview_report.json").exists())
        self.assertEqual(self.read_state()["status"], "checked")
        self.run_workflow("preview", "--output", str(self.output), "--name", "wide-half")
        invalid_number = self.run_workflow(
            "preview", "--output", str(self.output), "--name", "wide-half",
            "--progress", "root/meter=not-a-number", expect_success=False,
        )
        self.assertNotEqual(invalid_number.returncode, 0)
        self.assertFalse((preview / "preview_report.json").exists())
        self.assertFalse((preview / "reconstruction.png").exists())
        self.run_workflow("preview", "--output", str(self.output), "--name", "wide-half")
        self.run_workflow("check", "--output", str(self.output))
        self.assertFalse((preview / "preview_report.json").exists())

    def test_check_and_finalize_record_hashes_and_review_coverage(self) -> None:
        self.prepare()
        structure = {
            "canvas": {"width": 64, "height": 64},
            "metadata": {
                "approximations": [{
                    "path": "root/probe",
                    "kind": "test_fixture",
                    "reason": "The fixture intentionally uses a solid rectangle.",
                    "accepted": True,
                }]
            },
            "root": {
                "type": "container",
                "name": "root",
                "position": {"x": 0, "y": 0},
                "size": {"width": 64, "height": 64},
                "children": [{
                    "type": "rect",
                    "name": "probe",
                    "position": {"x": 10, "y": 10},
                    "size": {"width": 20, "height": 20},
                    "color": "#FFFFFF",
                }],
            },
        }
        add_default_anchors(structure["root"])
        (self.output / "ui_structure.json").write_text(
            json.dumps(structure), encoding="utf-8"
        )
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()

        self.run_workflow("finalize", "--output", str(self.output))

        state = json.loads((self.output / "workflow_state.json").read_text())
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertEqual(state["status"], "completed")
        self.assertEqual(state["check_count"], 1)
        self.assertEqual(state["structure_revision"], 1)
        self.assertTrue(report["complete"])
        self.assertEqual(report["review_coverage"]["reviewed"], 1)
        self.assertTrue((self.output / "reconstruction.png").is_file())
        self.assertTrue((self.output / "render_trace.json").is_file())
        self.assertTrue((self.output / "detail_comparison.png").is_file())

        self.run_workflow("finalize", "--output", str(self.output))
        retried_state = json.loads(
            (self.output / "workflow_state.json").read_text()
        )
        self.assertEqual(retried_state["status"], "completed")
        self.assertEqual(retried_state["finalize_count"], 2)

        with (self.output / "comparison.png").open("ab") as handle:
            handle.write(b"tampered")
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("changed outside the workflow", failed.stderr)

    def test_high_risk_review_requires_current_focused_evidence(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 8, "y": 8},
            "size": {"width": 40, "height": 40},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        risk = json.loads(
            (self.output / "review_risk.json").read_text(encoding="utf-8")
        )
        probe = next(
            item for item in risk["items"]
            if item["path"] == "root/probe"
        )
        self.assertEqual(probe["risk_level"], "high")
        self.assertTrue(probe["risk_review_covered"])

        self.write_valid_review(evidence="all_elements.png")
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed.returncode, 0)
        report = json.loads(
            (self.output / "completion_report.json").read_text(encoding="utf-8")
        )
        self.assertTrue(any(
            "High-risk review rows require current focused evidence" in blocker
            for blocker in report["blockers"]
        ))

        self.write_valid_review(evidence="risk_review.png")
        self.run_workflow("finalize", "--output", str(self.output))
        completed = json.loads(
            (self.output / "completion_report.json").read_text(encoding="utf-8")
        )
        self.assertTrue(completed["complete"])
        self.assertEqual(
            completed["review_coverage"]["high_risk_focused"], 1
        )
        self.assertEqual(
            completed["review_risk"]["strategy"],
            "visual-salience-plus-residual",
        )

    def test_finalize_rejects_tampered_risk_evidence(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 8, "y": 8},
            "size": {"width": 40, "height": 40},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        with (self.output / "risk_review.png").open("ab") as handle:
            handle.write(b"tampered")

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("risk_review", failed.stderr)
        self.assertIn("changed outside the workflow", failed.stderr)

    def test_failed_refinalize_invalidates_prior_completion(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        self.run_workflow("finalize", "--output", str(self.output))
        report_path = self.output / "completion_report.json"
        self.assertTrue(json.loads(report_path.read_text())["complete"])

        with (self.output / "comparison.png").open("ab") as handle:
            handle.write(b"tampered")
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        state = self.read_state()
        self.assertEqual(state["status"], "finalize_failed")
        self.assertNotIn("completion", state)
        self.assertNotIn("completed_at", state)
        self.assertNotIn("finalized_at", state)
        self.assertFalse(report_path.exists())

    def test_empty_structure_cannot_finalize(self) -> None:
        self.prepare()
        self.write_structure([])
        failed_check = self.run_workflow(
            "check", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed_check.returncode, 0)
        self.assertEqual(self.read_state()["status"], "check_failed")

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("successful full check", failed.stderr)

    def test_unresolved_review_status_cannot_finalize(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        (self.output / "alignment_review.md").write_text(
            self.review_binding_comment() + "\n\n"
            "| Element path | Status | Review / change | Recheck |\n"
            "| --- | --- | --- | --- |\n"
            "| `root/probe` | needs-targeted-check | Still uncertain. | - |\n",
            encoding="utf-8",
        )

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(any("invalid statuses" in item for item in report["blockers"]))

    def test_review_must_be_refreshed_after_structure_revision(self) -> None:
        self.prepare()
        child = {
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }
        self.write_structure([child])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        self.run_workflow("finalize", "--output", str(self.output))

        child["position"]["x"] = 12
        self.write_structure([child])
        self.run_workflow("check", "--output", str(self.output))
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(any("hash binding" in item for item in report["blockers"]))

    def test_each_check_and_target_advances_and_binds_review_evidence(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        first_state = self.read_state()
        first_binding = self.review_binding_comment()

        self.run_workflow("check", "--output", str(self.output))
        second_state = self.read_state()
        second_binding = self.review_binding_comment()
        self.assertEqual(second_state["evidence_generation"], 2)
        self.assertNotEqual(first_binding, second_binding)
        for name in workflow_module.REVIEW_BINDING_ARTIFACTS:
            self.assertEqual(
                first_state["check"]["artifact_snapshots"][name]["sha256"],
                second_state["check"]["artifact_snapshots"][name]["sha256"],
            )

        self.write_valid_review()
        review_before_target = (self.output / "alignment_review.md").read_text(
            encoding="utf-8"
        )
        self.run_workflow(
            "target",
            "--output", str(self.output),
            "--element-path", "root/probe",
        )
        target_state = self.read_state()
        target_binding = self.review_binding_comment()
        self.assertEqual(target_state["evidence_generation"], 3)
        self.assertNotEqual(second_binding, target_binding)
        for name in workflow_module.REVIEW_BINDING_ARTIFACTS:
            self.assertEqual(
                second_state["check"]["artifact_snapshots"][name]["sha256"],
                target_state["check"]["artifact_snapshots"][name]["sha256"],
            )

        target_names = (
            "target_root_probe.png",
            "target_root_probe_legend.json",
        )
        for name in target_names:
            with self.subTest(target_artifact=name):
                changed_state = copy.deepcopy(target_state)
                changed_state["targets"]["artifacts"][name]["sha256"] = "0" * 64
                self.assertNotEqual(
                    target_binding,
                    workflow_module.review_binding_comment(changed_state),
                )

        self.assertEqual(
            (self.output / "alignment_review.md").read_text(encoding="utf-8"),
            review_before_target,
        )
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(any("hash binding" in item for item in report["blockers"]))

        self.write_valid_review(evidence="target_root_probe.png")
        self.run_workflow("finalize", "--output", str(self.output))
        self.assertEqual(self.read_state()["status"], "completed")

    def test_target_rejects_replaced_overview_artifacts_before_reregistering(
        self,
    ) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        original_state = self.read_state()

        for key, name in (
            ("bbox", "all_elements.png"),
            ("bbox_legend", "all_elements_legend.json"),
        ):
            with self.subTest(artifact=name):
                artifact = self.output / name
                original = artifact.read_bytes()
                artifact.write_bytes(original + b"tampered")

                failed = self.run_workflow(
                    "target",
                    "--output", str(self.output),
                    "--element-path", "root/probe",
                    expect_success=False,
                )

                self.assertNotEqual(failed.returncode, 0)
                self.assertIn("Check artifacts changed outside the workflow", failed.stderr)
                current_state = self.read_state()
                self.assertEqual(
                    current_state["check"]["artifact_snapshots"][key]["sha256"],
                    original_state["check"]["artifact_snapshots"][key]["sha256"],
                )
                artifact.write_bytes(original)

    def test_review_requires_hash_binding_even_with_newer_mtime(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_review([(
            "root/probe",
            "aligned",
            "Current output inspected.",
            "all_elements.png",
        )], include_binding=False)
        review = self.output / "alignment_review.md"
        newest_artifact_mtime = max(
            item["mtime_ns"]
            for item in self.read_state()["check"]["artifact_snapshots"].values()
        )
        os.utime(
            review,
            ns=(newest_artifact_mtime + 1_000_000_000,) * 2,
        )

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(
            any("missing a hash binding" in item for item in report["blockers"])
        )

    def test_finalize_rejects_unregistered_recheck_png(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review(evidence="reconstruction.png")

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(any(
            "not an allowed recheck PNG" in item
            for item in report["blockers"]
        ))

    def test_current_overview_evidence_is_case_insensitive(self) -> None:
        matching_design = Image.new("RGB", (64, 64), "black")
        matching_design.paste("white", (10, 10, 30, 30))
        matching_design.save(self.design)
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review(evidence="COMPARISON.PNG")

        self.run_workflow("finalize", "--output", str(self.output))

        self.assertEqual(self.read_state()["status"], "completed")

    def test_target_legend_must_cover_the_reviewed_path(self) -> None:
        self.prepare()
        self.write_structure([
            {
                "type": "rect",
                "name": "probe",
                "position": {"x": 10, "y": 10},
                "size": {"width": 20, "height": 20},
                "color": "#FFFFFF",
            },
            {
                "type": "rect",
                "name": "other",
                "position": {"x": 36, "y": 36},
                "size": {"width": 16, "height": 16},
                "color": "#808080",
            },
        ])
        self.run_workflow("check", "--output", str(self.output))
        self.run_workflow(
            "target",
            "--output", str(self.output),
            "--element-path", "root/other",
        )
        self.write_review([
            (
                "root/probe",
                "aligned",
                "Incorrectly reused evidence for another element.",
                "target_root_other.png",
            ),
            (
                "root/other",
                "aligned",
                "Current output inspected.",
                "comparison.png",
            ),
        ])

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(any(
            "does not cover review path root/probe" in item
            for item in report["blockers"]
        ))

    def test_finalize_hash_checks_assets_inventory(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        inventory_path = self.output / "assets" / "assets_inventory.json"
        timestamp = inventory_path.stat().st_mtime_ns
        inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
        inventory["tampered_after_check"] = True
        inventory_path.write_text(json.dumps(inventory), encoding="utf-8")
        os.utime(inventory_path, ns=(timestamp, timestamp))

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        self.assertIn("assets_inventory", failed.stderr)
        self.assertIn("changed outside the workflow", failed.stderr)

    def test_render_dependencies_bind_toolchain_and_library_versions(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.write_valid_review()
        state = self.read_state()
        dependencies = state["check"]["render_dependencies"]
        self.assertEqual(dependencies["version"], 2)
        self.assertTrue(dependencies["packages"]["Pillow"])
        self.assertTrue(dependencies["packages"]["NumPy"])
        expected_fonts = []
        for find_font in (annotate_element.find_font, audit_render.find_font):
            font_path = getattr(find_font(12), "path", None)
            if isinstance(font_path, (str, Path)):
                resolved_font = Path(font_path).expanduser().resolve()
                if resolved_font.is_file():
                    expected_fonts.append(resolved_font)
        for font_path in expected_fonts:
            self.assertIn(str(font_path), dependencies["files"])
        workflow_key = next(
            path for path in dependencies["files"]
            if Path(path).name == "workflow.py"
        )

        pillow_version = dependencies["packages"]["Pillow"]
        dependencies["packages"]["Pillow"] = "0.0-review-fixture"
        self.write_state(state)
        failed_version = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed_version.returncode, 0)
        self.assertIn("Render dependency versions changed after check", failed_version.stderr)
        self.assertIn("Pillow", failed_version.stderr)

        state = self.read_state()
        dependencies = state["check"]["render_dependencies"]
        dependencies["packages"]["Pillow"] = pillow_version
        dependencies["files"][workflow_key]["sha256"] = "0" * 64
        self.write_state(state)
        failed_script = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )
        self.assertNotEqual(failed_script.returncode, 0)
        self.assertIn("Render dependency files changed after check", failed_script.stderr)
        self.assertIn("workflow.py", failed_script.stderr)

    def test_memory_backed_pillow_font_is_not_treated_as_a_path(self) -> None:
        class MemoryFont:
            path = BytesIO(b"embedded default font")

        self.assertIsNone(workflow_module.font_dependency_path(MemoryFont()))
        self.assertTrue(workflow_module.runtime_dependency_versions()["Pillow"])

    def test_asset_snapshot_hashes_jpeg_content_not_mtime(self) -> None:
        path = self.assets / "used.jpg"
        path.write_bytes(b"JPEG-CONTENT-A")
        timestamp = path.stat().st_mtime_ns
        before = workflow_module.assets_snapshot(self.assets)
        path.write_bytes(b"JPEG-CONTENT-B")
        os.utime(path, ns=(timestamp, timestamp))

        after = workflow_module.assets_snapshot(self.assets)

        self.assertEqual(before["file_count"], after["file_count"])
        self.assertNotEqual(before["signature"], after["signature"])

    def test_finalize_rejects_structural_warnings_even_when_check_allows_them(
        self,
    ) -> None:
        self.prepare()
        self.write_structure([{
            "type": "image",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "asset": "probe.png",
            "nineSlice": "meta",
        }])
        self.run_workflow(
            "check", "--output", str(self.output), "--allow-warnings"
        )
        self.write_valid_review()

        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(
            any("warning-free check" in item for item in report["blockers"])
        )

    def test_finalize_rejects_tampered_target_evidence(self) -> None:
        self.prepare()
        self.write_structure([{
            "type": "rect",
            "name": "probe",
            "position": {"x": 10, "y": 10},
            "size": {"width": 20, "height": 20},
            "color": "#FFFFFF",
        }])
        self.run_workflow("check", "--output", str(self.output))
        self.run_workflow(
            "target",
            "--output", str(self.output),
            "--element-path", "root/probe",
        )
        target = self.output / "target_root_probe.png"
        self.write_valid_review(evidence=target.name.upper())
        self.run_workflow("finalize", "--output", str(self.output))

        with target.open("ab") as handle:
            handle.write(b"tampered")
        failed = self.run_workflow(
            "finalize", "--output", str(self.output), expect_success=False
        )

        self.assertNotEqual(failed.returncode, 0)
        report = json.loads((self.output / "completion_report.json").read_text())
        self.assertTrue(
            any("targeted evidence is stale" in item for item in report["blockers"])
        )


class AuditWarningGateTests(unittest.TestCase):
    def warning(self, path: str = "root/probe") -> dict:
        return {
            "issues": [{
                "severity": "warning",
                "code": "parent_overflow",
                "path": path,
            }]
        }

    def structure(self, waivers: list[dict]) -> dict:
        return {"metadata": {"auditWaivers": waivers}}

    def test_exact_accepted_waiver_passes(self) -> None:
        waiver = {
            "path": "root/probe",
            "code": "parent_overflow",
            "reason": "The badge intentionally overlaps its parent edge.",
            "accepted": True,
        }

        report, blockers = workflow_module.audit_warning_gate(
            self.structure([waiver]), self.warning()
        )

        self.assertEqual(blockers, [])
        self.assertEqual(report["required"], 1)
        self.assertEqual(report["accepted"], 1)

    def test_missing_waiver_blocks(self) -> None:
        _report, blockers = workflow_module.audit_warning_gate(
            self.structure([]), self.warning()
        )

        self.assertTrue(any("Unaccepted" in item for item in blockers))

    def test_stale_waiver_blocks(self) -> None:
        waiver = {
            "path": "root/old_probe",
            "code": "parent_overflow",
            "reason": "This warning belonged to an older structure revision.",
            "accepted": True,
        }

        _report, blockers = workflow_module.audit_warning_gate(
            self.structure([waiver]), self.warning()
        )

        self.assertTrue(any("Stale" in item for item in blockers))


if __name__ == "__main__":
    unittest.main()
