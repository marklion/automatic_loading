#!/bin/bash
sed -i "6a[web${1}]" ${2}
sed -i "9asubdomain = ${1}" ${2}