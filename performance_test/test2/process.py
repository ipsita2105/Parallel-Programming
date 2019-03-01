#!/usr/bin/env python

import sys
import os
import re
import optparse

def main():

    parser = optparse.OptionParser()
    parser.add_option('-f','--file',dest = 'fname',help='Enter file name to process');
    (options, args) = parser.parse_args()

    if options.fname is None:
        print "give file name"
        sys.exit(1)

    f1 = open(options.fname, 'r')

    line = f1.readline()
    
    while line != "":

        if 'input' in line:
            templine = line.split(" ")

            tsum = 0.0
            for i in range(0, 2):
                line = f1.readline()
                tsum = tsum + float(line) 

            print str(tsum/2.0)

        line = f1.readline() 

if __name__ == '__main__':
    main()
