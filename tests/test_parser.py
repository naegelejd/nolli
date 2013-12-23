import os
import sys
import glob
import subprocess

def main():
    if len(sys.argv < 2):
        print("Missing argument: nolli binary")
        return 1
    testdir = os.path.join(os.path.abspath(__file__), 'parser')
    inputs = glob.glob(os.path.join(testdir, "*.nl"))
    for filename in inputs:
        cmd = [sys.argv[1]]


if __name__ == "__main__":
    sys.exit(main())
