#!/bin/tcsh
set sourced=($_)
if ("$sourced" != "") then
    echo "sourced $sourced[2]"
    set t=`dirname $sourced[2]`
    setenv WRAP_HOME ${t}/..
else if ("$0" != "tcsh") then
    echo "run $0"
endif

setenv LD_LIBRARY_PATH ${WRAP_HOME}/lib
setenv SCM_TWR 1000

setenv AliasTable AT2
setenv AliasTableSize 13
setenv AliasTableThresh 8
setenv Debug 0
