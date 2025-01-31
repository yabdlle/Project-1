#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"    // Include your minitar functions

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 1;
    }

    file_list_t files;
    file_list_init(&files);

    char *operation = argv[1];    // -c, -a, -t, -u, or -x
    char *archive_name = NULL;
    int file_arg_start = 0;

    // Perform the requested operation
    if (strcmp(operation, "-c") == 0) {
        // Create a new archive
        if (create_archive(archive_name, &files) != 0) {
            fprintf(stderr, "Error: Could not create the archive '%s'.\n", archive_name);
        }
    } else if (strcmp(operation, "-a") == 0) {
        // Append files to an existing archive
        if (append_files_to_archive(archive_name, &files) != 0) {
            fprintf(stderr, "Error: Could not append files to the archive '%s'.\n", archive_name);
        }
    } else if (strcmp(operation, "-t") == 0) {
        // List the contents of the archive
        if (list_archive_contents(archive_name) != 0) {
            fprintf(stderr, "Error: Could not list the contents of the archive '%s'.\n",
                    archive_name);
        }
    } else if (strcmp(operation, "-u") == 0) {
        // Update files in the archive
        if (update_archive(archive_name, &files) != 0) {
            fprintf(stderr, "Error: Could not update the archive '%s'.\n", archive_name);
        }
    } else if (strcmp(operation, "-x") == 0) {
        // Extract files from the archive
        if (extract_files_from_archive(archive_name) != 0) {
            fprintf(stderr, "Error: Could not extract files from the archive '%s'.\n",
                    archive_name);
        }
    } else {
        // Invalid operation
        fprintf(stderr, "Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        file_list_clear(&files);
        return 1;
    }

    file_list_clear(&files);    // Free any memory associated with the file list
    return 0;
}
