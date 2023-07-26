#!/bin/sh
 
set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-./out}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install}
echo ${INSTALL_DIR}
BUILD_NO_EXAMPLES=${BUILD_NO_EXAMPLES:-1}
BUILD_DYNAMIC_LIB=${BUILD_DYNAMIC_LIB:-0}
BUILD_JEMALLOC=0

#if [ $1 = "jemalloc" ];then
#BUILD_JEMALLOC=1
#fi

mkdir -p $BUILD_DIR/$BUILD_TYPE \
  && cd $BUILD_DIR/$BUILD_TYPE \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_BUILD_NO_EXAMPLES=$BUILD_NO_EXAMPLES \
           -DCMAKE_BUILD_DYNAMIC_LIB=$BUILD_DYNAMIC_LIB \
		   -DCMAKE_BUILD_JEMALLOC=$BUILD_JEMALLOC \
           $SOURCE_DIR \
   && make $*
#  && make 

# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# cd $SOURCE_DIR && doxygen
