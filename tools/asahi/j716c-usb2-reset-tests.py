#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile actual reset registration and provider initializer; no hardware."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--driver', type=Path)
parser.add_argument('--expect-refusal', action='store_true')
args = parser.parse_args()
driver = (args.driver or root / 'drivers/phy/apple/j716c-usb2-device.c').read_text()
core = (root / 'drivers/reset/core.c').read_text()
def function(text, name):
    found = re.search(r'(?:static )?int\s+' + name + r'\([^;{]+\)\s*\{', text)
    if found is None:
        return ''
    start, pos = found.start(), found.end()
    depth = 1
    while depth:
        depth += (text[pos] == '{') - (text[pos] == '}')
        pos += 1
    return text[start:pos]
start = driver.index('p->rcdev = (struct reset_controller_dev)')
end = driver.index('\n\tret = devm_reset_controller_register', start)
initializer = driver[start:end]
prelude = (root / 'tools/asahi/j716c-usb2-reset/fixture.c').read_text()
source = prelude.replace('/* ACTUAL_FUNCTIONS */', '\n'.join([
    function(core, 'fwnode_reset_simple_xlate'),
    function(core, 'reset_controller_register'),
    function(driver, 'j716c_usb2_reset_xlate'),
])).replace('/* ACTUAL_INITIALIZER */', initializer)
with tempfile.TemporaryDirectory() as d:
    c = Path(d) / 'fixture.c'
    c.write_text(source)
    binary = str(Path(d) / 'fixture')
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    str(c), '-o', binary], check=True)
    run = subprocess.run([binary], capture_output=True, text=True)
    if args.expect_refusal:
        assert run.returncode == -6 and 'Assertion' in run.stderr, run
        print('PASS: frozen one-cell default fails the zero-cell binding contract (negative control)')
    else:
        assert run.returncode == 0, run.stderr
        print(run.stdout, end='')
