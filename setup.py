"""The setup script for installing and distributing the orderly-chaos package."""
from glob import glob
import os
from setuptools import setup, find_packages, Extension


# set the compiler for the C++ framework
os.environ['CC'] = 'g++'
os.environ['CCX'] = 'g++'


# read the contents from the README file
with open('README.md') as README_file:
    README = README_file.read()


# The prefix name for the .so library to build. It will follow the format
# lib_lob.*.so where the * changes depending on the build system
LIB_NAME = 'orderly_chaos.lib_orderly_chaos'
# The source files for building the extension. Globs locate all the cpp files
# used by the LaiNES subproject. MANIFEST.in has to include the blanket
# "cpp" directory to ensure that the .inc file gets included too
SOURCES = glob('src/*.cpp')
# The directory pointing to header files used by the LaiNES cpp files.
# This directory has to be included using MANIFEST.in too to include the
# headers with sdist
INCLUDE_DIRS = [
    'include',
    'vendor/binary-search-tree/include',
    'vendor/cpptqdm/include',
    'vendor/doubly-linked-list/include',
    'vendor/hopscotch_map/include',
    'vendor/robin-map/include',
    'vendor/sparse-map/include',
]
# Build arguments to pass to the compiler
EXTRA_COMPILE_ARGS = ['-std=c++1y', '-march=native', '-pipe', '-O3']
# The official extension using the name, source, headers, and build args
LIBRARY = Extension(LIB_NAME,
    sources=SOURCES,
    include_dirs=INCLUDE_DIRS,
    extra_compile_args=EXTRA_COMPILE_ARGS,
)


setup(
    name='orderly-chaos',
    version='2.0.0',
    description='Orderly Chaos: A Non-Consensual Matching Engine for Unruly Capital',
    long_description=README,
    long_description_content_type='text/markdown',
    keywords='Limit Order Book Matching Engine Orderly Chaos',
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Financial and Insurance Industry',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Operating System :: MacOS :: MacOS X',
        'Operating System :: POSIX :: Linux',
        'Operating System :: Microsoft :: Windows',
        'Programming Language :: C++',
        'Programming Language :: Python :: 3 :: Only',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Topic :: Office/Business :: Financial',
        'Topic :: Office/Business :: Financial :: Investment'
    ],
    packages=find_packages(exclude=['tests', '*.tests', '*.tests.*']),
    ext_modules=[LIBRARY],
    zip_safe=False,
)
