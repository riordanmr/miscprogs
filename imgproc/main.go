// main.go - A command-line tool named imgproc to perform unusual processing
// on image files.
// For example, rename image files based on their dimensions and aspect ratios.
// The purpose is to support my workflow for creating a graphic novel.
//
// Mark Riordan  2025-12-11; credit to Github Copilot for much of the code.

package main

import (
	"flag"
	"fmt"
	"image"
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"os"
	"path/filepath"
	"strings"
)

func usage() {
	fmt.Fprintf(os.Stderr, "Usage: imgproc -a action -i inputfilename [-t]\n")
	fmt.Fprintf(os.Stderr, "  -a action: Action to perform (e.g., renamesize)\n")
	fmt.Fprintf(os.Stderr, "  -i inputfilename: Input image filename\n")
	fmt.Fprintf(os.Stderr, "  -t: Test mode - don't actually rename files\n")
}

func main() {
	// Define command line flags
	action := flag.String("a", "", "Action to perform (e.g., renamesize)")
	inputFile := flag.String("i", "", "Input image filename")
	testMode := flag.Bool("t", false, "Test mode - don't actually rename files")
	flag.Parse()

	// Validate inputs
	if *action == "" {
		fmt.Fprintln(os.Stderr, "Error: -a action parameter is required")
		usage()
		os.Exit(1)
	}
	if *inputFile == "" {
		usage()
		fmt.Fprintln(os.Stderr, "Error: -i inputfilename parameter is required")
		os.Exit(1)
	}

	// Handle different actions
	switch *action {
	case "renamesize":
		handleRenameSize(*inputFile, *testMode)
	default:
		fmt.Fprintf(os.Stderr, "Error: unknown action '%s'\n", *action)
		os.Exit(1)
	}
}

func handleRenameSize(filename string, testMode bool) {
	// Open the image file
	file, err := os.Open(filename)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error opening file: %v\n", err)
		os.Exit(1)
	}

	// Decode image to get dimensions
	config, _, err := image.DecodeConfig(file)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error decoding image: %v\n", err)
		os.Exit(1)
	}

	file.Close() // Close and reopen to reset read pointer

	// Extract dimensions
	x := config.Width
	y := config.Height

	// Define acceptable aspect ratios
	acceptableRatios := [][2]int{
		{21, 9}, {16, 9}, {5, 4}, {4, 3}, {3, 2}, {1, 1},
		{4, 5}, {3, 4}, {2, 3}, {9, 16},
	}

	// Find closest matching aspect ratio
	actualRatio := float64(x) / float64(y)
	closestRatio := acceptableRatios[0]
	minDiff := abs(actualRatio - float64(closestRatio[0])/float64(closestRatio[1]))
	fmt.Printf("Actual image ratio: %f\n", actualRatio)

	for _, ratio := range acceptableRatios[1:] {
		ratioValue := float64(ratio[0]) / float64(ratio[1])
		//fmt.Printf("Checking ratio: %d:%d (%f)\n", ratio[0], ratio[1], ratioValue)
		diff := abs(actualRatio - ratioValue)
		if diff < minDiff {
			minDiff = diff
			closestRatio = ratio
		}
	}

	// Display results
	fmt.Printf("Image dimensions: %d x %d\n", x, y)
	fmt.Printf("Closest standard ratio: %d:%d\n", closestRatio[0], closestRatio[1])

	// Compute new filename
	newFilename := computeNewFilename(filename, closestRatio[0], closestRatio[1])

	if testMode {
		fmt.Printf("Test mode: Would rename to: %s\n", newFilename)
	} else {
		// Rename the file
		err = os.Rename(filename, newFilename)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error renaming file: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("File renamed to: %s\n", newFilename)
	}

}

// abs returns the absolute value of a float64
func abs(x float64) float64 {
	if x < 0 {
		return -x
	}
	return x
}

// computeNewFilename generates the new filename based on the aspect ratio
func computeNewFilename(filename string, ratioW, ratioH int) string {
	dir := filepath.Dir(filename)
	base := filepath.Base(filename)

	// Find the extension
	ext := filepath.Ext(base)
	nameWithoutExt := strings.TrimSuffix(base, ext)

	// If filename has "_", remove it and everything after until the extension
	if idx := strings.Index(nameWithoutExt, "_"); idx != -1 {
		nameWithoutExt = nameWithoutExt[:idx]
	}

	// Append the aspect ratio
	newName := fmt.Sprintf("%s_%d-%d%s", nameWithoutExt, ratioW, ratioH, ext)

	return filepath.Join(dir, newName)
}
