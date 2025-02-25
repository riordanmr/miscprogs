// procinclude.go - A simple Go program to process C/C++ source files and
// handle #include directives.
// It reads a file, processes its content, and recursively processes included files.
// It logs the opening and closing of files, and handles both double-quoted and angle-bracketed includes.
// The purpose of the program is to help diagnose problems with
// include files being included out-of-order.
// Mark Riordan 2025-02-24 with help from Microsoft Copilot
package main

import (
	"bufio"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
)

var includePattern = regexp.MustCompile(`^#include\s+["<](.*?)[">]`)

func processFile(filename string, processed map[string]bool) {
	fmt.Printf("Processing %s\n", filename)
	if processed[filename] {
		return
	}
	processed[filename] = true

	file, err := os.Open(filename)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error opening file %s: %v\n", filename, err)
		return
	}
	defer file.Close()

	//fmt.Printf("Processing %s\n", filename)
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := scanner.Text()
		//fmt.Println(line)

		matches := includePattern.FindStringSubmatch(line)
		if matches != nil {
			includeFile := matches[1]
			fmt.Printf("Found include: %s in %s\n", line, filename)
			if line[9] == '"' {
				processFile(filepath.Join(filepath.Dir(filename), includeFile), processed)
			}
		}
	}

	if err := scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "Error reading file %s: %v\n", filename, err)
	}

	fmt.Printf("Closing %s\n", filename)
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: go run procinclude.go <filename>")
		return
	}

	// directory := os.Args[1]
	// // Change working directory to the specified directory
	// err := os.Chdir(directory)
	// if err != nil {
	// 	fmt.Fprintf(os.Stderr, "Error changing directory to %s: %v\n", directory, err)
	// 	return
	// }
	filename := os.Args[1]
	processed := make(map[string]bool)
	processFile(filename, processed)
}
