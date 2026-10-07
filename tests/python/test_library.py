"""Tests for loading the native library and package metadata."""

import os
import re
import subprocess
import sys
import threading
import unittest

import orderly_chaos
from orderly_chaos import LimitOrderBook, Side, UnknownOrderIdError, _library

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class VersionTest(unittest.TestCase):
    def test_native_and_python_versions_match(self):
        self.assertEqual(orderly_chaos.__version__, orderly_chaos.native_version())

    def test_versions_are_consistent_across_the_repository(self):
        version = orderly_chaos.__version__
        with open(os.path.join(ROOT, "include", "orderly_chaos", "version.hpp")) as header:
            self.assertIn(f'#define ORDERLY_CHAOS_VERSION_STRING "{version}"', header.read())
        pyproject = os.path.join(ROOT, "pyproject.toml")
        if os.path.exists(pyproject):  # not present in installed wheels
            with open(pyproject) as config:
                self.assertRegex(config.read(), rf'(?m)^version = "{re.escape(version)}"$')
        module = os.path.join(ROOT, "MODULE.bazel")
        if os.path.exists(module):
            with open(module) as config:
                self.assertIn(f'version = "{version}"', config.read())

    def test_author(self):
        self.assertEqual("Meet Mendapara", orderly_chaos.__author__)


class LoadingTest(unittest.TestCase):
    def run_python(self, code, **env):
        environment = dict(os.environ, **env)
        return subprocess.run(
            [sys.executable, "-c", code], capture_output=True, text=True, env=environment, timeout=60
        )

    def test_candidate_paths_find_the_library(self):
        paths = _library.candidate_paths()
        self.assertTrue(paths)
        self.assertTrue(all(os.path.exists(p) for p in paths))

    def test_missing_library_gives_a_helpful_error(self):
        result = self.run_python(
            "import orderly_chaos\n"
            "try:\n"
            "    orderly_chaos.LimitOrderBook()\n"
            "except orderly_chaos.LibraryError as e:\n"
            "    print('LibraryError:', e)\n",
            ORDERLY_CHAOS_LIBRARY=os.path.join(ROOT, "does-not-exist.so"),
        )
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("LibraryError: failed to load", result.stdout)

    def test_import_does_not_require_the_library(self):
        result = self.run_python(
            "import orderly_chaos; print(orderly_chaos.__version__)",
            ORDERLY_CHAOS_LIBRARY=os.path.join(ROOT, "does-not-exist.so"),
        )
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual(orderly_chaos.__version__, result.stdout.strip())

    def test_environment_override(self):
        path = _library.candidate_paths()[0]
        result = self.run_python(
            "from orderly_chaos import LimitOrderBook; b = LimitOrderBook(); b.limit_buy(1, 5, 9); print(b.volume())",
            ORDERLY_CHAOS_LIBRARY=path,
        )
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertEqual("5", result.stdout.strip())


class ThreadingTest(unittest.TestCase):
    def test_concurrent_use_of_one_book_is_safe(self):
        book = LimitOrderBook()
        errors = []

        def worker(offset):
            try:
                for i in range(2000):
                    order_id = offset * 100_000 + i
                    book.limit(Side.BUY if i % 2 else Side.SELL, order_id, 1, 1000 + (i % 7) - 3)
                    if i % 3 == 0:
                        # has() + cancel() is not atomic: another thread may
                        # fill the order in between, so handle that case
                        try:
                            book.cancel(order_id)
                        except UnknownOrderIdError:
                            pass
            except Exception as error:  # pragma: no cover - reported below
                errors.append(error)

        threads = [threading.Thread(target=worker, args=(n,)) for n in range(8)]
        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()
        self.assertEqual([], errors)
        bid, ask = book.best_buy(), book.best_sell()
        if bid is not None and ask is not None:
            self.assertLess(bid, ask)  # never crossed


if __name__ == "__main__":
    unittest.main()
