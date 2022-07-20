#!/usr/bin/python3

usage = """python sdcc_map.py <filename.map>
 * Parses an sdcc linker .map file and outputs symbol, section, module 
   information in .csv format
 * Requires python 3.8+
"""

import sys,os
import re
from types import SimpleNamespace

class Symbol:
    def __init__(self, name, address, module, section):
        self.name = name
        self.address = address
        self.module = module
        self.section = section
        self.size = -1

class Module:
    def __init__(self, name):
        self.name = name
        self.symbols = {}
        self.size = 0

    def addSymbol(self, symbol):
        self.symbols[symbol.name] = symbol

    def process(self):
        self.size = 0
        for (n,s) in self.symbols.items():
            self.size += s.size

class Section:
    def __init__(self, name, address, size, attributes):
        self.name =name
        self.address = address
        self.size = size
        self.attributes = attributes
        self.symbols = []

    def begin(self):
        return self.address

    def end(self):
        return self.address+self.size

    def process(self):
        if len(self.symbols) <= 0:
            return

        self.symbols.sort(key=lambda x: x.address, reverse=True)

        sectionEnd = self.end()
        for symbol in self.symbols:
            symbol.size = sectionEnd - symbol.address
            sectionEnd = sectionEnd - symbol.size





class MapFile:
    def __init__(self):
        self.sections = []
        self.modules = {}
        self.symbols = []

    def addSection(self, section):
        self.sections.append(section)

    def getModule(self, name):
        if name in self.modules:
            return self.modules[name]
        
        module = self.modules[name] = Module(name)
        return module

    def fromFile(self, filename):
        currentSection = None
        sectionHeaderRegex = re.compile("(?P<name>.*?)\s+(?P<address>.*?)\s+(?P<sizeHex>.*?)\s=\s+(?P<sizeBytes>\d+)\.\sbytes\s\((\w+),(\w+),(?P<type>\w+)\)")
        symbolReg = re.compile("(?P<type>C|D):\s+(?P<address>.*?)\s+(?P<name>\w+)\s+(?P<module>\w+|.*)")
        
        mapfile = open(filename, "r")
        skip = 0
        while line := mapfile.readline():
            # SECTION DATA
            if len(line) > 0 and line[0] == '\x0c':
                # skip header
                for skip in range(4): 
                    mapfile.readline()

                line = mapfile.readline()
                match = sectionHeaderRegex.match(line)
                if not match:
                    continue

                secHead = SimpleNamespace(**match.groupdict())
                
                # print("Section Header: {secHead.name}")

                existing = [x for x in self.sections if x.name == secHead.name]
                if len(existing) >= 1:
                    
                    # print(F"found existing section: {secHead.name}@{existing[0]}")

                    found = existing[0]
                    if found != currentSection:
                        # print(F"Found existing section: {found.name} that does not match current: {currentSection.name}")
                        currentSection = found
                else:
                    currentSection = Section(secHead.name, int(secHead.address,16), int(secHead.sizeBytes), secHead.type)
                    self.sections.append(currentSection)
                    # print(F"Creating new section: {currentSection.name}")
            elif match := symbolReg.match(line):
                sd = SimpleNamespace(**match.groupdict())
                if len(sd.module) > 0:
                    module = self.getModule(sd.module)
                    symbol = Symbol(sd.name, int(sd.address,16), module, currentSection)
                    module.addSymbol(symbol)
                    self.symbols.append(symbol)

                    # print(F"Found symbol: {symbol.name}")
                    currentSection.symbols.append(symbol)
                

    def process(self):
        for section in self.sections:
            section.process()

        for module in self.modules.values():
            module.process()

        self.symbols.sort(key=lambda x: x.size, reverse=True)
        self.sections.sort(key=lambda x: x.size, reverse=True)

    def symbolsCsv(self) -> str:
        out = ""
        out += "Bytes, Symbol, Module, Address, Section, SectionType\n"
        
        
        for symbol in self.symbols:
            out += F'{symbol.size}, {symbol.name}, {symbol.module.name}, {hex(symbol.address)}, {symbol.section.name}, {symbol.section.attributes}\n'

        return out
    
    def sectionsCsv(self) -> str:
        out = ""
        out += "Bytes, Section, Address, SectionType\n"
        
        for s in self.sections:
            out += F'{s.size}, {s.name}, {hex(s.address)}, {s.attributes}\n'

        return out

    def modulesCsv(self) -> str:
        out = ""
        out += "Bytes, Module, Symbols\n"
                
        for m in sorted(self.modules.values(), key=lambda x: x.size, reverse=True):
            out += F'{m.size}, {m.name}, {len(m.symbols.keys())}\n'

        return out

def writeFile(filename, name, ext, data):
    file = open(f"{os.path.abspath(filename)}.{name}.{ext}", "w")
    file.write(data)
    file.close()

def main(infile):
    
    if not os.path.isfile(infile):  
        print("Unreadable file '%s'." % infile)
        return 1

    map = MapFile()
    map.fromFile(infile)
    map.process()

    writeFile(infile, "symbols", "csv", map.symbolsCsv())
    writeFile(infile, "sections", "csv", map.sectionsCsv())
    writeFile(infile, "modules", "csv", map.modulesCsv())

    return 0

if __name__=="__main__":
    if len(sys.argv)>=2:
        sys.exit(main(sys.argv[1]))
    else:
        print(help)
        sys.exit(1)
