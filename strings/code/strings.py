#! /usr/bin/python
import sys
from locales import generatePOTFile

def main():
    if len(sys.argv) < 3:
        print os.path.basename(sys.argv[0]), \
              "<input.txt> <output.pot>"
        return -1

    generatePOTFile(sys.argv[1], sys.argv[2])

if __name__ == "__main__": main()
