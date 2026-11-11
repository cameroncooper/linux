#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Compile public Aura PMGR accessors over actual ADT rows and compiled DT."""
import argparse
from pathlib import Path
import re,subprocess,sys
parser=argparse.ArgumentParser()
parser.add_argument('--adt',type=Path,required=True)
parser.add_argument('--dtb',type=Path,required=True)
parser.add_argument('--aura-source',type=Path,required=True)
parser.add_argument('--out',type=Path,required=True)
parser.add_argument('--active',action='store_true')
parser.add_argument('--expect-refusal',action='store_true')
a=parser.parse_args();a.out.mkdir(parents=True,exist_ok=True)
sys.path.insert(0,str(a.aura_source/'proxyclient'))
from aura import adt
raw=adt.ADTNodeStruct.parse(a.adt.read_bytes())
def child(n,name):return next(c for c in n.children if next(p.value.rstrip(b'\0').decode() for p in c.properties if p.name=='name')==name)
pmgr=child(child(raw,'arm-io'),'pmgr')
blob=next(p.value for p in pmgr.properties if p.name=='devices')
binary=a.out/'actual-pmgr-devices.bin';binary.write_bytes(blob);binary.chmod(0o600)
source=(a.aura_source/'src/pmgr.c').read_text()
def block(start):
 pos=source.index('{',start)+1;depth=1
 while depth:depth+=(source[pos]=='{')-(source[pos]=='}');pos+=1
 return source[start:pos]
struct=block(source.index('struct pmgr_device {'))+' PACKED;'
functions=[]
for name in ('pmgr_adt_get_id','pmgr_adt_get_parents'):
 m=re.search(r'static (?:u16|void) '+name+r'\(',source);functions.append(block(m.start()))
root=Path(__file__).resolve().parents[2]
text=(root/'tools/asahi/j716c-usb2-closure/adt-dt.c').read_text().replace('/* ACTUAL_PMGR */',struct+'\n'+'\n'.join(functions))
c=a.out/'fixture.c';c.write_text(text);exe=a.out/'fixture'
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-Wno-misleading-indentation',str(c),'-lfdt','-o',str(exe)],check=True)
r=subprocess.run([str(exe),str(binary),str(a.dtb),'1' if a.active else '0'],text=True,capture_output=True)
if a.expect_refusal:
 assert r.returncode==-6 and 'Assertion' in r.stderr,r
 print('PASS: old/unflattened DT closure rejected by actual ADT/source-DT fixture')
else:
 assert r.returncode==0,r.stderr
 print(r.stdout,end='')
