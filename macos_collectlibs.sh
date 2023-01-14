#!/usr/bin/env bash
####################

collectlibraries() {
    final=0
    for lib in $(otool -L ${1} | grep "/opt/local" | awk '{ print $1 }' | sort | uniq); do
        basename=$(basename ${lib})
        if [ ! -f "ep128emu.app/Contents/Frameworks/${basename}" ]; then 
            final=1
            cp ${lib} ep128emu.app/Contents/Frameworks/
        fi
    done
    return ${final}
}

updatelibrpath() {
    for lib in $(otool -L ${1} | grep "/opt/local" | awk '{ print $1 }' | sort | uniq); do
        basename=$(basename ${lib})
        install_name_tool -change "${lib}" "@rpath/${basename}" "${1}"
    done
    install_name_tool -add_rpath "@executable_path/../Frameworks" "${1}" 
}

rm -rf ep128emu.app/Contents/Frameworks
mkdir -p ep128emu.app/Contents/Frameworks

for executable in ep128emu.app/Contents/MacOS/*; do
    collectlibraries ${executable}
    updatelibrpath ${executable}
done

while [ true ]; do
    final=0
    for library in ep128emu.app/Contents/Frameworks/*; do
        collectlibraries ${library}
        final=$?
        updatelibrpath ${library}
    done
    if [ ${final} -eq 0 ]; then
        break
    fi
done

chmod 0755 ep128emu.app/Contents/Frameworks/*
