// gsheetfmt.go: A Go program to read a Google Sheet and format it as HTML.
// The sheet is expected to have a structure where the first column contains room names,
// and subsequent columns contain item descriptions and a "yes" or "no" status.
// The program outputs an HTML table with room names and items marked as "yes".
// It uses the Google Sheets API and OAuth2 for authentication.
//
// Mark Riordan (and Github Copilot)  2025-06-21
package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strings"

	"golang.org/x/oauth2"
	"golang.org/x/oauth2/google"
	"google.golang.org/api/option"
	"google.golang.org/api/sheets/v4"
)

func getTokenFromWeb(config *oauth2.Config) *oauth2.Token {
	authURL := config.AuthCodeURL("state-token", oauth2.AccessTypeOffline)
	fmt.Fprintf(os.Stderr, "Go to the following link in your browser then type the authorization code embedded in the redirect: \n%v\n", authURL)
	var authCode string
	if _, err := fmt.Scan(&authCode); err != nil {
		log.Fatalf("Unable to read authorization code: %v", err)
	}
	tok, err := config.Exchange(context.TODO(), authCode)
	if err != nil {
		log.Fatalf("Unable to retrieve token from web: %v", err)
	}
	return tok
}

func tokenFromFile(file string) (*oauth2.Token, error) {
	f, err := os.Open(file)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	tok := &oauth2.Token{}
	err = json.NewDecoder(f).Decode(tok)
	return tok, err
}

func saveToken(path string, token *oauth2.Token) {
	f, err := os.Create(path)
	if err != nil {
		log.Fatalf("Unable to cache oauth token: %v", err)
	}
	defer f.Close()
	json.NewEncoder(f).Encode(token)
}

// getClient handles the OAuth2 flow and token caching.
func getClient(config *oauth2.Config) *http.Client {
	const tokenFile = "token.json"
	tok, err := tokenFromFile(tokenFile)
	if err != nil {
		tok = getTokenFromWeb(config)
		saveToken(tokenFile, tok)
	}
	return config.Client(context.Background(), tok)
}

func printUsage() {
	fmt.Println("Usage: gsheetfmt -action:{convert|invert}")
}

func parseCmdLine() (string, error) {
	if len(os.Args) < 2 {
		printUsage()
		return "", fmt.Errorf("no action specified")
	}
	arg := os.Args[1]
	if strings.HasPrefix(arg, "-action:") {
		action := strings.TrimPrefix(arg, "-action:")
		if action == "" {
			printUsage()
			return "", fmt.Errorf("no action specified after -action:")
		}
		if action != "convert" && action != "invert" {
			printUsage()
			return "", fmt.Errorf("invalid action specified: %s", action)
		}
		return action, nil
	}
	printUsage()
	return "", fmt.Errorf("invalid argument: %s", arg)
}

func convert(srv *sheets.Service) {
	spreadsheetId := "1q147i9CqoCmdJu-KlRanC5tRcz4qG6MVCqGYdQ4ie8g"
	readRange := "Sheet1!A2:D" // Adjust as needed
	resp, err := srv.Spreadsheets.Values.Get(spreadsheetId, readRange).Do()
	if err != nil {
		log.Fatalf("Unable to retrieve data from sheet: %v", err)
	}
	//fmt.Println("Successfully retrieved data from the spreadsheet")

	fmt.Print("<table>\n")
	if len(resp.Values) == 0 {
		fmt.Fprintln(os.Stderr, "No data found.")
	} else {
		nItemsThisRoom := 0
		curRoomHTML := ""
		curRoomName := ""
		for _, row := range resp.Values {
			numCols := len(row)
			// fmt.Printf("Processing row with %d columns; len(row[0].(string)): %d val=%v\n",
			// 	numCols, len(row[0].(string)), row[0].(string))
			if numCols == 0 {
				fmt.Fprintf(os.Stderr, "Error: Empty row encountered\n")
			} else if len(row[0].(string)) > 0 {
				// Non-blank first column indicates a new room
				if nItemsThisRoom > 0 {
					// If we have items from the previous room, print them
					fmt.Printf("<tr><td colspan=\"2\"><b>%v</b></td></tr>\n", curRoomName)
					fmt.Printf("%s\n", curRoomHTML)
				}
				curRoomName = row[0].(string)
				nItemsThisRoom = 0
				curRoomHTML = ""
			} else {
				// fmt.Printf("Processing item in room: %s\n", curRoomName)
				desc := row[1].(string)
				yesNo := row[2].(string)
				// fmt.Printf("This row has desc %s and yesno=%v\n", desc, yesNo)
				if yesNo == "y" {
					curRoomHTML += fmt.Sprintf("<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>%s</td></tr>\n", desc)
					nItemsThisRoom++
				} else if yesNo == "n" {
					// Do nothing for "n"
				} else {
					fmt.Fprintf(os.Stderr, "Error: %v\n", row)
				}
			}
		}
		if nItemsThisRoom > 0 {
			// If we have items from the previous room, print them
			fmt.Printf("<tr><td colspan=\"2\"><b>%v</b></td></tr>\n", curRoomName)
			fmt.Printf("%s\n", curRoomHTML)
		}
		fmt.Print("</table>\n")
		// strrow := ""
		// for i, cell := range row {
		// 	if strrow != "" {
		// 		strrow += ", "
		// 	}
		// 	strrow += fmt.Sprintf("Column %d: %v", i, cell)
		// }
		// mt.Println(strrow)
	}
}

