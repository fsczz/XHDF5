#! /bin/sh
#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the files COPYING and Copyright.html.  COPYING can be found at the root
# of the source code distribution tree; Copyright.html can be found at the
# root level of an installed copy of the electronic HDF5 document set and
# is linked from the top-level documents page.  It can also be found at
# http://hdfgroup.org/HDF5/doc/Copyright.html.  If you do not have
# access to either file, you may request a copy from help@hdfgroup.org.

#
#  This file:  run-c-ex.sh
# Written by:  Larry Knox
#       Date:  May 11, 2010
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                               #
# This script will compile and run the c examples from source files installed   #
# in .../share/hdf5_examples/c using h5cc or h5pc.  The order for running       #
# programs with RunTest in the MAIN section below is taken from the Makefile.   #
# The order is important since some of the test programs use data files created #
# by earlier test programs.  Any future additions should be placed accordingly. #
#                                                                               #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Initializations
EXIT_SUCCESS=0
EXIT_FAILURE=1
 
# Where the tool is installed.
# default is relative path to installed location of the tools 
prefix="${prefix:-/usr/vol-rest}"
PARALLEL=@PARALLEL@             # Am I in parallel mode?
AR="ar"
RANLIB="ranlib"
if [ "$PARALLEL" = no ]; then
    H5TOOL="h5cc"               # The tool name
else
    H5TOOL="h5pcc"               # The tool name
fi
H5TOOL_BIN="${prefix}/bin/${H5TOOL}"   # The path of the tool binary

#### Run test ####
RunTest()
{
    TEST_EXEC=$1
    Test=$1".c"

    echo
    echo "#################  $1  #################"
    ${H5TOOL_BIN} -o $TEST_EXEC $Test
    if [ $? -ne 0 ]
    then
        echo "messed up compiling $Test"
        exit 1
    fi
    ./$TEST_EXEC
}



##################  MAIN  ##################

if ! test -d red; then
   mkdir red
fi
if ! test -d blue; then
   mkdir blue
fi
if ! test -d u2w; then
   mkdir u2w
fi

# Run tests
if [ $? -eq 0 ]
then
    if (RunTest rv_crtdat &&\
        rm rv_crtdat &&\
        RunTest rv_rdwt &&\
        rm rv_rdwt &&\
        RunTest rv_crtatt &&\
        rm rv_crtatt &&\
        RunTest rv_crtgrp &&\
        rm rv_crtgrp &&\
        RunTest rv_crtgrpar &&\
        rm rv_crtgrpar &&\
        RunTest rv_crtgrpd &&\
        rm rv_crtgrpd &&\
        RunTest rv_subset &&\
        rm rv_subset &&\
        RunTest rv_write &&\
        rm rv_write &&\
        RunTest rv_read &&\
        rm rv_read &&\
        RunTest rv_chunk_read &&\
        rm rv_chunk_read &&\
        RunTest rv_compound &&\
        rm rv_compound &&\
        RunTest rv_group &&\
        rm rv_group &&\
        RunTest rv_select &&\
        rm rv_select &&\
        RunTest rv_attribute &&\
        rm rv_attribute &&\
        RunTest rv_extlink &&\
        rm rv_extlink &&\
        RunTest hl/rv_ds1 && \
        rm hl/rv_ds1); then
        EXIT_VALUE=${EXIT_SUCCESS}
    else
        EXIT_VALUE=${EXIT_FAILURE}
    fi
fi

# Cleanup
rm *.o
rm *.h5
rm -rf red blue u2w
echo

exit $EXIT_VALUE 

