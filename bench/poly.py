#######################################################################
# This script compares the speed of the computation of a polynomial
# for different (numpy.memmap and tables.Expr) out-of-memory paradigms.
#
# Author: Francesc Alted
# Date: 2010-02-24
#######################################################################

import os
import sys
from time import time
import numpy as np
import tables as tb
import numexpr as ne

expr = ".25*x**3 + .75*x**2 - 1.5*x - 2"  # the polynomial to compute
N = 10*1000*1000          # the number of points to compute expression (80 MB)
step = 100*1000           # perform calculation in slices of `step` elements
dtype = np.dtype('f8')    # the datatype
#CHUNKSHAPE = (2**17,)
CHUNKSHAPE = None

# Global variable for the x values for pure numpy & numexpr
x = None

# *** The next variables do not need to be changed ***

# Filenames for numpy.memmap
fprefix = "numpy.memmap"             # the I/O file prefix
mpfnames = [fprefix+"-x.bin", fprefix+"-r.bin"]

# Filename for tables.Expr
h5fname = "tablesExpr.h5"     # the I/O file

MB = 1024*1024.               # a MegaByte


def print_filesize(filename, clib=None, clevel=0):
    """Print some statistics about file sizes."""

    #os.system("sync")    # make sure that all data has been flushed to disk
    if isinstance(filename, list):
        filesize_bytes = 0
        for fname in filename:
            filesize_bytes += os.stat(fname)[6]
    else:
        filesize_bytes = os.stat(filename)[6]
    filesize_MB  = round(filesize_bytes / MB, 1)
    print "\t\tTotal file sizes: %d -- (%s MB)" % (filesize_bytes, filesize_MB),
    if clevel > 0:
        print "(using %s lvl%s)" % (clib, clevel)
    else:
        print


def populate_x_numpy():
    """Populate the values in x axis for numpy."""
    global x
    # Populate x in range [-1, 1]
    x = np.linspace(-1, 1, N)


def populate_x_memmap():
    """Populate the values in x axis for numpy.memmap."""
    # Create container for input
    x = np.memmap(mpfnames[0], dtype=dtype, mode="w+", shape=(N,))

    # Populate x in range [-1, 1]
    for i in xrange(0, N, step):
        chunk = np.linspace((2*i-N)/float(N), (2*(i+step)-N)/float(N), step)
        x[i:i+step] = chunk
    del x        # close x memmap


def populate_x_tables(clib, clevel):
    """Populate the values in x axis for pytables."""
    f = tb.openFile(h5fname, "w")

    # Create container for input
    atom = tb.Atom.from_dtype(dtype)
    filters = tb.Filters(complib=clib, complevel=clevel)
    x = f.createCArray(f.root, "x", atom=atom, shape=(N,),
                       filters=filters,
                       chunkshape=CHUNKSHAPE,
                       )

    # Populate x in range [-1, 1]
    for i in xrange(0, N, step):
        chunk = np.linspace((2*i-N)/float(N), (2*(i+step)-N)/float(N), step)
        x[i:i+step] = chunk
    f.close()


def compute_numpy():
    """Compute the polynomial with pure numpy."""
    y = eval(expr)


def compute_numexpr():
    """Compute the polynomial with pure numexpr."""
    y = ne.evaluate(expr)


def compute_memmap():
    """Compute the polynomial with numpy.memmap."""
    # Reopen inputs in read-only mode
    x = np.memmap(mpfnames[0], dtype=dtype, mode='r', shape=(N,))
    # Create the array output
    r = np.memmap(mpfnames[1], dtype=dtype, mode="w+", shape=(N,))

    # Do the computation by chunks and store in output
    r[:] = eval(expr)          # where is stored the result?
    #r = eval(expr)            # result is stored in-memory

    del x, r                   # close x and r memmap arrays
    print_filesize(mpfnames)


def compute_tables(clib, clevel):
    """Compute the polynomial with tables.Expr."""
    f = tb.openFile(h5fname, "a")
    x = f.root.x               # get the x input
    # Create container for output
    atom = tb.Atom.from_dtype(dtype)
    filters = tb.Filters(complib=clib, complevel=clevel)
    r = f.createCArray(f.root, "r", atom=atom, shape=(N,),
                       filters=filters,
                       chunkshape=CHUNKSHAPE,
                       )

    # Do the actual computation and store in output
    ex = tb.Expr(expr)         # parse the expression
    ex.setOutput(r)            # where is stored the result?
                               # when commented out, the result goes in-memory
    ex.eval()                  # evaluate!

    f.close()
    print_filesize(h5fname, clib, clevel)


if __name__ == '__main__':

    tb.print_versions()

    print "Total size for datasets:", round(2*N*dtype.itemsize/MB, 1), "MB"

    # Get the compression libraries supported
    #supported_clibs = [clib for clib in ("zlib", "lzo", "bzip2", "blosc")
    #supported_clibs = [clib for clib in ("zlib", "lzo", "blosc")
    supported_clibs = [clib for clib in ("blosc",)
                       if tb.whichLibVersion(clib)]

    # Initialization code
    #for what in ["numpy", "numpy.memmap", "numexpr"]:
    for what in ["numpy", "numexpr"]:
        #break
        print "Populating x using %s with %d points..." % (what, N)
        t0 = time()
        if what == "numpy":
            populate_x_numpy()
            compute = compute_numpy
        elif what == "numexpr":
            populate_x_numpy()
            compute = compute_numexpr
        elif what == "numpy.memmap":
            populate_x_memmap()
            compute = compute_memmap
        print "*** Time elapsed populating:", round(time() - t0, 3)
        print "Computing: '%s' using %s" % (expr, what)
        t0 = time()
        compute()
        print "**************** Time elapsed computing:", round(time() - t0, 3)

    for what in ["tables.Expr"]:
        t0 = time()
        first = True    # Sentinel
        for clib in supported_clibs:
            #for clevel in (0, 1, 3, 6, 9):
            for clevel in range(10):
            #for clevel in (1,):
                if not first and clevel == 0:
                    continue
                print "Populating x using %s with %d points..." % (what, N)
                populate_x_tables(clib, clevel)
                print "*** Time elapsed populating:", round(time() - t0, 3)
                print "Computing: '%s' using %s" % (expr, what)
                t0 = time()
                compute_tables(clib, clevel)
                print "**************** Time elapsed computing:", \
                      round(time() - t0, 3)
                first = False

