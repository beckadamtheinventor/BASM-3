#!/bin/sh

mkdir bin

echo "Building BASM include files"

fasmg src/BASM_30.asm bin/BASM30.8xv

read -p ""

