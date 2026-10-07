"""Run every demo project with small workloads so they cannot rot."""

import os
import subprocess
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEMOS = os.path.join(ROOT, "examples")


@unittest.skipUnless(os.path.isdir(DEMOS), "demos are not part of installed packages")
class DemosTest(unittest.TestCase):
    def run_demo(self, directory, script, *args):
        path = os.path.join(DEMOS, directory)
        env = dict(os.environ)
        # The CI job sets a relative PYTHONPATH=python, which would resolve
        # against cwd below instead of the repo root. Always pass the
        # absolute package directory so demos import the library under test.
        package_dir = os.path.join(ROOT, "python")
        if env.get("PYTHONPATH"):
            env["PYTHONPATH"] = package_dir + os.pathsep + env["PYTHONPATH"]
        else:
            env["PYTHONPATH"] = package_dir
        result = subprocess.run(
            [sys.executable, os.path.join(path, script), *args],
            capture_output=True,
            text=True,
            timeout=180,
            cwd=path,
            env=env,
        )
        self.assertEqual(0, result.returncode, result.stderr)
        return result.stdout

    def test_sma_backtest(self):
        output = self.run_demo("sma-backtest", "backtest.py", "--steps", "200", "--seed", "7")
        self.assertIn("strategy P&L", output)
        self.assertIn("buy-and-hold P&L", output)

    def test_twap_executor(self):
        output = self.run_demo(
            "twap-executor", "twap.py", "--quantity", "1000", "--slices", "3", "--seed", "3"
        )
        self.assertIn("slippage vs arrival", output)
        self.assertIn("slice  filled  avg price", output)

    def test_book_ladder(self):
        output = self.run_demo(
            "book-ladder", "ladder.py", "--snapshots", "2", "--events", "200", "--seed", "11"
        )
        self.assertIn("=== snapshot 1", output)
        self.assertIn("spread", output)


if __name__ == "__main__":
    unittest.main()
