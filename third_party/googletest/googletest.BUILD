load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])
licenses(["notice"])

GTEST_SRCS = [
    "googletest/src/gtest-assertion-result.cc",
    "googletest/src/gtest-death-test.cc",
    "googletest/src/gtest-filepath.cc",
    "googletest/src/gtest-internal-inl.h",
    "googletest/src/gtest-matchers.cc",
    "googletest/src/gtest-port.cc",
    "googletest/src/gtest-printers.cc",
    "googletest/src/gtest-test-part.cc",
    "googletest/src/gtest-typed-test.cc",
    "googletest/src/gtest.cc",
]

cc_library(
    name = "gtest",
    srcs = GTEST_SRCS,
    hdrs = glob(["googletest/include/**/*.h"]),
    copts = select({
        "@platforms//os:windows": [],
        "//conditions:default": ["-pthread"],
    }),
    includes = [
        "googletest",
        "googletest/include",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = ["googletest/src/gtest_main.cc"],
    hdrs = glob(["googletest/include/**/*.h"]),
    includes = [
        "googletest",
        "googletest/include",
    ],
    deps = [":gtest"],
)
