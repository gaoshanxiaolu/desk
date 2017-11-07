#!/bin/bash

case "$1" in
( '' )  root="$PWD/demo/aj/allseen" ;;
( * )   root="$1" ;;
esac

if [ ! $AJ_SERVICE_SAMPLE ];
then
    export AJ_SERVICE_SAMPLE=ACServerSample
fi

function copytodestination() {

    echo "Copying sample source files to sample destination"


    (
        cp "$1/services/base_tcl/sample_apps/$AJ_SERVICE_SAMPLE/"*.c "$1/../alljoyn/applications/sample_apps/tcl/ServicesSamples/target/aj_qca4004/aj_ServiceSample/"
        cp "$1/services/base_tcl/sample_apps/$AJ_SERVICE_SAMPLE/"*.h "$1/../alljoyn/applications/sample_apps/tcl/ServicesSamples/target/aj_qca4004/aj_ServiceSample/"
        cp "$1/services/base_tcl/sample_apps/AppsCommon/src/"*.c "$1/../alljoyn/applications/sample_apps/tcl/ServicesSamples/target/aj_qca4004/aj_ServiceSample/"
        cp "$1/services/base_tcl/sample_apps/AppsCommon/inc/"*.h "$1/../alljoyn/applications/sample_apps/tcl/ServicesSamples/target/aj_qca4004/aj_ServiceSample/"

        cp "$1/core/ajtcl/test/svclite.c" "$1/../alljoyn/aj_qca4004/apps/aj_svclite/"
        
    )

    return 0
}

(
    LAUNCH_DIR=$(echo $(cd $(dirname $0); pwd )) 
    "$LAUNCH_DIR/dl-alljoyn.sh" "$1" && copytodestination "$root"
)
