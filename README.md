# simple-file-system
A simple hierarchical filesystem with storage only in main memory implemented in standard C99

For testing purposes, anyone can uncomment rows 573-575 of the source code and modify the name of the input file.

The program receives one of the following commands for each line of the input diary file, where a path indicates a generic path and an alphanumeric string of up to 255 characters.
* create <path>: The command creates an empty file, or rather without associated data, inside the filesystem. Output the "ok" result if the file was created regularly, "no" if the file creation was not successful (for example, if you try to create a file in a non-existent directory, or if the limits of the filesystem are exceeded). 
* create_dir <path>: Create an empty directory inside the filesystem. Print the outcome "ok" if the directory has been created regularly, "no" if the creation was not successful. 
* read <path>: Reads the entire contents of a file, printing in "content" output followed by a space character and the content of the file if the file exists or prints "no" if the file does not exist. 
* write <path> <contenuto>: Writes, in full, the content of a file; output "ok" followed by the number of characters written if the writing is successful, "no" otherwise. The <contenuto> parameter takes the form of a sequence of alphanumeric characters and spaces delimited by double-quotes. Example: write /poems/jabberwocky "It was brilliant and the slithy toves" 
* delete <percorso>: Delete a resource, print the result ("ok" "no"). A resource is erasable only when it has no children. 
* delete_r <percorso>: Delete a resource and all its children if present. Print the result ("ok" - "no").
* find <name>: Search for the location of the <name> resource within the filesystem. Output print "ok" followed by the path of the resource for each resource with the correct name found or "no" if no resource with the given name is present. 
* exit: Ends the execution of the resource manager. Does not print anything in output.
  