// contains checks if a slice of strings contains a specific string.
func contains(slice []string, str string) bool {
	for _, v := range slice {
		if v == str {
			return true
		}
	}
	return false
}

func invert(srv *sheets.Service) {
	spreadsheetId := "1hHcEJM6sibaGwL7itIQAZVjma1_T2nJUQ-up5pXvrCU"
	readRange := "Sheet1!A2:G"
	resp, err := srv.Spreadsheets.Values.Get(spreadsheetId, readRange).Do()
	if err != nil {
		log.Fatalf("Unable to retrieve data from sheet: %v", err)
	}
	//fmt.Println("Successfully retrieved data from the spreadsheet")

	fmt.Print("<table>\n")
	if len(resp.Values) == 0 {
		fmt.Println("No data found.")
	} else {
		// Gather unique destination room names
		var destRooms []string
		var needToDecide []string
		// Create map of room name to array of items
		roomItems := make(map[string][]string)
		for _, row := range resp.Values {
			if len(row) > 0 {
				houseFrom := row[0].(string)
				roomFrom := row[1].(string)
				if houseFrom == "Applewood" {
					if !contains(destRooms, roomFrom) {
						destRooms = append(destRooms, roomFrom)
						roomItems[roomFrom] = []string{}
					}
				}
			}
		}
		//fmt.Printf("Unique destination rooms: %v\n\n", destRooms)

		// Iterate over the rows again to collect items for each destination room
		for _, row := range resp.Values {
			nColumns := len(row)
			if nColumns >= 4 {
				houseFrom := row[0].(string)
				roomFrom := row[1].(string)
				item := row[2].(string)
				yesNo := row[3].(string)
				if yesNo == "y" {
					if nColumns >= 6 {
						roomTo := row[5].(string)
						if roomTo == "same" {
							roomTo = row[1].(string) // Use the original room if "same"
						} else {
							item = fmt.Sprintf("%s (from %s %s)", item, houseFrom, roomFrom) // Append original house and room to item
						}
						if !contains(destRooms, roomTo) {
							fmt.Fprintf(os.Stderr, "Error: Room '%s' not found in destination rooms: %v\n", roomTo, row)
						}
						//fmt.Fprintf(os.Stderr, "Adding item '%s' to room '%s'\n", item, roomTo)
						roomItems[roomTo] = append(roomItems[roomTo], item)
					} else {
						fmt.Fprintf(os.Stderr, "Skipping row with insufficient columns: %v\n", row)
					}
				} else if yesNo != "n" {
					if len(item) > 0 {
						needToDecide = append(needToDecide, fmt.Sprintf("'%s' from %s %s", item, houseFrom, roomFrom))
					}
				}
			} else {
				fmt.Fprintf(os.Stderr, "Skipping row with %d columns: %v\n", nColumns, row)
			}
		}

		// Print an HTML table with destination rooms and their items
		for _, room := range destRooms {
			fmt.Printf("<tr><td colspan=\"2\"><b>%s</b></td></tr>\n", room)
			items := roomItems[room]
			for _, item := range items {
				fmt.Printf("<tr><td>&nbsp;&nbsp;&nbsp;&nbsp;</td><td>%s</td></tr>\n", item)
			}
		}
		fmt.Printf("</table>\n")

		fmt.Println("<p>Items that need to be decided:</p>")
		if len(needToDecide) > 0 {
			fmt.Println("<ul>")
			for _, item := range needToDecide {
				fmt.Printf("<li>%s</li>\n", item)
			}
			fmt.Println("</ul>")
		} else {
			fmt.Println("None")
		}
	}
}

func main() {
	action, err := parseCmdLine()
	if err != nil {
		log.Fatalf("Error parsing command line: %v", err)
	}
	ctx := context.Background()

	// Load credentials.json (OAuth2 client ID from Google Cloud Console)
	b, err := ioutil.ReadFile("credentials.json")
	if err != nil {
		log.Fatalf("Unable to read client secret file: %v", err)
	}
	//fmt.Println("Successfully read credentials.json")

	// If modifying these scopes, delete your previously saved token.json.
	config, err := google.ConfigFromJSON(b, sheets.SpreadsheetsReadonlyScope)
	if err != nil {
		log.Fatalf("Unable to parse client secret file to config: %v", err)
	}
	//fmt.Println("Successfully parsed client secret file to config")

	// Set a specific redirect URL with a port, but we don't actually have
	// a web server running here; we just need to inspect the redirect URL
	// for the authorization code.
	config.RedirectURL = "http://localhost:8080"
	client := getClient(config)

	srv, err := sheets.NewService(ctx, option.WithHTTPClient(client))
	if err != nil {
		log.Fatalf("Unable to retrieve Sheets client: %v", err)
	}
	//fmt.Println("Successfully created Sheets service client")
	if action == "convert" {
		convert(srv)
	} else if action == "invert" {
		invert(srv)
	} else {
		fmt.Println("Unknown action specified.")
		printUsage()
		os.Exit(1)
	}
}
