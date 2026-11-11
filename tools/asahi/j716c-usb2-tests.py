#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile the actual PHY sequence and OF admission header without hardware."""
import argparse
from pathlib import Path
import subprocess
import tempfile
args = argparse.ArgumentParser()
args.add_argument("--dtb")
args.add_argument("--expect-disabled", action="store_true")
args = args.parse_args()
root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as d:
    directory = Path(d)
    for name in ('fixture', 'policy') + (('fdt',) if args.dtb else ()):
        source = root / ('tools/asahi/j716c-usb2/' + name + '.c')
        if name in ('policy', 'fdt'):
            header = (root / 'include/linux/soc/apple/j716c-usb2.h').read_text()
            header = '\n'.join(s for s in header.splitlines() if not s.startswith('#include'))
            text = source.read_text().replace('/* ACTUAL_HEADER */', header)
            source = directory / (name + '.c')
            source.write_text(text)
        binary = str(directory / name)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(source), '-o', binary] + (['-lfdt', '-Wno-misleading-indentation'] if name == 'fdt' else []), check=True)
        subprocess.run([binary] + ([args.dtb, '0' if args.expect_disabled else '1'] if name == 'fdt' else []), check=True)
