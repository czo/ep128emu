#!/bin/bash

BASE_DIR="$HOME/Library/Application Support/ep128emu"
MAKECFG="./epmakecfg"

if ( ! [ -e "$BASE_DIR/config/ep128uk/EP_640k_EXOS24_SDEXT_utils.cfg" ] ) ; then
  "$MAKECFG" -f "$BASE_DIR" ;
else
  "$MAKECFG" "$BASE_DIR" ;
fi

