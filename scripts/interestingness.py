#!/usr/bin/env python3
import argparse
import logging
import os
import subprocess
import shlex
import sys

###############################################################################
# Argument parsing
###############################################################################

parser = argparse.ArgumentParser(
    description = "MF++ interestingness test helper")
parser.add_argument("input", type=str,
    help = "Path to test file to be checked.")
parser.add_argument("--run-timeout", type=int, default=120,
    help = "Maximum time, in seconds, to allow execution of generated test cases.")

###############################################################################
# Helper functions
###############################################################################

def exec_cmd(name, cmd, timeout=None, log_test=False):
    if not timeout:
        log_console.debug(f"Running {name} command:\n\t*** {cmd}")
    else:
        log_console.debug(f"Running {name} command with t/o {timeout}:\n\t*** {cmd}")

    cmd_proc = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, encoding="utf-8")
    try:
        out, err = cmd_proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        cmd_proc.kill()
        out, err = cmd_proc.communicate()
    return cmd_proc.returncode

def make_abs_path(pth, log, check_exists = False):
    if not os.path.isabs(pth):
        log.debug(f"Expanding found relative path `{pth}`.")
        a_pth = os.path.abspath(pth)
        try:
            assert (not check_exists or os.path.exists(pth))
        except AssertionError:
            print(f"Inexistent absolute path `{a_pth}` expanded from relative path `{pth}`.")
        return a_pth
    log.debug(f"Returning found absolute path `{pth}`.")
    return pth

###############################################################################
# Main function
###############################################################################

args = parser.parse_args()

log_console = logging.getLogger('console')
log_console.addHandler(logging.StreamHandler(sys.stdout))
log_console.setLevel(logging.DEBUG)

input_path = make_abs_path(args.input, log_console, True)
compile_script = "/home/sentenced/Documents/Internships/2018_ETH/work/spec_ast/scripts/compile.sh"
cmake_script = "/home/sentenced/Documents/Internships/2018_ETH/work/spec_ast/input/spec_repo/SMT_QF_NIA/z3/runtime"

compile_cmd = f"{compile_script} {input_path} {cmake_script}"
compile_result = exec_cmd("compile", compile_cmd)
input_exec = os.path.splitext(f"{input_path}")[0]
run_cmd = f"{input_exec}"
if not os.path.exists(f"{input_exec}"):
    print(f"Could not find given executable {input_exec}.")
    sys.exit(-1)
run_result = exec_cmd("execute", run_cmd, timeout=args.run_timeout)
print(run_result)
sys.exit(run_result != 0)
