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

	"golang.org/x/oauth2"
	"golang.org/x/oauth2/google"
	"google.golang.org/api/option"
	"google.golang.org/api/sheets/v4"
)

func getTokenFromWeb(config *oauth2.Config) *oauth2.Token {
	authURL := config.AuthCodeURL("state-token", oauth2.AccessTypeOffline)
	fmt.Printf("Go to the following link in your browser then type the authorization code: \n%v\n", authURL)
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

func main() {
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

	// Set a specific redirect URL with a port if you want to avoid conflicts
	config.RedirectURL = "http://localhost:8080"
	client := getClient(config)

	srv, err := sheets.NewService(ctx, option.WithHTTPClient(client))
	if err != nil {
		log.Fatalf("Unable to retrieve Sheets client: %v", err)
	}
	//fmt.Println("Successfully created Sheets service client")

	spreadsheetId := "1q147i9CqoCmdJu-KlRanC5tRcz4qG6MVCqGYdQ4ie8g"
	readRange := "Sheet1!A2:D" // Adjust as needed
	resp, err := srv.Spreadsheets.Values.Get(spreadsheetId, readRange).Do()
	if err != nil {
		log.Fatalf("Unable to retrieve data from sheet: %v", err)
	}
	//fmt.Println("Successfully retrieved data from the spreadsheet")

	fmt.Print("<table>\n")
	if len(resp.Values) == 0 {
		fmt.Println("No data found.")
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
					fmt.Printf("<tr><td colspan=\"2\">%v</td></tr>\n", curRoomName)
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
			fmt.Printf("<tr><td colspan=\"2\">%v</td></tr>\n", curRoomName)
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
