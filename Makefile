# Orderly Chaos development tasks. Run `make help` for a list.
PYTHON ?= python3

all: test

help:
	@echo "test          run all tests (C++, Python, examples)"
	@echo "test-cpp      run the C++ test suites with Bazel"
	@echo "test-python   run the Python test suite against the Bazel-built library"
	@echo "pytest        run the Python test suite against the installed package"
	@echo "python-lib    build the shared library into python/orderly_chaos/"
	@echo "bench         run the benchmarks (optimized build)"
	@echo "bench-quick   run the benchmarks with a small workload"
	@echo "docs          generate the Doxygen C/C++ reference into build/docs"
	@echo "serve-docs    serve the documentation site at http://localhost:8000"
	@echo "dist          build the source distribution and wheel"
	@echo "docker-build  build the runtime Docker image"
	@echo "docker-test   run the Python suite inside the Docker image"
	@echo "clean         remove build outputs"

test-cpp:
	bazel test //... --test_output=errors

python-lib:
	bazel build //python:shared_lib
	cp -f bazel-bin/python/orderly_chaos/lib_orderly_chaos.* python/orderly_chaos/

test-python: python-lib
	$(PYTHON) -m pytest tests/python -q

pytest:
	$(PYTHON) -m pytest tests/python -q

test: test-cpp test-python

bench:
	bazel run -c opt //benchmarks:benchmark_lob

bench-quick:
	bazel run -c opt //benchmarks:benchmark_lob -- --quick

docs:
	doxygen Doxyfile

serve-docs:
	$(PYTHON) -m http.server --directory docs 8000

clean:
	rm -rf build/ dist/ .eggs/ *.egg-info/ python/orderly_chaos/*.so python/orderly_chaos/*.dylib python/orderly_chaos/*.pyd || true
	find . -name "__pycache__" -type d -prune -exec rm -rf {} + || true
	find . -name "*.py[cod]" -delete || true

dist: clean
	$(PYTHON) -m build

docker-build:
	docker build -t orderly-chaos .

docker-test:
	docker build --target test -t orderly-chaos-test .

ship: test dist
	twine upload dist/*
