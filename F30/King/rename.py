#python rename.py <listA> <listB> <_b>
#dark_b.fits

import numpy as np
import sys
import re
#import IPython

def return_xx(string,x1,x2):
    """Return whatever is between x1 and x2 in string"""
    return re.search('(.*)'+x2,string).group(1)

def rename():
	in_file = sys.argv[1]
	out_file = sys.argv[2]
	try:
		ext = sys.argv[3]
	except:
		ext = '_b'

	f1 = open(in_file,'r')
	lines = f1.readlines()
	f2 = open(out_file,'w')

	for l in lines:
		filename = return_xx(l,'','.fits')
		out_filename = filename+ext+'.fits\n'
		f2.write(out_filename)
	f1.close()
	f2.close()

if __name__ == "__main__":
	rename()
