#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 0;
    }

    file_list_t files;
    file_list_init(&files);

    // TODO: Parse command-line arguments and invoke functions from 'minitar.h'
    // to execute archive ops
    char *op = argv[1];
    char *archive_name = argv[3];

    for (int i = 4; i < argc; i++) {
        file_list_add(&files, argv[i]);
    }

    if (strcmp(op, "-c") == 0) {
        if (!files.head) {
            printf("Error: No files given to archive\n");
            file_list_clear(&files);
            return 1;
        }
        if (create_archive(archive_name, &files) == -1) {
            printf("Error: Failed to create archive\n");
            file_list_clear(&files);
            return 1;
        }

    } else if (strcmp(op, "-a") == 0) {
        if (!files.head) {
            printf("Error: No files provided to append.\n");
            file_list_clear(&files);
            return 1;
        }
        if (append_files_to_archive(archive_name, &files) == -1) {
            printf("Error: Failed to append files to archive.\n");
            file_list_clear(&files);
            return 1;
        }

    } else if (strcmp(op, "-t") == 0) {
        file_list_t archive_files;
        file_list_init(&archive_files);

        if (get_archive_file_list(archive_name, &archive_files) == -1) {
            printf("Error: Failed to read archive '%s'.\n", archive_name);
            file_list_clear(&archive_files);    // Free memory
            return 1;
        }

        printf("Files in archive '%s':\n", archive_name);    // Corrected printf statement
        node_t *current = archive_files.head;
        while (current != NULL) {
            printf("  %s\n", current->name);
            current = current->next;
        }

        file_list_clear(&archive_files);
    } else if (strcmp(op, "-u") == 0) {
        if (!files.head) {
            printf("Error: No files provided to update.\n");
            file_list_clear(&files);
            return 1;
        }

        file_list_t archive_files;
        file_list_init(&archive_files);

        if (get_archive_file_list(archive_name, &archive_files) == -1) {
            printf("Error: Failed to read archive '%s'.\n", archive_name);
            file_list_clear(&files);
            return 1;
        }

        if (!file_list_is_subset(&files, &archive_files)) {
            printf(
                "Error: One or more of the specified files is not already present in archive.\n");
            file_list_clear(&archive_files);
            return 1;
        }

        if (append_files_to_archive(archive_name, &files) == -1) {
            printf("Error: Failed to update files in archive '%s'.\n", archive_name);
            file_list_clear(&archive_files);
            return 1;
        }

        printf("Files updated successfully.\n");
        file_list_clear(&archive_files);
    } else if (strcmp(op, "-x") == 0) {
        if (extract_files_from_archive(archive_name) == -1) {
            printf("Error: Failed to extract files from archive '%s'.\n", archive_name);
            return 1;
        }
        printf("Files extracted successfully.\n");
    } else {
        printf("Unknown command, try again\n");
        file_list_clear(&files);
        return 1;
    }

    file_list_clear(&files);
    return 0;
}
