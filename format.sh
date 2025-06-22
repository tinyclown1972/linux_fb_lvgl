#!/bin/bash

# Simplified clang-format formatting script

# Configuration variables - modify these values as needed
TARGET_DIRS=("src" "include" "drivers/fb_display")  # Directories to format
SINGLE_FILES=()  # Individual files to format
FILE_EXTENSIONS=("c" "cpp" "h" "hpp" "cc" "hh")  # File extensions to process

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed. Please install clang-format first."
    echo "On Ubuntu you can use: sudo apt install clang-format"
    exit 1
fi

# Collect all files to format
all_files=()

# 1. Add files from directories
for dir in "${TARGET_DIRS[@]}"; do
    # Ensure directory exists
    if [ ! -d "$dir" ]; then
        echo "Warning: Directory does not exist - $dir"
        continue
    fi

    # Find files with matching extensions
    for ext in "${FILE_EXTENSIONS[@]}"; do
        while IFS= read -r -d $'\0' file; do
            all_files+=("$file")
        done < <(find "$dir" -type f -name "*.$ext" -print0)
    done
done

# 2. Add individually specified files
for file in "${SINGLE_FILES[@]}"; do
    # Ensure file exists
    if [ ! -f "$file" ]; then
        echo "Warning: File does not exist - $file"
        continue
    fi

    # Check file extension
    filename=$(basename -- "$file")
    extension="${filename##*.}"

    if [[ " ${FILE_EXTENSIONS[*]} " =~ " ${extension} " ]]; then
        all_files+=("$file")
    else
        echo "Skipping unsupported extension: $file"
    fi
done

# 3. Deduplicate file list
readarray -t unique_files < <(printf "%s\n" "${all_files[@]}" | sort -u)

# 4. Format files
total_files=${#unique_files[@]}
if [ "$total_files" -eq 0 ]; then
    echo "No files found to format"
    exit 0
fi

echo "Starting to format $total_files files..."
echo "##################################################################################"
counter=0

for file in "${unique_files[@]}"; do
    ((counter++))
    echo "Formatting [$counter/$total_files]: $file"
    clang-format -i --style=file "$file"
done

echo "##################################################################################"
echo "Formatting complete! Processed $counter files"
