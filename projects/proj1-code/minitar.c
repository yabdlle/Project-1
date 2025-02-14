/*partners worked on this project: Abdirahman Hassan (hassa878) and Youssef Abdulle (abdul664)*/

#include "minitar.h"

#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 128
#define BLOCK_SIZE 512

// Constants for tar compatibility information
#define MAGIC "ustar"

// Constants to represent different file types
// We'll only use regular files in this project
#define REGTYPE '0'
#define DIRTYPE '5'

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *) header;
    for (int i = 0; i < sizeof(tar_header); i++) {
        sum += bytes[i];
    }
    snprintf(header->chksum, 8, "%07o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or -1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
    memset(header, 0, sizeof(tar_header));
    char err_msg[MAX_MSG_LEN];
    struct stat stat_buf;
    // stat is a system call to inspect file metadata
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    strncpy(header->name, file_name, 100);    // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%07o",
             stat_buf.st_mode & 07777);    // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%07o", stat_buf.st_uid);    // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid);       // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->uname, pwd->pw_name, 32);    // Owner name of the file, null-terminated string

    snprintf(header->gid, 8, "%07o", stat_buf.st_gid);    // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid);        // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->gname, grp->gr_name, 32);    // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%011o",
             (unsigned) stat_buf.st_size);    // File size, 0-padded octal
    snprintf(header->mtime, 12, "%011o",
             (unsigned) stat_buf.st_mtime);    // Modification time, 0-padded octal
    header->typeflag = REGTYPE;                // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6);          // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2);          // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%07o",
             major(stat_buf.st_dev));    // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%07o",
             minor(stat_buf.st_dev));    // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];

    struct stat stat_buf;
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    off_t file_size = stat_buf.st_size;
    if (nbytes > file_size) {
        file_size = 0;
    } else {
        file_size -= nbytes;
    }

    if (truncate(file_name, file_size) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        return -1;
    }
    return 0;
}

// creates a new archive files from given files
int create_archive(const char *archive_name, const file_list_t *files) {
    // opening archive in write mode, if exists fopen overwrites
    FILE *wr = fopen(archive_name, "w");
    // error check
    if (wr == NULL) {
        perror("Failed to open file");
        return -1;
    }

    node_t *cur = files->head;
    while (cur != NULL) {    // loop through files
        // skip if the cur file is the archive
        if (strcmp(cur->name, archive_name) == 0) {
            cur = cur->next;
            continue;
        }
        // open cur file to be archived
        FILE *f = fopen(cur->name, "r");
        // error check
        if (f == NULL) {
            perror("Failed to open a file");
            cur = cur->next;
            continue;
        }

        // Creating and filling the tar header
        tar_header header;
        if (fill_tar_header(&header, cur->name) != 0) {
            fclose(f);
            cur = cur->next;
            return -1;
        }
        // Writing the header to the archive (512 bytes)
        if (fwrite(&header, 1, sizeof(tar_header), wr) < 0) {
            perror("Failed to write header to file");
            fclose(f);
            fclose(wr);
            return -1;
        }

        // Writing file contents to the archive in 512-byte blocks
        char buffer[BLOCK_SIZE] = {0};    // declaring buffer to read file contents in 512 bB
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, f)) > 0) {
            if (fwrite(buffer, 1, BLOCK_SIZE, wr) !=
                BLOCK_SIZE) {    // write exactly 512 bytes to archive
                perror("Failed to write file data");
                fclose(f);
                fclose(wr);
                return -1;
            }
            memset(buffer, 0, BLOCK_SIZE);    // clears buffer after write to avoid leftover
        }
        fclose(f);
        cur = cur->next;    // on to the next file
    }

    char emptyBlock[BLOCK_SIZE] = {0};    // creating a block of 512 bytes of zeros
    if (fwrite(emptyBlock, 1, BLOCK_SIZE, wr) !=
        BLOCK_SIZE) {    // Write first empty block (TAR format requirement)
        perror("Failed to write end of archive blocks");
        fclose(wr);
        return -1;
    }
    if (fwrite(emptyBlock, 1, BLOCK_SIZE, wr) != BLOCK_SIZE) {    // Write second empty block to
        perror("Failed to write end of archive blocks");          // signal the end of the archive.
        fclose(wr);
        return -1;
    }

    if (fclose(wr) != 0) {
        perror("Failed to close archive file");
        return -1;
    }
    return 0;
}

