#!/usr/bin/env python

from __future__ import with_statement
import base64, zlib, re

def compress_file(filename):
    with open(filename) as f:
        contents = f.read()

    bin = zlib.compress(contents)
    return ('"%s" : r"""' % filename) + base64.b64encode(bin) + '"""'

def decompress_file(content):
    return zlib.decompress(base64.b64decode(content))

def build_table(filenames):
    table = "\n\nCLAY_FILES = {\n"
    table += ",\n".join(compress_file(f) for f in filenames)
    table += "\n}"
    return table

CLAY_FOOTER = """
if __name__ == '__main__':
    main()
"""

if __name__ == '__main__':
    clay_table = build_table(['clay.c', 'clay_sandbox.c', 'clay_fixtures.c', 'clay_fs.c', 'clay.h'])

    with open('_clay.py') as f:
        clay_source = f.read()

    with open('clay.py', 'w') as f:
        f.write(clay_source)
        f.write(clay_table)
        f.write(CLAY_FOOTER)
