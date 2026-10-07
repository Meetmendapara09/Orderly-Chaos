"""Run every Python example so the documentation examples cannot rot."""

import os
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXAMPLES = os.path.join(ROOT, "examples", "python")


@unittest.skipUnless(os.path.isdir(EXAMPLES), "examples are not part of installed packages")
class ExamplesTest(unittest.TestCase):
    def run_example(self, name, *args):
        result = subprocess.run(
            [sys.executable, os.path.join(EXAMPLES, name), *args],
            capture_output=True,
            text=True,
            timeout=120,
        )
        self.assertEqual(0, result.returncode, result.stderr)
        return result.stdout

    def test_quickstart(self):
        output = self.run_example("quickstart.py")
        self.assertIn("order 4 filled 100, resting 150", output)
        self.assertIn("market buy filled 200 of 500", output)
        self.assertIn("rejected: cancel order 999: unknown order id", output)

    def test_replay_events(self):
        output = self.run_example("replay_events.py")
        self.assertIn("REJECTED", output)  # the duplicate cancel in the sample
        self.assertIn("trades, volume", output)

    def test_replay_events_from_file(self):
        with tempfile.TemporaryDirectory() as directory:
            path = os.path.join(directory, "events.csv")
            with open(path, "w") as handle:
                handle.write("action,order_id,side,quantity,price\nlimit,1,buy,10,100\nlimit,2,sell,4,99\n")
            output = self.run_example("replay_events.py", path)
        self.assertIn("1 trades, volume 4, VWAP 100.00", output)

    def test_market_maker(self):
        output = self.run_example("market_maker.py", "500")
        self.assertIn("P&L (ticks)", output)


if __name__ == "__main__":
    unittest.main()
