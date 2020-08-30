#!/bin/sh -eux

platformio ci $PWD/examples/$BOARDTYPE$EXAMPLE_FOLDER$EXAMPLE_NAME/ --lib="." --board=$BOARD
