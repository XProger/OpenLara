"""Regression checks for the multi-view renderer comparison harness."""

import contextlib
import io
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import viewcheck


class ViewCheckTests(unittest.TestCase):
    def test_fx3_cycles_and_frame_use_the_fx3_format_and_opening_camera(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = str(Path(tmp) / "frame.bin")

            def render(args, **kwargs):
                self.assertIn("--fx3", args)
                self.assertNotIn("--mode7", args)
                self.assertEqual(args[args.index("--cam-ty") + 1], "2491")
                self.assertEqual(args[args.index("--probe-x") + 1], "9728")
                Path(raw).write_bytes(b"8bpp pixels including shade")
                # FX3 has no separate tile shade map.
                return subprocess.CompletedProcess(args, 0,
                    "STOP after 100 instructions, 10 pixels plotted\n"
                    "  500 reference GSU cycles; FX3 hardware timing is not modelled", "")

            with patch.object(viewcheck.subprocess, "run", side_effect=render):
                result = viewcheck.model("fx3.sfc", viewcheck.VIEWS[0], raw, "palette", False, True)
            self.assertEqual(result[:2], (500, 10))

    def test_palette_only_change_is_visible(self):
        with tempfile.TemporaryDirectory() as tmp:
            raw = str(Path(tmp) / "frame.bin")

            def render(args, **kwargs):
                Path(raw).write_bytes(b"same pixels")
                shade = args[args.index("--raw-shade") + 1]
                Path(shade).write_bytes(b"light" if args[2] == "before" else b"dark")
                return subprocess.CompletedProcess(args, 0,
                    "STOP after 100 instructions, 10 pixels plotted\n  500 cycles", "")

            with patch.object(viewcheck.subprocess, "run", side_effect=render):
                before = viewcheck.model("before", viewcheck.VIEWS[0], raw, "palette", False)
                after = viewcheck.model("after", viewcheck.VIEWS[0], raw, "palette", False)
            self.assertEqual(before[:2], after[:2])
            self.assertNotEqual(before[2], after[2])

    def test_simulator_failure_is_not_accepted_as_a_frame(self):
        result = subprocess.CompletedProcess([], 1,
            "STOP after 100 instructions, 10 pixels plotted\n  500 cycles", "write failed")
        with patch.object(viewcheck.subprocess, "run", return_value=result):
            with self.assertRaisesRegex(SystemExit, "write failed"):
                viewcheck.model("bad", viewcheck.VIEWS[0], "unused", "palette", False)

    def test_exit_status_reports_changed_views(self):
        for after, expected in [("same", 0), ("different", 1)]:
            with self.subTest(after=after), \
                 patch.object(viewcheck.sys, "argv", ["viewcheck", "before", "after"]), \
                 patch.object(viewcheck, "VIEWS", viewcheck.VIEWS[:1]), \
                 patch.object(viewcheck, "model", side_effect=[(500, 10, "same"),
                                                               (450, 10, after)]), \
                 contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(viewcheck.main(), expected)


if __name__ == "__main__":
    unittest.main()
