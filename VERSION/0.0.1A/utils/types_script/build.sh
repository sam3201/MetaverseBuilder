#!/bin/bash

read -p "Enter File Name .deff: " FILE 

SRC_FILES="types_script_jit_parser.c types_script_jit.c"
OUTPUT="types_script_jit_parser"

echo "Compiling source files..."
gcc $SRC_FILES -o $OUTPUT

if [ $? -eq 0 ]; then
    echo "Compilation successful. Executable created: $OUTPUT"
    
    ./$OUTPUT $FILE output.c
    
    if [ $? -eq 0 ]; then
        echo "Execution successful."
        rm $OUTPUT
    else
        echo "Execution failed."
    fi
else
    echo "Compilation failed."
    exit 1
fi

