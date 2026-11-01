#!/usr/bin/env python3
"""Compiler selection for portable host-only tests; never emulates a macOS SDK."""
import os
import shlex
import shutil
import sys

def compiler(language="c++"):
    if language not in {"c", "c++"}:
        raise ValueError("language must be c or c++")
    variable = "CC" if language == "c" else "CXX"
    clang = "clang" if language == "c" else "clang++"
    fallback = "cc" if language == "c" else "c++"
    explicit = os.environ.get(variable)
    if explicit:
        command = shlex.split(explicit)
        if not command:
            raise RuntimeError(variable + " must name a compiler")
        return command
    if sys.platform == "darwin" and shutil.which("xcrun"):
        return ["xcrun", clang]
    command = shutil.which(clang) or shutil.which(fallback)
    if not command:
        raise RuntimeError("A " + language + " compiler is required")
    return [command]

def sanitizer_flags():
    selection = os.environ.get("RTW_HOST_SANITIZERS", "address,undefined")
    if selection not in {"address,undefined", "undefined"}:
        raise RuntimeError("RTW_HOST_SANITIZERS must be address,undefined or undefined")
    return ["-fsanitize=" + selection, "-fno-sanitize-recover=all"]

def dead_strip_flags():
    if sys.platform == "darwin":
        return ["-Wl,-dead_strip"]
    return ["-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections"]

if __name__ == "__main__":
    print(" ".join(compiler() + sanitizer_flags()))
