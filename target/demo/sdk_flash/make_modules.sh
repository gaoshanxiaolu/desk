#!/bin/bash
#$1 RAM image
export AR6002_REV=7

#secure image for modules is not supported currently
MAKE_SECURE_BOOT_IMAGE=0

md5_out_file=${MODULEDIR}/md5.bin


NM=${NM:-xt-nm}
LD=${LD:-xt-ld}
STRIP=${STRIP:-xt-strip}

#DEBUG=1
dbg()
{
    if [ "$DEBUG" ]; then
        echo $(basename $0) : $*
    fi
}

dbg Start $*

if [ -z "$1" ]; then
    echo $0 ERROR: No RAM Image specified
    exit 1
fi

RAM_IMAGE=$1
RAM_IMAGE_NAME=`basename $1 .out`
MODULES=`ls ${MODULEDIR}/*.mout`
FLASH_FILE_SUFFIX=_AR401X_REV${AR6002_REV7_VER}_IOT_${RAM_IMAGE_NAME}.mbin

dbg RAM_IMAGE=$1


OTA_FILENAME="${BINDIR}/ota_image${FLASH_FILE_SUFFIX}"
#RAW_FILENAME="${BINDIR}/raw_flashimage${FLASH_FILE_SUFFIX}"

FLASH_FILENAME=${MODULEDIR}/flash.mbin

rm -f $FLASH_FILENAME $OTA_FILENAME $RAW_FILENAME 

VER=$AR6002_REV7_VER


# Append a 32-bit word to the md5 output file.
print_32bit_md5()
{
	word32=$($PRINTF "%08x\n" $1)

	byte0=$(echo $word32 | cut -b 7-8)
	byte1=$(echo $word32 | cut -b 5-6)
	byte2=$(echo $word32 | cut -b 3-4)
	byte3=$(echo $word32 | cut -b 1-2)

	$PRINTF "\x$byte0" >> $md5_out_file
	$PRINTF "\x$byte1" >> $md5_out_file
	$PRINTF "\x$byte2" >> $md5_out_file
	$PRINTF "\x$byte3" >> $md5_out_file
}

prepare_module_image()
{

    dbg "Prepare module images"

    ${NM} -s ${RAM_IMAGE} >${MODULEDIR}/${RAM_IMAGE_NAME}.syms
    for file in $MODULES
    do
       # Undefined symbols
       SYMS=`${NM} -s ${file} | awk '{if($1 == "U") print $2}'`
       echo "$SYMS" > ${file}.syms
       rm -f ${file}.ram.addrs.ld

       # Get addess of symbols
       for sym in $SYMS
       do
           ADDR=`awk '{if($3 == sym) print $1}' sym=$sym ${MODULEDIR}/${RAM_IMAGE_NAME}.syms`
           if [ -n "$ADDR" ]; then
               echo "PROVIDE ( ${sym} = 0x${ADDR} );" >> ${file}.ram.addrs.ld
           else
               echo "Module file ${file}:"
               echo "Undefined symbol: ${sym}"
               exit 1
           fi
       done

       # Partial Linking for RAM symbols
       if [ -f ${file}.ram.addrs.ld ]; then
           ${LD} -r  -o ${file}.abs -T ${file}.ram.addrs.ld  ${file}
       else
           cp -f ${file} ${file}.abs
       fi

       # Strip unused sections
       ${STRIP} -g -R .comment -R .xtensa.info -R .rela.xt.prop -R .xt.prop -o ${file}.strip ${file}.abs

    done
}

