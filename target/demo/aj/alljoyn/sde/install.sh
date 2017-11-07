#!/bin/bash

SDE_VERSION=1.0.0
QCA401X_LIBRARY_PROJECT=qca401xLibraries
ALLJOYN_LIBRARY_PROJECT=liballjoyn
ALLJOYN_SERVICES_LIBRARY_PROJECT=liballjoyn_services

LIBRARY_PROJECT_DIR="$PWD"
QCA401X_SDK_ROOT_DIR="$PWD/../../../.."

########################################
# Print usage message
########################################
usage()
{
cat << USAGE_MSG
Options:

--xtensadir xtensa_install_directory
    Specify the path to the Xtensa base install directory.

--help
    Show this help message.

USAGE_MSG
}

########################################
# Parse command line arguments
########################################
while [ $# -ne 0 ]
do
    arg=$1
    shift 1
    case $arg in
    --xtensadir)
        if [ $# -ge 1 ]; then
            XTENSA_INSTALL_DIR="$1"
            shift 1
        else
            echo No directory specified with $arg.
            exit 1
        fi
        ;;
    -? | --? | -h | --help)
        usage
        exit 1
        ;;
    esac
done

echo "Installing SDE..."
echo "SDE_VERSION=$SDE_VERSION"
echo "XTENSA_INSTALL_DIR=$XTENSA_INSTALL_DIR"
echo "QCA401X_SDK_ROOT_DIR=$QCA401X_SDK_ROOT_DIR"
echo "LIBRARY_PROJECT_DIR=$LIBRARY_PROJECT_DIR"

########################################
# delete any previous template projects
########################################
echo "deleting any previous template projects in $LIBRARY_PROJECT_DIR"
rm -rf $LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT
rm -rf $LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT
rm -rf $LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT

########################################
# extract package.tgz
########################################
echo "extracting package.tgz"
tar -xzvf package.tgz

########################################
# copy source from SDK into 3 template projects
########################################

# QCA4010x SDK library project
echo "populating template project: $QCA401X_LIBRARY_PROJECT"
cp -r "$QCA401X_SDK_ROOT_DIR/bddata" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/bddata"
mkdir "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image"
cp "$QCA401X_SDK_ROOT_DIR/image/boot_router.out" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/otp_iot.out" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/sdk_flash.out" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/sdk_proxy.out" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/tune.out" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/rom.addrs.ld" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp "$QCA401X_SDK_ROOT_DIR/image/rom.externs.ld" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/image/"
cp -r "$QCA401X_SDK_ROOT_DIR/lib" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/lib"
rm "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/lib/liballjoyn.a"
rm "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/lib/liballjoyn_services.a"
rm "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/lib/libaj_ServiceSample.a"
mkdir -p "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/Internal/include"
cp -r "$QCA401X_SDK_ROOT_DIR/Internal/include/"* "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/Internal/include/"
cp -r "$QCA401X_SDK_ROOT_DIR/include" "$LIBRARY_PROJECT_DIR/$QCA401X_LIBRARY_PROJECT/include"

# liballjoyn library project
echo "populating template project: $ALLJOYN_LIBRARY_PROJECT"
mkdir "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004"
cp -r "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/inc" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/inc"
mkdir "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_malloc.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_malloc.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_net.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_net.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_peer_q.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_peer_q.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_target_crypto.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_target_crypto.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_target_nvram.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_target_nvram.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_target_util.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_target_util.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/aj_wifi_ctrl.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/aj_wifi_ctrl.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/alljoyn/aj_qca4004/src/alljoyn.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/aj_qca4004/src/alljoyn.c"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl"
cp -r "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/inc" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/inc"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_about.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_about.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_ardp.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_ardp.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_authentication.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_authentication.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_bufio.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_bufio.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_bus.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_bus.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_cert.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_cert.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_connect.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_connect.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_crc16.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_crc16.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_creds.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_creds.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_debug.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_debug.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_disco.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_disco.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_guid.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_guid.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_helper.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_helper.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_init.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_init.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_introspect.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_introspect.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_link_timeout.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_link_timeout.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_msg.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_msg.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_nvram.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_nvram.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_std.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_std.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/src/aj_util.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/src/aj_util.c"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/crypto/ecc"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/crypto/ecc/aj_crypto_ecc.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/crypto/ecc/aj_crypto_ecc.c"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/malloc"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/malloc/aj_malloc.c" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/malloc/aj_malloc.c"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/malloc/aj_malloc.h" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/malloc/aj_malloc.h"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/external/sha2"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/core/ajtcl/external/sha2/sha2.h" "$LIBRARY_PROJECT_DIR/$ALLJOYN_LIBRARY_PROJECT/allseen/core/ajtcl/external/sha2/sha2.h"

# liballjoyn_services library project
echo "populating template project: $ALLJOYN_SERVICES_LIBRARY_PROJECT"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/config/inc/alljoyn/config"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/config/src"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/config/inc/alljoyn/config/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/config/inc/alljoyn/config/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/config/src/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/config/src/"

mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Common"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Widgets"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee/Common"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee/Widgets"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Common/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Common/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Widgets/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/inc/alljoyn/controlpanel/Widgets/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/src/CPSControllee/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/src/CPSControllee/Common/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee/Common/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/controlpanel/src/CPSControllee/Widgets/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/controlpanel/src/CPSControllee/Widgets"

mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/inc/alljoyn/notification"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationCommon"
#mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationConsumer"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationProducer"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/notification/inc/alljoyn/notification/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/inc/alljoyn/notification/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/notification/src/NotificationCommon/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationCommon/"
#cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/notification/src/NotificationConsumer/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationConsumer/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/notification/src/NotificationProducer/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/notification/src/NotificationProducer/"

mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/onboarding"
cp -r "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/onboarding/inc" "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/onboarding/inc"
cp -r "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/onboarding/src" "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/onboarding/src"

mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/services_common/inc/alljoyn/services_common"
mkdir -p "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/services_common/src"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/services_common/inc/alljoyn/services_common/"*.h "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/services_common/inc/alljoyn/services_common/"
cp "$QCA401X_SDK_ROOT_DIR/demo/aj/allseen/services/base_tcl/services_common/src/"*.c "$LIBRARY_PROJECT_DIR/$ALLJOYN_SERVICES_LIBRARY_PROJECT/allseen/services/base_tcl/services_common/src/"

########################################
# Install SDE plugin and dependencies
########################################

# install plugin dependencies
echo "downloading and installing SDE plugin dependencies"
$XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/jre/bin/java -jar $XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/plugins/org.eclipse.equinox.launcher_1.2.0.v20110502.jar -application org.eclipse.equinox.p2.director -repository http://download.eclipse.org/releases/indigo -i org.eclipse.wst.sse.core,org.eclipse.wst.sse.ui,org.eclipse.wst.xml.core,org.eclipse.wst.xml.ui

# remove any previous versions of the plugin
rm -rf $XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/dropins/org.alljoyn.sde*
rm -rf $XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/dropins/com.qualcomm.qce.sde*

# copy jar into eclipse dropins dir
unzip com.qualcomm.qce.sde_*.jar -d $XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/dropins/com.qualcomm.qce.sde_$SDE_VERSION
chmod a+x $XTENSA_INSTALL_DIR/Xplorer-5.0.3/eclipse/dropins/com.qualcomm.qce.sde_$SDE_VERSION/scripts/*

echo "Install complete..."
