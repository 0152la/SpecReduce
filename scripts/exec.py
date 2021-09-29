#!/usr/bin/env python3

import argparse
import os
import yaml

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(description = "MF++ executor wrapper")
parser.add_argument("mfreduce-bin", type=str,
    help = "Path to MF++R binary")
parser.add_argument("input-path", type=str,
    help = "Path to input test file to reduce")
parser.add_argument("config-file-path", type=str,
    help = "Path to config file to use for reduction")
parser.add_argument("--interest-script", type=str,
    help = "Path to interestingness test")
parser.add_argument("--output", type=str,
    help = "Path to output reduced file")

###############################################################################
# Main function
###############################################################################

args = parser.parse_args()
sep = os.path.sep()

if args.interest_script is None:
    interest_path = os.path.join(os.path.dirname(args.mfreduce_bin), "..", "scripts", "interestingness.py")
if args.output is None:
    output_path = os.path.join(os.path.dirname(args.input-path), "reduced.cpp")

with open(args.config_file_path, 'r') as config_fd:
    config = yaml.load(config_fd, Loader=yaml.FullLoader)

cmake_path = config["cmake_path"]
compile_commands_path = config["compile_commands_path"]
compile_script = config["compile_script_path"]

cmd = f"{args.mfreduce_bin} --output {output_path} --debug 2"
      f"--p {compile_commands_path} --interest-test {interest_path}"
      f"--cmake-path {cmake_path} --compile-script {compile_script}"

print("Executing cmd " + cmd)
reduce_proc = subprocess.Popen(shlex.split(cmd), encoding="utf-8")
reduce_proc.communicate()
