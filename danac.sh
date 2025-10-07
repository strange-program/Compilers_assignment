#!/usr/bin/env bash

optimize=0
outfile="a.out"
mode="file"   # μπορεί να είναι: file, f, i

while getopts "Oo:fi" opt; do
  case $opt in
    O) optimize=1 ;;
    o) outfile="$OPTARG" ;;
    f) mode="f" ;;  # πρόγραμμα από stdin, έξοδος τελικού κώδικα
    i) mode="i" ;;  # πρόγραμμα από stdin, έξοδος ενδιάμεσου κώδικα
    \?) echo "Invalid option: -$OPTARG" >&2; exit 1 ;;
    :) echo "Option -$OPTARG requires an argument." >&2; exit 1 ;;
  esac
done

shift $((OPTIND -1))

if [[ "$mode" == "f" || "$mode" == "i" ]]; then
  # Διαβάζουμε από stdin
  tmp_src=$(mktemp /tmp/danaXXXXXX.dana)
  cat > "$tmp_src"

  tmp_ir=$(mktemp /tmp/danaXXXXXX.imm)
  tmp_as=$(mktemp /tmp/danaXXXXXX.asm)

  if ! ./danac $optimize < "$tmp_src" > "$tmp_ir"; then
    rm -f "$tmp_src" "$tmp_ir" "$tmp_as"
    exit 1
  fi

  if [[ "$mode" == "i" ]]; then
    cat "$tmp_ir"
    rm -f "$tmp_src" "$tmp_ir" "$tmp_as"
    exit 0
  fi

  # mode=f → παράγουμε τελικό κώδικα
  if ! llc "$tmp_ir" -o "$tmp_as"; then
    rm -f "$tmp_src" "$tmp_ir" "$tmp_as"
    exit 1
  fi

  cat "$tmp_as"
  rm -f "$tmp_src" "$tmp_ir" "$tmp_as"
  exit 0
else
  # Κανονική περίπτωση με αρχείο
  input_file=$1
  if [[ -z "$input_file" ]]; then
    echo "No input file provided" >&2
    exit 1
  fi

  input_name="${input_file%.*}"
  ir_file="$input_name.imm"
  as_file="$input_name.asm"

  if ! ./danac $optimize < "$input_file" > "$ir_file"; then
    exit 1
  fi

  if ! llc "$ir_file" -o "$as_file"; then
    exit 1
  fi

  if ! clang -no-pie -o "$outfile" "$as_file" lib.a; then
    exit 1
  fi

  exit 0
fi
