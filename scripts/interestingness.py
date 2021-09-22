#!/usr/bin/env python3
import argparse
import logging
import os
import subprocess
import shlex
import signal
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
parser.add_argument("--logging", type=str, choices=['none', 'debug', 'info'],
    default='info',
    help = "Level of data to be logged.")
parser.add_argument("--compile-script-path", type=str,
    help = "Path to compilation script.")
parser.add_argument("--cmake-path", type=str,
    help = "Path to cmake script for compilation script.")

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
        log_console.debug("Timeout")
        cmd_proc.kill()
        out, err = cmd_proc.communicate()
    log_console.debug(f"Return code: {cmd_proc.returncode}")
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

signal.signal(signal.SIGPIPE, signal.SIG_DFL)

args = parser.parse_args()

log_console = logging.getLogger('console')
if not args.logging == 'none':
    log_console.addHandler(logging.StreamHandler(sys.stdout))
if args.logging == 'debug':
    log_console.setLevel(logging.DEBUG)
elif args.logging == 'info':
    log_console.setLevel(logging.INFO)

input_path = make_abs_path(args.input, log_console, True)
compile_script = make_abs_path(args.compile_script_path, log_console, True)
cmake_script = make_abs_path(args.cmake_path, log_console, True)

compile_cmd = f"{compile_script} {input_path} {cmake_script}"
compile_result = exec_cmd("compile", compile_cmd)
input_exec = os.path.splitext(f"{input_path}")[0]
run_cmd = f"{input_exec}"
if not os.path.exists(f"{input_exec}"):
    print(f"Could not find given executable {input_exec}.")
    sys.exit(-1)
run_result = exec_cmd("execute", run_cmd, timeout=args.run_timeout)
os.remove(input_exec)
sys.exit(run_result)