gen_flash_image()
{
########################################
# Create the applications segment of the flash image from ELF files
# NVRAM format:
#     32-bit pointer to partition table
#     32-bit unused
#     32-bit unused
#     32-bit unused
#     (Start of partition 0)
#     32-bit partition magic value (validates partition)
# START post-REV72 addition {
#     32-bit partition age (used when selecting which partition to use)
#     32-bit unused
#     32-bit unused
#     32-bit unused
#     32-bit unused
#     32-bit unused
#     32-bit unused
#	  32-bit * 4 MD5 of $1	
# END post_rev72 addition }
#     first meta-data entry
#     first data
#     second meta-data entry
#     second data
#      ....
########################################



# Partition header preceded by the 4-word flash header
# for partition 0.  This will be stripped out later if
# the image is not intended for use as partition 0.
    if [ $AR6002_REV7_VER -ge 5 ] #{
        then
        #Split the flash image so that secure boot header and footer
        #can be inserted
        ${TOOLDIR}/makeimg.sh \
            -out flash_start \
            -new \
            -word 0 \
            -fill 0xffffffff 3 \
            -magic \
            -word 0\
            -fill 0xffffffff 6

		MD5=$(md5sum $RAM_IMAGE | cut -f1 -d " ")
		dbg md5=$MD5
		
		rm -f $md5_out_file
		
	  	#Create md5sum image header
		print_32bit_md5 0x$(echo $MD5 | cut -b 1-8)
		print_32bit_md5 0x$(echo $MD5 | cut -b 9-16)
		print_32bit_md5 0x$(echo $MD5 | cut -b 17-24)
		print_32bit_md5 0x$(echo $MD5 | cut -b 25-32)
		
        for file in $MODULES
        do
            dbg ${TOOLDIR}/makeimg.sh -out mseg -name `basename $file .mout` -noload "${file}.strip"
            ${TOOLDIR}/makeimg.sh -out mseg -name `basename $file .mout` -noload "${file}.strip"
        done       
    	
        if [ "$MAKE_SECURE_BOOT_IMAGE" -eq 1 ]; then #{

             #openssl commands to generate key pair
             #openssl genpkey -algorithm RSA -out testkey.private -pkeyopt rsa_keygen_bits:2048 -pkeyopt rsa_keygen_pubexp:3
             #openssl rsa -in testkey.private -pubout -out testkey.public

            if [ ! -f testkey.private ] || [ ! -f testkey.public ]; then #{
                echo "Key files not found! Exiting..."
                exit 1
            fi #}

            #Calculate image hash
            imghash=$(sha256sum mseg | sed 's/ .*//')

            #Length of unsigned image
            imglen=$(stat --format=%s mseg)
  
            echo img length $imglen

            #changeme: Values used for testing
            imgtype=0x12345678
            imgversion=0x6
            oemid=0x5678
            prdid=0xabcd

            #Create secure image header
            ${TOOLDIR}/makeimg.sh \
                 -out secure_img_hdr \
                 -new \
                 -secureimghdr $imgtype $imglen $imgversion $oemid $prdid $imghash

             #Calculate signature of image header using SHA256_2048_RSA_PSS
             openssl dgst -sha256 -sign testkey.private -out signature.sign -sigopt rsa_padding_mode:pss -sigopt rsa_pss_saltlen:32 secure_img_hdr

             #OpenSSL command to verify
             #openssl dgst -sha256 -verify testkey.public -signature signature.sign -sigopt rsa_padding_mode:pss -sigopt rsa_pss_saltlen:32 secure_img_hdr 

             #Extract modulo of public key
             ${TOOLDIR}/makeimg.sh \
                 -out public_key_modulo \
                 -new \
                 -publickeymod testkey.public

             cat flash_start secure_img_hdr mseg signature.sign public_key_modulo > $FLASH_FILENAME

             rm -f flash_start
             rm -f secure_img_hdr
             rm -f mseg
             rm -f public_key_modulo
             rm -f apps
             rm -f signature.sign
         #}
         else  #{
             #Secure boot image not needed
             cat flash_start $md5_out_file mseg > $FLASH_FILENAME
             rm -f flash_start
             rm -f $md5_out_file
             rm -f mseg
         fi #}
    fi #}
}   

gen_raw_module_image()
{
	RAW_FILENAME_WITH_MODULE=$BINDIR/`basename $1 _4bitflash.bin`_module_4bitflash.bin

	dbg $RAW_FILENAME_WITH_MODULE

	echo $1
	echo Creating image: `basename $RAW_FILENAME_WITH_MODULE`
	LENGTH=$(stat --format=%s $FLASH_FILENAME)
	dbg original module image length=$LENGTH

	# Remove the leading 4 words which are always in the raw_flashimage:
	#	pointer to partition table
	#	3 TBD words
	let LENGTH=LENGTH-16

	tail -c $LENGTH $FLASH_FILENAME > tmp.bin

	cat $1 > $RAW_FILENAME_WITH_MODULE
	
	cat tmp.bin >> $RAW_FILENAME_WITH_MODULE
		rm -f tmp.bin

}

# Parse RAM symbols
prepare_module_image

# Generate flash image
gen_flash_image

#Generate raw flash image with modules
gen_raw_module_image $BINDIR/raw_flashimage_AR401X_REV6_IOT_hostless_ap+sta_mcc_singleband_4bitflash.bin

#cp $FLASH_FILENAME $RAW_FILENAME
# Done creating RAW image

# Generate OTA image 
${TOOLDIR}/ota.sh $FLASH_FILENAME  $OTA_FILENAME
if [ $? -ne 0 ]; then
    echo $0 Error: ota.sh failed to create OTA image
    exit 1
fi

echo Flash images are in $BINDIR

dbg End
exit 0
