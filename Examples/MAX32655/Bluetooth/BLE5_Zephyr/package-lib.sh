#!/bin/bash
#
# Build and package up libblectrl.a for zephyr hal.
# TODO: strip out unrequired libperiph, strip symbols.
# option to select libphy-hard?

ZEPHYR_PATH=${ZEPHYR_PATH:-~/zephyrproject}

MAXIM_PATH=${MAXIM_PATH:-~/Workspace/rcornall/msdk}


echo "starting..."

pushd ${MAXIM_PATH}/Examples/MAX32655/Bluetooth/BLE5_Zephyr

echo "PWD: "`pwd`
rm -rf tmp\

cd ../../../../Libraries/
find . -name *.o | xargs rm; find . -name *.a | xargs rm
cd -
echo "PWD: "`pwd`

set +e
rm -r tmp
make distclean
set -e
sleep 0.2
echo "compiling..."
set +e
make -j8; make lib

set -e
echo "packaging..."
sleep 2
mkdir tmp && pushd tmp

ls *.a

cp   ${MAXIM_PATH}/Libraries/PeriphDrivers/bin/MAX32655/spi-v1_softfp/libPeriphDriver_spi-v1_softfp.a ${MAXIM_PATH}/Libraries/Cordio/bin/max32655_zephyr_T0_softfp/cordio_max32655_zephyr_T0_softfp.a ${MAXIM_PATH}/Libraries/BlePhy/MAX32655/libphy.a ../build/max32655.a .
# cp ${MAXIM_PATH}/Libraries/PeriphDrivers/bin/MAX32655/spi-v1_softfp/libPeriphDriver_spi-v1_softfp.a ${MAXIM_PATH}/Libraries/PeriphDrivers/bin/MAX32655/softfp/libPeriphDriver_softfp.a ${MAXIM_PATH}/Libraries/Cordio/bin/max32655_zephyr_T0_softfp/cordio_max32655_zephyr_T0_softfp.a ${MAXIM_PATH}/Libraries/BlePhy/MAX32655/libphy.a ../build/max32655.a .

ar -x cordio_max32655_zephyr_T0_softfp.a
ar -x libPeriphDriver_spi-v1_softfp.a
ar -x libphy.a
ar -x max32655.a

~/zephyr-sdk-0.16.5-1/arm-zephyr-eabi/bin/arm-zephyr-eabi-ar -rcs libblectrl-msdk.a *.o

cp libblectrl-msdk.a /mnt/c/Users/ycai3/Documents/Workspaces/zephyr/adi/modules/hal/adi/zephyr/blobs/lib/max32655/libblectrl.a
sha256sum libblectrl-msdk.a

echo "done...\n$(sha256sum libblectrl-msdk.a)"

sed -i "s/sha256: \(.*\)/sha256: $(sha256sum libblectrl-msdk.a | cut -d ' ' -f1)/" ${ZEPHYR_PATH}/modules/hal/adi/zephyr/module.yml

popd
popd
