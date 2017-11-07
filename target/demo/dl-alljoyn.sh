#!/bin/bash

case "$1" in
( '' )  root="$PWD/demo/aj/" ;;
( * )   root="$1" ;;
esac

function downloadexpand() {

    echo "Downloading and extracting https://git.allseenalliance.org/cgit/$2/$3.git/snapshot/$4.zip"

    (
        set -e
        cd "$1/allseen/$2"    || { echo >&2 "error, directory $1/allseen/$2 not found" ; exit 2 ; }
        rm -rf "$4" "$4.zip"
        w_opt="-nv --no-check-certificate"
        wget $w_opt "https://git.allseenalliance.org/cgit/$2/$3.git/snapshot/$4.zip"
        unzip -q "$4.zip" 
        mv -f  "$4" "$3"
        rm -rf "$4.zip" "$4"
    )

    ( cd "$1/allseen/$2/$3" ) || { echo >&2 "error, directory $1/allseen/$2/$3 not found after downloading and expanding" ; exit 2 ; }
}

echo "Installing AllSeen Alliance files in $root"

mkdir -p "$root/allseen/core" "$root/allseen/services" || : ok

downloadexpand  "$root" core        ajtcl       6ab10a087d14d25b9850cab025afb9e0c97a5a59
downloadexpand  "$root" core        alljoyn-js  57e7f26fad727bb41015fff46f7d96fe261dc38c
downloadexpand  "$root" services    base_tcl    91fcc0a2bdffd9630de71cc9c4d660c0d508841e

function downloadexpandduktape() {

    echo "Downloading and extracting http://duktape.org/duktape-$2.tar.xz"

    (
        set -e
        cd "$1"    || { echo >&2 "error, directory $1 not found" ; exit 2 ; }
        rm -rf "duktape-$2.tar.xz"
        w_opt="-nv"
        wget $w_opt "http://duktape.org/duktape-$2.tar.xz"
        tar xfJ "duktape-$2.tar.xz" 
        mv -f  "duktape-$2" "duktape"
        rm "duktape-$2.tar.xz" 
    )

    ( cd "$1/duktape" ) || { echo >&2 "error, directory $1/duktape not found after downloading and expanding" ; exit 2 ; }
}

echo "Installing Duktape files in $root"

downloadexpandduktape  "$root" "1.2.1"
