#! /usr/bin/python
import sys
from locales import generateDefines

def main():
    if len(sys.argv) < 3:
        print os.path.basename(sys.argv[0]), \
              "<input.txt> <strings.h>"
        return -1

    generateDefines(sys.argv[1], sys.argv[2])

if __name__ == "__main__": main()