// Append each file specified in 'files' to the archive
int append_files_to_archive(const char *archive_name, const file_list_t *files) {
    FILE *wr = fopen(archive_name, "a");
    if (wr == NULL) {
        perror("Failed to open archive");
        return -1;
    }
    if (fseek(wr, -2 * BLOCK_SIZE, SEEK_END) != 0) {    // seek the last 2 byte Blocks
        perror("Error: Failed to append files to archive.");
        fclose(wr);
        return -1;
    }

    // now remove last two byte Blocks
    if (remove_trailing_bytes(archive_name, 2 * BLOCK_SIZE) != 0) {
        perror("Failed to remove trailing bB");
        fclose(wr);
        return -1;
    }

    node_t *cur = files->head;
    while (cur != NULL) {
        if (strcmp(cur->name, archive_name) == 0) {    // if file exist, don't overwrite
            cur = cur->next;
            continue;
        }
        FILE *toRead = fopen(cur->name, "r");    // open to read each file
        if (toRead == NULL) {
            perror("Failed to read cur file");
            cur = cur->next;
            continue;
        }

        // creating and filling tar header
        tar_header header;
        if (fill_tar_header(&header, cur->name) != 0) {
            fclose(toRead);
            cur = cur->next;
            return -1;
        }

        // Writing the header to the archive (512 bytes)
        if (fwrite(&header, 1, sizeof(tar_header), wr) < 0) {
            perror("Failed to write header to file");
            fclose(toRead);
            fclose(wr);
            return -1;
        }

        // Writing file contents to the archive in 512-byte blocks
        char buffer[BLOCK_SIZE] = {0};    // declaring buffer to read file contents in 512 bB
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, BLOCK_SIZE, toRead)) > 0) {
            if (fwrite(buffer, 1, BLOCK_SIZE, wr) !=
                BLOCK_SIZE) {    // write exactly 512 bytes to archive
                perror("Failed to write file contents");
                fclose(toRead);
                fclose(wr);
                return -1;
            }
            memset(buffer, 0, BLOCK_SIZE);    // clears buffer after write to avoid leftover
        }
        fclose(toRead);
        cur = cur->next;    // on to the next file
    }

    char emptyBlock[BLOCK_SIZE] = {0};    // creating a block of 512 bytes of zeros
    if (fwrite(emptyBlock, 1, BLOCK_SIZE, wr) !=
        BLOCK_SIZE) {    // Write first empty block (TAR format requirement).
        perror("Failed to write first empty block");
        fclose(wr);
        return -1;
    }

    if (fwrite(emptyBlock, 1, BLOCK_SIZE, wr) !=
        BLOCK_SIZE) {    // Write second empty block to signal the end of the archive.
        perror("Failed to write second empty block");
        fclose(wr);
        return -1;
    }

    if (fclose(wr) != 0) {
        perror("Failed to close archive file");
        return -1;
    }
    return 0;
}

