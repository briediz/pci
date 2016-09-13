#!/bin/sh
#
CWD=`pwd`
BUILDDIR=`readlink -e $0`
BUILDDIR+=`dirname $BUILDDIR`
STARS="**************			"
RELEASE=LinuxOutput

exit_with_help(){
	cd $CWD
	echo "three arguments is needed"
	echo "first arg: path to the kernel modules sources "
	echo "second arg: path to the kernel sources"
	echo "third arg: module name"
	exit -1
}

# check argument
if [ $# -lt 3 ]; then
	exit_with_help
fi

source /opt/fsl-qoriq/1.9/environment-setup-ppce500v2-fsl-linux-gnuspe
unset LDFLAGS

MODULESOURCE=$1
KERNELSOURCE=$2

#check whether path is relative
#if relative, than make it absolute
[[ "$2" == /* ]] || KERNELSOURCE=$CWD/$2
DEVICE=`head -n1 $KERNELSOURCE/SickDeviceConfig.cfg`
MODULENAME=$3
RES=0

echo ==================== 1

[ -d $MODULESOURCE ] || { echo "module source does not exist"; exit_with_help; }
[ -d $KERNELSOURCE ] || { echo "kernel source does not exist"; exit_with_help; }

cd $MODULESOURCE

echo " $MODULENAME remove unecessary lines "
sed -ie '/DEV_PATH/d' Makefile

echo ==================== 2

echo "$MODULENAME replace the Module name"
sed -ie 's|^\s*MODULES\s*:=.*|MODULES := '$MODULENAME'|g' Makefile

#replace the kernel source location
sed -ie 's|^\s*ROOTDIR.*|ROOTDIR := '$KERNELSOURCE'|g' Makefile

#build the module
make clean || RES=1
echo ==================== 3.1
echo === $PWD ===


make || RES=1

echo ==================== 3.2

[ $RES -eq 1 ] && { echo " Could not build module $MODULENAME"; exit -1; }

[ -d $CWD/$RELEASE ] || mkdir $CWD/$RELEASE

echo ==================== 4

echo "DEVICE............$DEVICE"
if grep -ie stereo <<< $DEVICE; then #check whether the word stereo exist in the second argument
	 OUTPUTFOLDER=$CWD/$RELEASE/KernelStereoOutput
   echo "$STARS create Stereo Output directory"
   [ -d $OUTPUTFOLDER ] || mkdir $OUTPUTFOLDER
	 cp $MODULENAME.ko $OUTPUTFOLDER
elif grep -ie tof <<< $DEVICE; then
	 OUTPUTFOLDER=$CWD/$RELEASE/KernelTofOutput
   echo "$STARS create Tof Output directory"
   [ -d $OUTPUTFOLDER ] || mkdir $OUTPUTFOLDER
	 cp $MODULENAME.ko $OUTPUTFOLDER
elif grep -ie pcirescan <<< $DEVICE; then
	 OUTPUTFOLDER=$CWD/$RELEASE/KernelTofOutput
   echo "$STARS create Tof Output directory"
   [ -d $OUTPUTFOLDER ] || mkdir $OUTPUTFOLDER
	 cp $MODULENAME.ko $OUTPUTFOLDER

fi

[ -e $CWD/$MODULESOURCE/.svn/wc.db ] && cp $CWD/$MODULESOURCE/.svn/wc.db $CWD/$MODULENAME.db

cd $CWD




