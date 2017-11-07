cd ./demo/sdk_shell
make clean
make
cd ../../tool
./qonstruct.sh --qons /tmp/qons
cp ../bin/raw_flashimage_AR401X_REV6_IOT_MP1_hostless_unidev_singleband.bin ~/Share/raw1030.bin
cd ..
