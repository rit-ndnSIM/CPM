#!/usr/bin/env python3

from collections import defaultdict
import os.path
import string
import argparse
import json

def parse_topo(f):
    topo = defaultdict(list)
    section = ""

    for line in f.readlines():
        # remove trailing whitespace
        line = line.rstrip()

        # ignore empty lines
        if not line:
            continue

        # ignore comments
        if line.startswith('#'):
            continue

        # if label
        if all(c in string.ascii_letters for c in line):
            section = line
            continue

        match section:
            case "router" | "link":
                fields = []
                if section == "router":
                    fields = ["node", "city", "y", "x", "mpi-partition"]
                elif section == "link":
                    fields = ["from", "to", "capacity", "metric", "delay", "queue"]

                values = line.split()
                if len(values) != len(fields):
                    raise ValueError(f"Expected {fields.len()} fields for section '{section}', got {values.len()}")

                topo[section].append({field: value for field, value in zip(fields, values)})
            case "":
                raise ValueError(f"Unexpected line '{line}' in empty section")
            case _:
                raise ValueError(f"Unknown section '{section}'")

    return topo


def main():
    parser = argparse.ArgumentParser("topo2json")
    parser.add_argument("file", nargs='+', help="Files to convert")
    args = parser.parse_args()

    for filename in args.file:
        json_name = os.path.splitext(filename)[0] + ".json"
        with open(filename, "r") as topo_file:
            with open(json_name, "w") as json_file:
                topo = parse_topo(topo_file)
                json_file.write(json.dumps(topo))


if __name__ == '__main__':
    main()
