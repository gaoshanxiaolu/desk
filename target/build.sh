cd ./demo/sdk_shell
make clean
make
cd ../../tool
./qonstruct.sh --qons /tmp/qons
cp ../bin/raw_flashimage_AR401X_REV6_IOT_MP1_hostless_unidev_singleband.bin /mnt/hgfs/E/ART2_IOE/bin/raw_flashimage_AR401X_REV6_IOT_hostless_unidev_dualband.bin
cd ..
