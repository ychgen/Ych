import json
import sys
from pathlib import Path

class GeneratedItem:
    def __init__(self, inc: str, inasm: str):
        self.inc = inc
        self.inasm = inasm

class ToGenerateItem:
    def __init__(self, name: str, ty: str, val: str):
        self.name = name
        self.ty = ty
        self.val = val
    def generate(self) -> GeneratedItem:
        if self.ty == "QWORD" or self.ty == "DWORD":
            modval = self.val.replace('"', '')
            return GeneratedItem("#define {} {}\n".format(self.name, modval), "%define {} {}\n".format(self.name, modval))
        if self.ty == "STRING":
            modval = self.val.replace("\\", "\\\\")
            return GeneratedItem("#define {} {}\n".format(self.name, modval), "")

genlist: list[GeneratedItem] = []

if len(sys.argv) != 2:
    print('MakeContract: Only provide the Boot Contract JSON file.')
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    data = json.load(f)

for key, entry in data.items():
    item = ToGenerateItem(entry["Name"], entry["Type"], entry["Value"])
    genlist.append(item.generate())

jsonfile = Path(sys.argv[1])
cfile = jsonfile.parent / "Include" / "BootContract" / "Defines.h"
asmfile = jsonfile.parent / "Defines.asm"

with open(cfile, "wb") as f:
    for gen in genlist:
        f.write(gen.inc.encode('utf-8'))
with open(asmfile, "wb") as f:
    for gen in genlist:
        f.write(gen.inasm.encode('utf-8'))