int get_archive_file_list(const char *archive_name, file_list_t *files) {
    // Check for valid arguments
    if (archive_name == NULL || files == NULL) {
        printf("Invalid args");
        return -1;
    }

    // Open the archive for reading
    FILE *archive = fopen(archive_name, "r");
    if (archive == NULL) {
        perror("Error opening archive");
        return -1;
    }

    file_list_init(files);

    while (1) {
        tar_header header;
        if (fread(&header, 1, sizeof(header), archive) != sizeof(header)) {
            break;    // End of archive
        }

        // Stop if we reach an empty header (end of archive)
        if (header.name[0] == '\0') {
            break;
        }

        // Add file name to the list
        if (file_list_add(files, header.name) != 0) {
            perror("Error adding file to list");
            fclose(archive);
            file_list_clear(files);
            return -1;
        }

        int file_size = 0;
        // Parse file size from octal format
        if (sscanf(header.size, "%o", &file_size) != 1) {
            perror("Error parsing file size");
            fclose(archive);
            file_list_clear(files);
            return -1;
        }

        // Calc the number of 512-byte blocks needed ie [n/2]
        int blocks_to_skip = (file_size + 511) / 512;

        // Skip past file contents
        if (fseek(archive, blocks_to_skip * BLOCK_SIZE, SEEK_CUR) != 0) {
            perror("Error skipping file block");
            fclose(archive);
            file_list_clear(files);
            return -1;
        }
    }

    fclose(archive);
    return 0;
}

int extract_files_from_archive(const char *archive_name) {
    if (archive_name == NULL) {
        perror("Invalid archive filename");
        return -1;
    }

    // Open the archive for reading
    FILE *archive = fopen(archive_name, "r");
    if (archive == NULL) {
        perror("Error opening archive file");
        return -1;
    }

    // Initialize the list to track extracted files
    file_list_t extracted_files;
    file_list_init(&extracted_files);

    tar_header header;

    // Iterate through each file entry in the archive
    while (fread(&header, 1, sizeof(header), archive) == sizeof(header)) {
        if (header.name[0] == '\0') {
            break;    // End of archive (null terminator)
        }

        // Read file size from header (stored in octal format)
        int file_size = 0;
        if (sscanf(header.size, "%o", &file_size) != 1) {
            perror("Error reading file size from header");
            file_list_clear(&extracted_files);
            fclose(archive);
            return -1;
        }

        // Open the output file for writing (overwrite if exists)
        FILE *out_file = fopen(header.name, "w");
        if (out_file == NULL) {
            perror("Error creating output file");
            file_list_clear(&extracted_files);
            fclose(archive);
            return -1;
        }

        char buffer[BLOCK_SIZE];
        int remaining_size = file_size;

        // Read and write file data in chunks
        while (remaining_size > 0) {
            int chunk_size;    // Ensure data fits in 512 blocks if not use remaining size
            if (remaining_size > BLOCK_SIZE) {
                chunk_size = BLOCK_SIZE;
            } else {
                chunk_size = remaining_size;    // Handle final bytes
            }

            // Read chunk from archive
            if (fread(buffer, 1, chunk_size, archive) != chunk_size) {
                perror("Error reading archive data");
                fclose(out_file);
                file_list_clear(&extracted_files);
                fclose(archive);
                return -1;
            }

            // Write chunk to output file (overwrite old version)
            if (fwrite(buffer, 1, chunk_size, out_file) != chunk_size) {
                perror("Error writing to output file");
                fclose(out_file);
                file_list_clear(&extracted_files);
                fclose(archive);
                return -1;
            }

            remaining_size -= chunk_size;
        }

        fclose(out_file);    // Close the extracted file

        // Add file to extracted list (ensuring latest version is written)
        if (file_list_add(&extracted_files, header.name) != 0) {
            perror("Error tracking extracted file");
            file_list_clear(&extracted_files);
            fclose(archive);
            return -1;
        }

        // Align archive pointer to 512-byte blocks
        int blocks_needed = (file_size + 511) / 512;    // Ensures [file_size / 512]
        int padding = (blocks_needed * 512) - file_size;

        if (fseek(archive, padding, SEEK_CUR) != 0) {
            perror("Error seeking archive");
            file_list_clear(&extracted_files);
            fclose(archive);
            return -1;
        }
    }
    file_list_clear(&extracted_files);
    fclose(archive);

    return 0;
}
