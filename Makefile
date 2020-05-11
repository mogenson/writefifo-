# Makefile to build class 'writefifo' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = writefifo

# input source file (class name == source file basename)
class.sources = writefifo~.c

# all extra files to be included in binary distribution of the library
datafiles =

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=../pd-lib-builder/
include Makefile.pdlibbuilder
