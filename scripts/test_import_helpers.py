"""Balanced source extraction for host tests that compile production methods."""
import re


def block(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:opening + token.end()]
    raise ValueError(marker)
