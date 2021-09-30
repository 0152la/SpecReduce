#!/usr/bin/env python3

import argparse
import os
import shlex
import subprocess
import yaml

from pathlib import Path

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "MF++ executor wrapper")
parser.add_argument("mfreduce_bin", type=str,
    help = "Path to MF++R binary")
parser.add_argument("input_path", type=str,
    help = "Path to input test file to reduce")
parser.add_argument("config_file_path", type=str,
    help = "Path to config file to use for reduction")
parser.add_argument("--interest-script", type=str,
    help = "Path to interestingness test")
parser.add_argument("--output", type=str,
    help = "Path to output reduced file")

###############################################################################
# Main function
###############################################################################

args = parser.parse_args()

if not Path(args.mfreduce_bin).is_file:
    print(f"Could not find MF++R binary at path {args.mfreduce_bin}")
    exit(1)
if not Path(args.input_path).is_file:
    print(f"Could not find input test file at path {args.input_path}")
    exit(1)
if not Path(args.config_file_path).is_file:
    print(f"Could not find config.yaml at path {args.config_file_path}")
    exit(1)

if args.interest_script is None:
    interest_path = os.path.join(os.path.dirname(args.mfreduce_bin), "..", "scripts", "interestingness.py")
if args.output is None:
    output_path = os.path.join(os.path.dirname(args.input_path), "reduced.cpp")

with open(args.config_file_path, 'r') as config_fd:
    config = yaml.load(config_fd, Loader=yaml.FullLoader)

cmake_path = config["cmake_path"]
compile_commands_path = config["compile_commands_path"]
compile_script = config["compile_script_path"]

cmd = f"{args.mfreduce_bin} --output {output_path} --debug 2 "\
      f"--p {compile_commands_path} --interest-test {interest_path} "\
      f"--cmake-path {cmake_path} --compile-script {compile_script} "\
      f"{args.input_path}"

print("Executing cmd " + cmd)
reduce_proc = subprocess.Popen(shlex.split(cmd), encoding="utf-8")
reduce_proc.communicate()
