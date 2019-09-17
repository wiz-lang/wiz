#!/usr/bin/env python3

import argparse
import os
import re
import subprocess
import sys
import shlex

from collections import namedtuple


BlockTestFile = namedtuple('BlockTestFile', ('filename', 'systems', 'blocks'))
BlockData = namedtuple('BlockData', ('address', 'data'))

def read_test_file(filename):
    _system_regex = re.compile(r'// SYSTEM\s+(.+)$')
    # // BLOCK [[0x]aaaa] [ bb]+ [  comment]
    #  where aaaa = address (from 4 - 8 hex digits, optional)
    #          bb = space separated bytes in hex
    _block_regex =  re.compile(r'// BLOCK(?:\s+(?:0x)?([0-9A-Fa-f]{4,8}))?\s*((?:\s[0-9A-Fa-f]{2})*)(?:\s{2}|$)')

    systems = list()
    blocks = list()

    previous_block = None

    with open(filename) as fp:
        lineno = 0;

        for line in fp:
            lineno += 1

            if '// SYSTEM' in line:
                m = _system_regex.search(line)
                if not m:
                    raise ValueError(f"{filename}:{lineno}: Invalid `// SYSTEM` tag")

                systems.extend(
                    re.split('[,\s]+', m.group(1).strip())
                )

                m = _block_regex.search(line)

            if '// BLOCK' in line:
                m = _block_regex.search(line)
                if not m:
                    raise ValueError(f"{filename}:{lineno}: Invalid `// BLOCK` tag")

                address = m.group(1)
                data = bytearray.fromhex(m.group(2))

                if address:
                    address = int(address, 16)

                    b = next((b for b in blocks if b.address + len(b.data) == address), None)
                    if b:
                        b.data.extend(data)
                    else:
                        b = BlockData(address, data)
                        blocks.append(b)
                    previous_block = b
                else:
                    if previous_block is None:
                        raise ValueError(f"{filename}:{lineno}: First `// BLOCK tag` must contain an address")
                    previous_block.data.extend(data)

    if not systems:
        raise ValueError(f"{filename}: Expected at least one `// SYSTEM` tag")

    if not blocks:
        raise ValueError(f"{filename}: Expected at least one `// BLOCK` tag")

    return BlockTestFile(filename, systems, blocks)



WIZ_EXECUTABLE = None
WIZ_OUTPUT_DIR = None
MISMATCHES_SHOWN_PER_BLOCK = 6

tests_passed = 0
tests_failed = 0

def do_test(test, system):
    global tests_failed, tests_passed

    print(f"{test.filename} {system}:", end='')

    bin_fn = os.path.join(WIZ_OUTPUT_DIR, os.path.splitext(os.path.basename(test.filename))[0] + '.' + system + '.bin')

    process = subprocess.Popen(
        (WIZ_EXECUTABLE, "--system", system, "-o", bin_fn, test.filename),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )
    pout, perr = process.communicate()


    errors = list()


    if process.returncode != 0:
        pout = pout.decode('utf-8')
        perr = perr.decode('utf-8')

        # ::TODO failure tests::

        errors.append(f"wiz returned failure code {process.returncode}")
        errors.append('> ' + ' '.join(shlex.quote(x) for x in process.args))
        if pout:
            errors.append(pout)
        if perr:
            errors.append(perr)

    else:
        # process.returncode == 0

        if test.blocks:
            with open(bin_fn, 'br') as fp:
                output_binary = fp.read()

            last_byte = max([b.address + len(b.data) for b in test.blocks])

            if len(output_binary) < last_byte:
                errors.append(f"{bin_fn}: expected at least {last_byte} bytes in output file")
            else:
                for block in test.blocks:
                    if not output_binary.startswith(block.data, block.address):
                        mismatches = 0
                        for i, expected_byte in enumerate(block.data):
                            addr = block.address + i
                            got_byte = output_binary[addr]
                            if got_byte != expected_byte:
                                if mismatches < MISMATCHES_SHOWN_PER_BLOCK:
                                    errors.append(f"{bin_fn} 0x{addr:06x}: expected 0x{expected_byte:02x} got 0x{got_byte:02x}")
                                mismatches += 1
                        if mismatches > MISMATCHES_SHOWN_PER_BLOCK:
                            errors.append(f"+ {mismatches - MISMATCHES_SHOWN_PER_BLOCK} more incorrect bytes")

    if not errors:
        tests_passed += 1
        print(" PASSED")
    else:
        tests_failed += 1
        print(" FAILED")
        for e in errors:
            print('\t', e.replace('\n', '\n\t'), sep='')
        print()



def do_test_files(test_files):
    # Read all test files first
    # Error out on malformed tests before showing the PASSED/FAILED text
    tests = list()
    for test_file in test_files:
        tests.append(read_test_file(test_file))

    for test in tests:
        for system in test.systems:
            do_test(test, system)



def list_of_test_files(block_tests_args):
    test_files = list()

    for arg in block_tests_args:
        if os.path.isdir(arg):
            for root, dirs, files in os.walk(arg, topdown=True):
                dirs.sort()
                files.sort()
                for f in files:
                    if f.endswith('.wiz') and not f.startswith('_'):
                        test_files.append(os.path.join(root, f))
        else:
            test_files.append(arg)

    return test_files



def bin_dir_argument_test(d):
    if not os.path.isdir(d):
        raise argparse.ArgumentTypeError(f"{d} is not a directory")
    elif not os.access(d, os.W_OK):
        raise argparse.ArgumentTypeError(f"{d} is not writable")
    else:
        return d


def read_program_arguments():
    global WIZ_EXECUTABLE, WIZ_OUTPUT_DIR, MISMATCHES_SHOWN_PER_BLOCK

    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--wiz', required=True,
                        help='location of wiz executable')
    parser.add_argument('-b', '--bin-dir', required=True, type=bin_dir_argument_test,
                        help='location to store the output binaries')
    parser.add_argument('-a', '--all-mismatches', action='store_true',
                        help='show all mismatches in a block test')
    parser.add_argument('tests', nargs='+',
                        help='files/directories to test\n'
                             '(NOTE: if a directory is given, filenames starting with an underscore are skipped)')

    args = parser.parse_args()

    WIZ_EXECUTABLE = args.wiz
    WIZ_OUTPUT_DIR = args.bin_dir

    if args.all_mismatches:
        MISMATCHES_SHOWN_PER_BLOCK = 0x1000000

    return args.tests



def main():
    global tests_failed, tests_passed

    test_files = read_program_arguments();
    test_files = list_of_test_files(test_files)
    do_test_files(test_files)

    print(f"{tests_passed} tests passed", file=sys.stderr)

    if tests_failed:
        print(f"{tests_failed} TESTS FAILED", file=sys.stderr)
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main();

