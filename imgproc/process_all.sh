#!/bin/zsh

# Set nullglob so the loop doesn't run if no PNG files exist
setopt nullglob

# Process all PNG files in the current directory
files=(*.png)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No PNG files found in current directory"
    exit 0
fi

for file in "${files[@]}"; do
    echo "Processing: $file"
    imgproc -a renamesize -i "$file"
done

echo "All PNG files processed"
