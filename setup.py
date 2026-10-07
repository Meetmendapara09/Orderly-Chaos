"""Build the native library for the orderly-chaos Python package.

Project metadata lives in pyproject.toml. This file only declares the C++
shared library (the C API in src/c_api.cpp) that the package loads with
ctypes. It is compiled as an extension module so that pip builds it
automatically for the target platform.
"""

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

# compiler flags per compiler family
COMPILE_ARGS = {
    "msvc": ["/std:c++17", "/O2", "/EHsc"],
    "unix": ["-std=c++17", "-O2", "-fvisibility=hidden", "-fvisibility-inlines-hidden"],
    "mingw32": ["-std=c++17", "-O2"],
}
LINK_ARGS = {
    "mingw32": ["-static-libgcc", "-static-libstdc++"],
}


class BuildExt(build_ext):
    """Apply compiler-specific flags."""

    def build_extensions(self):
        compiler = self.compiler.compiler_type
        for extension in self.extensions:
            extension.extra_compile_args = COMPILE_ARGS.get(compiler, COMPILE_ARGS["unix"])
            extension.extra_link_args = LINK_ARGS.get(compiler, [])
        super().build_extensions()


LIBRARY = Extension(
    name="orderly_chaos.lib_orderly_chaos",
    sources=["src/c_api.cpp"],
    include_dirs=["include", "third_party/robin_map/include"],
    define_macros=[
        ("ORDERLY_CHAOS_BUILDING_LIBRARY", "1"),
        ("ORDERLY_CHAOS_PYTHON_EXTENSION", "1"),
    ],
    language="c++",
)

setup(ext_modules=[LIBRARY], cmdclass={"build_ext": BuildExt})
