# This is a script to help convert the decompiled VS source code back into
# protobufs. This script ended up being unnecessary for version 1, because
# Tyron contributed the real protobuf file. The script is kept around in case
# it helps in updating the Protobuf files on the next VS version update.

import re
import sys

class_def = re.compile(r"\s*(public\s+)?class\s+Packet_(\S+)\s*(:.*)?$")
field_def = re.compile(r"\s*(public\s+)?(Packet_)?([^=,]+)\s+(\S+);$")
field_id_regex = re.compile(r"\s*(public\s+)?const int\s+(\S+)FieldID = (\S+);$")

current_class = None
fields = []
field_ids = {}

def translate_type(field_type):
    if field_type == "byte[]":
        return "bytes"

    if field_type.endswith("[]"):
        return 'repeated ' + translate_type(field_type[:-2])

    if field_type == "uint":
        return "uint32"
    if field_type == "ulong":
        return "uint64"
    if field_type == "int":
        return "int32"
    if field_type == "long":
        return "int64"
    return field_type

def flush_class():
    if current_class is None:
        return
    print('')
    print('message %s {' % current_class)
    for field_name, field_type in fields:
        if field_name not in field_ids:
            if field_name.endswith("Count"):
                continue
            if field_name.endswith("Length"):
                continue
            print('Missing key for field %s of type %s' % (field_name, field_type))
        print('  %s %s = %s;' % (translate_type(field_type), field_name, field_ids[field_name]))
    print('}')
    fields.clear()
    field_ids.clear()

print('syntax = "proto3";')
print('package Packet;')

for line in map(str.rstrip, sys.stdin):
    if match := re.fullmatch(class_def, line):
        flush_class()
        class_name = match.group(2)
        current_class = class_name
    elif match := re.fullmatch(field_def, line):
        field_type = match.group(3)
        field_name = match.group(4)
        if field_type == "return":
            continue
        fields.append((field_name, field_type))
    elif match := re.fullmatch(field_id_regex, line):
        field_name = match.group(2)
        field_id = match.group(3)
        field_ids[field_name] = field_id
