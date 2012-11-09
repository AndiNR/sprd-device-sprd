#!/bin/sh
echo "create dirs..."
target_dir=`date +%F_%T`
top_dir=`pwd`/logs/$target_dir
mkdir -p $top_dir
mkdir $top_dir/internal_storage
mkdir $top_dir/external_storage
internal_log_dir=`adb shell slogctl query | grep "^internal" | cut -d',' -f2`
external_log_dir=`adb shell slogctl query | grep "^external" | cut -d',' -f2`

echo "capture logs..."
adb shell slogctl screen
adb shell slogctl snap
adb shell slogctl snap bugreport

echo "dump logs..."
cd $top_dir/internal_storage
adb pull $internal_log_dir
cd $top_dir/external_storage
adb pull $external_log_dir
