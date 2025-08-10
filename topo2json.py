#!/usr/bin/env python3

from collections import defaultdict
import os.path
import string
import argparse
import json

def parse_topo(f):
    topo = defaultdict(list)
    section = ""

    line_num = 0

    for line in f.readlines():
        line_num += 1

        # remove trailing whitespace
        line = line.rstrip()

        # ignore empty lines
        if not line:
            continue

        # ignore comments
        if line.startswith('#'):
            continue

        # if label
        labels = [ "router", "link" ]
        if line in labels:
            section = line
            continue

        if not section:
            raise ValueError(f"line {line_num}: unexpected line in empty section")

        fields = []
        if section == "router":
            num_req_fields = 1
            fields = ["node", "comment", "y", "x", "mpi-partition"]
        elif section == "link":
            num_req_fields = 2
            fields = ["from", "to", "capacity", "metric", "delay", "queue"]

        values = line.split()
        if len(values) > len(fields):
            raise ValueError(f"line {line_num}: too many fields, maximum {len(fields)}, got {len(values)}")
        elif len(values) < num_req_fields:
            raise ValueError(f"line {line_num}: not enough fields, minimum {num_req_fields}, got {len(values)}")

        # TODO: validate contents of fields againt field-specific grammar
        topo[section].append({field: value for field, value in zip(fields, values)})

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
