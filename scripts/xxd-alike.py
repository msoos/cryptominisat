#!/usr/bin/env python

import sys

PY3 = sys.version_info.major == 3

input_name = sys.argv[1]
output_path = sys.argv[2]
output_name = input_name.replace('.', '_')

# In python 3, opening file as rb will return bytes and iteration is per byte
# In python 2, opening file as rb will return string and iteration is per char
# and char need to be converted to bytes.
# This function papers over the differences
def convert(c):
    if PY3:
        return c
    return ord(c)

with open(input_name, 'rb') as file:
    contents = file.read()
with open(output_path, 'w') as out:
    out.write('unsigned char {}[] = {{'.format(output_name))
    first = True
    for i, byte in enumerate(contents):
        if not first:
            out.write(', ')
        first = False
        if i % 12 == 0:
            out.write('\n  ')
        out.write('0x{:02x}'.format(convert(byte)))

    out.write(', ')
    out.write('0x00')

    out.write('\n};\n')
    out.write('unsigned int {}_len = {};\n'.format(output_name, len(contents)+1))
