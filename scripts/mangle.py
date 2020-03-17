#!/usr/bin/env python3
import subprocess
import sys
import os
import re
import pickle

HOME = os.getenv('HOME')

type = ' '.join(sys.argv[1:])

if not type: type = sys.stdin.read()

type = type.strip()

CXX = os.getenv('CXX', 'g++')
cache_key = (CXX, type)

def get_mangled_name(key):
    source = f'''
    #include <bits/stdc++.h>

    void foo({type}) {{}}
    '''

    os.makedirs(f'{HOME}/.cache/c++mangle/', mode=0o755, exist_ok=True)
    try:
        with open(f'{HOME}/.cache/c++mangle/mangle.cache', 'rb') as f:
            cache = pickle.load(f)
            if cache and key in cache:
                return cache[key]
    except:
        pass

    gcc = subprocess.Popen([CXX, *'-x c++ - -o - -S'.split()], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    gcc.stdin.write(source.encode())
    gcc.stdin.close()
    gcc.wait()

    out = gcc.stdout.read().decode()

    mangled = re.search(r'(_Z3foo.*):', out).group(1)
    without_fn = mangled.replace('3foo', '', 1)

    try:
        with open(f'{HOME}/.cache/c++mangle/mangle.cache', 'rb') as f:
            cache = pickle.load(f)
    except:
        cache = dict()

    try:
        with open(f'{HOME}/.cache/c++mangle/mangle.cache', 'wb') as f:
            cache[key] = without_fn
            pickle.dump(cache, f)
    except:
        pass

    return without_fn

print(get_mangled_name(cache_key))
