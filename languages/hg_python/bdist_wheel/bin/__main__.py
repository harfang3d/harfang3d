import sys
from .run import run
if __name__ == "__main__" and len(sys.argv) > 1:
    tool = run(sys.argv[1])
    exit(tool(*sys.argv[2:]))
