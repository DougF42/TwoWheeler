#!/bin/bash
# this copies the already-compiled mavlink *.h
# into the current project under the 'include/mavlink' directory
#
here=`pwd`

# Where to get the compiled code from
srcdir=~/shared/mavlink/generated/def2

# Simple name(s) of sub directories to include
component="TwoWheel Common battery joy"

# Where to put the headers (relative to 'here')
dstDir=include/mavLink

#
# IF therre is a Malink directory here,

rm -rf ${here}/${dstDir}
mkdir ${here}/${dstDir}
cp  ${srcdir}/* ${here}/${dstDir} &>/dev/null
#
# Copy the top-level protocol files
#
for word in ${component}
do
    from=${srcdir}/${word}
    to=${here}/${dstDir}/${word}
    echo "cp -r ${from} ${to}"
    cp -r ${from} ${to}
done
