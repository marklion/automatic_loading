#!/bin/bash
sed -i "5a[web${1}]" ${2}
sed -i "8asubdomain = ${1}" ${2}
sed -i "10a[web_rdp_${1}]" ${2}
sed -i "13asubdomain = ${1}_rdp" ${2}