# an alias to the python command
PYTHON=python3

# build the library, test the C++ and Python interfaces
all: test

#
# MARK: Development
#

# test the C++ code (Bazel + Google Test)
test_cpp:
	bazel test //test:all

# build the shared library consumed by the Python package
lib_orderly_chaos:
	bazel build //orderly_chaos:copy_dll

# run the Python test suite
test: test_cpp lib_orderly_chaos
	cp bazel-bin/orderly_chaos/lib_orderly_chaos.dll orderly_chaos/
	${PYTHON} -m unittest discover .

#
# MARK: Deployment
#

clean_dist:
	rm -rf build/ dist/ .eggs/ *.egg-info/ || true

clean_python_build:
	find . -name "*.pyc" -delete
	find . -name "__pycache__" -delete

clean_cpp_build:
	bazel clean

# clean the build directory
clean: clean_dist clean_python_build clean_cpp_build

# build the deployment package
deployment: clean
	${PYTHON} setup.py sdist #bdist_wheel

# ship the deployment package to PyPi
ship: test deployment
	twine upload dist/*
