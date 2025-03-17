// Simple JavaScript script to list the files and subfolders in the current
// folder on Windows.

function Main() {
    var fso = new ActiveXObject("Scripting.FileSystemObject");
    var stdout = fso.GetStandardStream(1); // 1 is for stdout
    var folderPath = "."; // WScript.Arguments(0);

    if (!fso.FolderExists(folderPath)) {
        stdout.WriteLine("The specified folder does not exist.");
        WScript.Quit(1);
    }

    var folder = fso.GetFolder(folderPath);
    var files = new Enumerator(folder.Files);
    var subfolders = new Enumerator(folder.Subfolders);

    stdout.WriteLine("Directory listing for: " + folderPath);
    stdout.WriteLine("Subfolders:");
    while (!subfolders.atEnd()) {
        stdout.WriteLine("  " + subfolders.item().Name);
        subfolders.moveNext();
    }    stdout.WriteLine("Files:");
    while (!files.atEnd()) {
        stdout.WriteLine("  " + files.item().Name);
        files.moveNext();
    }
}

Main();
