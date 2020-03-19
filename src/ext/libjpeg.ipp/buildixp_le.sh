#! /bin/sh

#
# Usage: build.sh {gcc}
#

ARCH=linuxixp_le

# only static IPP libraries are available for IPP for IXP
LINKAGE=static


IPPENV=${IPPROOT}/tools/env/ippvarsixp.sh

if [ ! -x "${IPPENV}" ]; then
  IPPENV=/opt/intel/ipp/5.0/ixp/tools/env/ippvarsixp.sh
fi

if [ ! -x "${IPPENV}" ]; then
  echo -e "*************************************************************************"
  echo -e "Intel(R) IPP is not found!"
  echo -e "Please install Intel(R) IPP or set IPPROOT environment variable correctly."
  echo -e "*************************************************************************"
  exit
fi

ARG=$1

CC=/opt/mvlcee/devkit/arm/iwmmxt_le/bin/iwmmxt_le-gcc
CXX=/opt/mvlcee/devkit/arm/iwmmxt_le/bin/iwmmxt_le-g++

. ${IPPENV}

make ARCH=${ARCH} clean
make ARCH=${ARCH} CC=${CC} CXX=${CXX} LINKAGE=${LINKAGE}
