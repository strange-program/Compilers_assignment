#!/usr/bin/env bash

optimize=0
outfile="a.out"

while getopts "Oo:" opt; do
  case $opt in
    O) optimize=1 ;;
    o) outfile="$OPTARG" ;;
    \?) echo "Invalid option: -$OPTARG" >&2; exit 1 ;;
    :) echo "Option -$OPTARG requires an argument." >&2; exit 1 ;;
  esac
done

shift $((OPTIND -1))

input_file=$1
input_name="${input_file%.dana}"
ir_file="$input_name.imm"
as_file="$input_name.asm"

./danac $optimize < $input_file > $ir_file
llc $ir_file -o $as_file
clang -o $outfile $as_file lib.a
