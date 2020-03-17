#!/usr/bin/env python3
import subprocess
import sys
import os
import re

type = ' '.join(sys.argv[1:])

source = f'''
#include <bits/stdc++.h>

void foo({type}) {{}}
'''

CXX = os.getenv('CXX', 'g++')

gcc = subprocess.Popen([CXX, *'-x c++ - -o - -S'.split()], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
gcc.stdin.write(source.encode())
gcc.stdin.close()
gcc.wait()

out = gcc.stdout.read().decode()

mangled = re.search(r'(_Z3foo.*):', out).group(1)
without_fn = mangled.replace('3foo', '', 1)

print(without_fn)
