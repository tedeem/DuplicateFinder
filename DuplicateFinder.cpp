/**************************************************************************************
 * PROGRAM: JPEG Image Duplicate Detector
 * Author: Terry Deem....and AI
 * ---- I used Google Gemini to write most of this code - credit or blame AI... 
 * ---- I used a set of prompts to have Gemini develop the HASH code, recursive drectory algorithm and CSV I/O.
 * ----- Since AI wrote most of this - I'm not making any claim to the code - feel free to use it for whatever you want
 * ----- Any license concerns are the AI's problem and not mine. This code was developed for personal use only. 
 * ----- If you or future AI's use it - I take no responsiblity.
 * ----- Motivation for the program:
 * ----- I have tons of family photos as do the rest of my family. Over time, and several attempts to organize them, 
 * ----- the photos have duplicated but the names might have changed. So I needed something that would look at all the photos
 * ----- on my disk and mark which ones were a duplicate to another. I could use machine learning or an AI model to do this. 
 * ----- But I wanted to go old school and use a hash instead. I wanted it to be small and quick too. I prompted the AI to 
 * ----- create a c++ program that would use open source techniques and static libs to make a simple EXE file. I planned to let
 * ----- my family use it and didn't want a bunch of DLL's to have to be shipped. I also didn't want inadvertanly delete or harm 
 * ----- the images in case I made a mistake in the code - so I just prompted the AI to be careful to not harm any file and create CSV file of the results. 
 * ----- After running the original program I noticed that the same image at different resolutions wouldn't be considered a duplicate. 
 * ----- Also the Disk I/O was annoying so I prompted the AI to rework the program to normalize all the images to the same resolution but
 * ----- not write it or replace the image...just keep it memory and use system memory for processing.
 * ----- For you professional coders out there - there are likely bugs or improvements you code make - feel free to let me know or implement.
 * ----- Works great - after scanning the directories you will get a CSV file, use Excel to look at it and find your duplicates.
 *
 * License: Public Domain / Unlicense. Feel free to fork, refactor, optimize, or integrate this code into any project.
 * Warranty: Provided "as is" without warranty of any kind. Take proper precautions (such as backing up your data) 
 *           when executing custom utilities over your storage drives. The authors assume no liability for bugs, 
 *           data loss, or system instability.
 * 
 * * DESCRIPTION:
 * This program recursively scans a target directory (starting from the current directory)
 * to catalog JPEG images (`.jpg`, `.jpeg`) and identify duplicates based on image content.
 * To ensure accuracy regardless of minor file variations, it loads each image, normalizes
 * it to a uniform 512x512 resolution using a nearest-neighbor algorithm, and generates
 * a custom MD5 hash of the raw pixel data. It then cross-references these hashes to
 * spot identical images.
 * * HOW IT WORKS:
 * 1. Directory Traversal: Uses the Windows API to recursively search folders.
 * 2. Image Normalization: Decodes and scales JPEGs to a standard 512x512 buffer.
 * 3. In-Memory Hashing: Computes an MD5 signature directly from RAM to eliminate disk I/O overhead.
 * 4. Cross-Matching: Compares signatures to spot duplicates and logs everything to a CSV.
 * * OUTPUT:
 * Generates a spreadsheet named 'imageDB.csv' containing the following columns:
 * - FilePath: The absolute or relative path to the scanned image.
 * - Hash: The 32-character hexadecimal MD5 fingerprint of the normalized image.
 * - DuplicateOf: Comma-separated 1-based CSV row numbers indicating subsequent duplicates.
 * * DEPENDENCIES & ENVIRONMENT:
 * - OS: Microsoft Windows (Relies on <windows.h> for file system navigation).
 * - Libraries: Uses the header-only `stb_image.h` library for image decoding.
 * No external software installations or complex linker setups are required.
 *************************************************************************************/

#define _CRT_SECURE_NO_WARNINGS // Suppress warnings for deprecated C runtime functions like fopen, localtime

#include <stdio.h>    // For standard input/output operations (fopen_s, fprintf, printf, perror)
#include <stdlib.h>   // For general utilities (malloc, free, exit)
#include <string.h>   // For string manipulation (strlen, strcpy, strrchr, strcmp, snprintf, _stricmp, memset)
#include <windows.h>  // For Windows-specific API functions (FindFirstFileA, FindNextFile, FindClose, GetFileAttributesExA)
#include <time.h>     // For time manipulation (time) - mktime and localtime_s no longer needed for file date
#include <io.h>       // For _access_s, used in file path checking (though not strictly necessary here, but common for Windows)

// --- STB Image Libraries Integration ---
// These are single-file, header-only libraries. By defining the IMPLEMENTATION macros,
// their source code is included and compiled directly into this file.
// This fulfills the requirement of not needing external installations.

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Available from https://github.com/nothings/stb/blob/master/stb_image.h

// --- End STB Image Libraries Integration ---


// Define a maximum path length to prevent buffer overflows
#define MAX_PATH_LEN 1024
// Define the desired temporary image resolution
#define TEMP_IMG_SIZE 512
// Define the MD5 hash length (32 hex characters + null terminator)
#define MD5_HASH_LEN 33

// --- MD5 Hashing Implementation (Simplified for demonstration) ---
// This is a basic, simplified MD5 implementation. For production,
// consider using a well-tested library like OpenSSL.
// It is primarily for demonstrating the concept of content hashing,
// not for cryptographic security.

typedef unsigned int MD5_UINT32; // 32-bit unsigned integer

// Basic MD5 functions
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// Left Rotate (ROL)
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// Round operations (simplified definitions for clarity)
// These macros apply the MD5 transformation functions and bitwise operations
#define FF(a, b, c, d, x, s, ac) \
    (a) += F((b), (c), (d)) + (x) + (MD5_UINT32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b);

#define GG(a, b, c, d, x, s, ac) \
    (a) += G((b), (c), (d)) + (x) + (MD5_UINT32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b);

#define HH(a, b, c, d, x, s, ac) \
    (a) += H((b), (c), (d)) + (x) + (MD5_UINT32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b);

#define II(a, b, c, d, x, s, ac) \
    (a) += I((b), (c), (d)) + (x) + (MD5_UINT32)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b);

// Initial MD5 hash values (A, B, C, D)
static MD5_UINT32 MD5_INIT_STATE[4] = {
    0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476
};

// Function to compute MD5 hash of an in-memory buffer
// Returns a dynamically allocated string containing the 32-character hex hash, or NULL on error
char* calculate_memory_md5(const unsigned char* data, size_t data_len) {
    MD5_UINT32 a = MD5_INIT_STATE[0];
    MD5_UINT32 b = MD5_INIT_STATE[1];
    MD5_UINT32 c = MD5_INIT_STATE[2];
    MD5_UINT32 d = MD5_INIT_STATE[3];

    size_t offset = 0;
    unsigned char buffer[64]; // 64 bytes (512 bits) block

    // Process data in 64-byte chunks
    while (offset < data_len) {
        size_t bytes_to_read = 64;
        if (offset + bytes_to_read > data_len) {
            bytes_to_read = data_len - offset;
        }

        memcpy(buffer, data + offset, bytes_to_read);

        // Pad the last block if it's less than 64 bytes (simplified: zero-pad)
        if (bytes_to_read < 64) {
            memset(buffer + bytes_to_read, 0, 64 - bytes_to_read);
        }

        MD5_UINT32 X[16];
        int i;
        // Convert 64 bytes to 16 32-bit words (Little-endian)
        for (i = 0; i < 16; i++) {
            X[i] = (MD5_UINT32)buffer[i * 4] |
                ((MD5_UINT32)buffer[i * 4 + 1] << 8) |
                ((MD5_UINT32)buffer[i * 4 + 2] << 16) |
                ((MD5_UINT32)buffer[i * 4 + 3] << 24);
        }

        // Save A, B, C, D for this block
        MD5_UINT32 A_prev = a;
        MD5_UINT32 B_prev = b;
        MD5_UINT32 C_prev = c;
        MD5_UINT32 D_prev = d;

        // MD5 Rounds
        FF(a, b, c, d, X[0], 7, 0xd76aa478); FF(d, a, b, c, X[1], 12, 0xe8c7b756); FF(c, d, a, b, X[2], 17, 0x242070db); FF(b, c, d, a, X[3], 22, 0xc1bdceee);
        FF(a, b, c, d, X[4], 7, 0xf57c0faf); FF(d, a, b, c, X[5], 12, 0x4787c62a); FF(c, d, a, b, X[6], 17, 0xa8304613); FF(b, c, d, a, X[7], 22, 0xfd469501);
        FF(a, b, c, d, X[8], 7, 0x698098d8); FF(d, a, b, c, X[9], 12, 0x8b44f7af); FF(c, d, a, b, X[10], 17, 0xffff5bb1); FF(b, c, d, a, X[11], 22, 0x895cd7be);
        FF(a, b, c, d, X[12], 7, 0x6b901122); FF(d, a, b, c, X[13], 12, 0xfd987193); FF(c, d, a, b, X[14], 17, 0xa679438e); FF(b, c, d, a, X[15], 22, 0x49b40821);

        GG(a, b, c, d, X[1], 5, 0xf61e2562); GG(d, a, b, c, X[6], 9, 0xc040b340); GG(c, d, a, b, X[11], 14, 0x265e5a51); GG(b, c, d, a, X[0], 20, 0xe9b6c7aa);
        GG(a, b, c, d, X[5], 5, 0xd62f105d); GG(d, a, b, c, X[10], 9, 0x02441453); GG(c, d, a, b, X[15], 14, 0xd8a1e681); GG(b, c, d, a, X[4], 20, 0xe7d3fbc8);
        GG(a, b, c, d, X[9], 5, 0x21e1cde6); GG(d, a, b, c, X[14], 9, 0xc33707d6); GG(c, d, a, b, X[3], 14, 0xf4d50d87); GG(b, c, d, a, X[8], 20, 0x455a14ed);
        GG(a, b, c, d, X[13], 5, 0xa9e3e905); GG(d, a, b, c, X[2], 9, 0xfcefa3f8); GG(c, d, a, b, X[7], 14, 0x676f02d9); GG(b, c, d, a, X[12], 20, 0x8d2a4c8a);

        HH(a, b, c, d, X[5], 4, 0xfffa3942); HH(d, a, b, c, X[8], 11, 0x8771f681); HH(c, d, a, b, X[11], 16, 0x6d9d6122); HH(b, c, d, a, X[14], 23, 0xfde5380c);
        HH(a, b, c, d, X[1], 4, 0xa4beea44); HH(d, a, b, c, X[4], 11, 0x4bdecfa9); HH(c, d, a, b, X[7], 16, 0xf6bb4b60); HH(b, c, d, a, X[10], 23, 0xbebfbc70);
        HH(a, b, c, d, X[13], 4, 0x289b7ec6); HH(d, a, b, c, X[0], 11, 0xeaa127fa); HH(c, d, a, b, X[3], 16, 0xd4ef3085); HH(b, c, d, a, X[6], 23, 0x04881d05);
        HH(a, b, c, d, X[9], 4, 0xd9d4d039); HH(d, a, b, c, X[12], 11, 0xe6db99e5); HH(c, d, a, b, X[15], 16, 0x1fa27cf8); HH(b, c, d, a, X[2], 23, 0xc4ac5665);

        II(a, b, c, d, X[0], 6, 0xf4292244); II(d, a, b, c, X[7], 10, 0x432aff97); II(c, d, a, b, X[14], 15, 0xab9423a7); II(b, c, d, a, X[5], 21, 0xfc93a039);
        II(a, b, c, d, X[12], 6, 0x655b59c3); II(d, a, b, c, X[3], 10, 0x8f0ccc92); II(c, d, a, b, X[10], 15, 0xffeff47d); II(b, c, d, a, X[1], 21, 0x85845dd1);
        II(a, b, c, d, X[8], 6, 0x6fa87e4f); II(d, a, b, c, X[15], 10, 0xfe2ce6e0); II(c, d, a, b, X[6], 15, 0xa3014314); II(b, c, d, a, X[13], 21, 0x4e0811a1);
        II(a, b, c, d, X[4], 6, 0xf7537e82); II(d, a, b, c, X[11], 10, 0xbd3af235); II(c, d, a, b, X[2], 15, 0x2ad7d2bb); II(b, c, d, a, X[9], 21, 0xeb86d391);

        a += A_prev;
        b += B_prev;
        c += C_prev;
        d += D_prev;

        offset += 64;
    }

    // Format the hash into a string
    char* hash_str = (char*)malloc(MD5_HASH_LEN);
    if (hash_str == NULL) {
        perror("Error allocating memory for hash string");
        return NULL;
    }
    // Print in little-endian byte order (as per MD5 standard)
    snprintf(hash_str, MD5_HASH_LEN, "%08x%08x%08x%08x", a, b, c, d);
    return hash_str;
}


// --- End MD5 Hashing Implementation ---


// Structure to hold image information for duplicate detection
typedef struct {
    char filepath[MAX_PATH_LEN];
    char hash[MD5_HASH_LEN];
    // char mod_date[80]; // Removed: Modification date is no longer needed
    int index;         // Original index in the list
} ImageEntry;

// Function to check if a file has a JPEG extension
// Returns 1 if it's a JPEG, 0 otherwise
int is_jpeg(const char* filename) {
    // Find the last occurrence of '.' to get the file extension
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) { // No dot found or dot is the first character
        return 0;
    }
    // Compare the extension (case-insensitive) using _stricmp for Windows
    if (_stricmp(dot + 1, "jpeg") == 0 || _stricmp(dot + 1, "jpg") == 0) {
        return 1;
    }
    return 0;
}

// Removed: get_file_modification_date function is no longer needed
/*
char* get_file_modification_date(const char *filepath) {
    WIN32_FILE_ATTRIBUTE_DATA file_attributes;

    if (!GetFileAttributesExA(filepath, GetFileExInfoStandard, &file_attributes)) {
        return NULL;
    }

    SYSTEMTIME st;
    FileTimeToSystemTime(&file_attributes.ftLastWriteTime, &st);

    struct tm mtime;
    mtime.tm_year = st.wYear - 1900;
    mtime.tm_mon = st.wMonth - 1;
    mtime.tm_mday = st.wDay;
    mtime.tm_hour = st.wHour;
    mtime.tm_min = st.wMinute;
    mtime.tm_sec = st.wSecond;
    mtime.tm_isdst = -1;

    time_t rawtime = mktime(&mtime);
    if (rawtime == (time_t)-1) {
        perror("Error converting SYSTEMTIME to time_t");
        return NULL;
    }

    struct tm mtime_buffer;
    if (localtime_s(&mtime_buffer, &rawtime) != 0) {
        perror("Error converting time with localtime_s");
        return NULL;
    }

    char *date_str = (char *)malloc(80);
    if (date_str == NULL) {
        perror("Error allocating memory for date string");
        return NULL;
    }

    if (strftime(date_str, 80, "%Y-%m-%d %H:%M:%S", &mtime_buffer) == 0) {
        fprintf(stderr, "Error formatting date string for %s.\n", filepath);
        free(date_str);
        return NULL;
    }
    return date_str;
}
*/

// Global list to store image entries
ImageEntry* g_image_entries = NULL;
int g_image_count = 0;
int g_image_capacity = 0;

// Function to add an image entry to the global list 
void add_image_entry(const char* filepath, const char* hash) {
    if (g_image_count >= g_image_capacity) {
        g_image_capacity = (g_image_capacity == 0) ? 10 : g_image_capacity * 2;
        ImageEntry* temp = (ImageEntry*)realloc(g_image_entries, g_image_capacity * sizeof(ImageEntry));
        if (temp == NULL) {
            perror("Failed to reallocate memory for image entries");
            exit(EXIT_FAILURE);
        }
        g_image_entries = temp;
    }

    ImageEntry* new_entry = &g_image_entries[g_image_count];
    new_entry->index = g_image_count; // Store its own index (0-based)
    strncpy_s(new_entry->filepath, MAX_PATH_LEN, filepath, _TRUNCATE);
    strncpy_s(new_entry->hash, MD5_HASH_LEN, hash, _TRUNCATE);
    // new_entry->mod_date is removed

    g_image_count++;
}


// Recursive function to process directories and find JPEG images
void process_directory(const char* current_dir_path) {
    WIN32_FIND_DATAA find_data;
    HANDLE h_find;
    char search_path[MAX_PATH_LEN]; // Path for FindFirstFileA

    printf("Scanning directory: %s\n", current_dir_path); // User feedback

    // Construct the search path for the current directory (e.g., "C:\path\to\dir\*.*")
    if (snprintf(search_path, MAX_PATH_LEN, "%s\\*.*", current_dir_path) >= MAX_PATH_LEN) {
        fprintf(stderr, "Path too long: %s\n", current_dir_path);
        return;
    }

    h_find = FindFirstFileA(search_path, &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening directory %s (Error Code: %lu)\n", current_dir_path, GetLastError());
        return;
    }

    do {
        // Skip current directory "." and parent directory ".."
        if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_LEN];
        // Construct the full path of the current item (file or directory)
        if (snprintf(full_path, MAX_PATH_LEN, "%s\\%s", current_dir_path, find_data.cFileName) >= MAX_PATH_LEN) {
            fprintf(stderr, "Path too long: %s\\%s\n", current_dir_path, find_data.cFileName);
            continue;
        }

        // Check if it's a directory
        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recursively call for subdirectories
            process_directory(full_path);
        }
        else {
            // It's a regular file, check if it's a JPEG
            if (is_jpeg(find_data.cFileName)) {
                printf("  Processing image: %s\n", full_path); // User feedback

                // Removed: mod_date logic
                /*
                char *mod_date = get_file_modification_date(full_path);
                if (mod_date == NULL) {
                    fprintf(stderr, "Could not get modification date for %s. Skipping.\n", full_path);
                    continue; // Skip if date cannot be obtained
                }
                */

                int img_width, img_height, img_channels;
                // Load the image using stb_image
                unsigned char* img_data = stbi_load(full_path, &img_width, &img_height, &img_channels, 0);
                if (!img_data) {
                    fprintf(stderr, "Error loading image %s: %s\n", full_path, stbi_failure_reason());
                    // Removed: free(mod_date);
                    continue; // Skip if image cannot be loaded
                }

                // Allocate memory for the resized image
                // Ensure the allocation size is correct: width * height * channels
                size_t resized_data_size = (size_t)TEMP_IMG_SIZE * TEMP_IMG_SIZE * img_channels;
                unsigned char* resized_data = (unsigned char*)malloc(resized_data_size);
                if (!resized_data) {
                    perror("Failed to allocate memory for resized image");
                    stbi_image_free(img_data);
                    // Removed: free(mod_date);
                    continue;
                }

                // Simple nearest-neighbor resizing
                for (int y = 0; y < TEMP_IMG_SIZE; ++y) {
                    for (int x = 0; x < TEMP_IMG_SIZE; ++x) {
                        int orig_x = (int)(x * (img_width / (double)TEMP_IMG_SIZE));
                        int orig_y = (int)(y * (img_height / (double)TEMP_IMG_SIZE));

                        for (int c = 0; c < img_channels; ++c) {
                            resized_data[(y * TEMP_IMG_SIZE + x) * img_channels + c] =
                                img_data[(orig_y * img_width + orig_x) * img_channels + c];
                        }
                    }
                }

                // Calculate hash directly from the resized_data in memory
                char* hash = calculate_memory_md5(resized_data, resized_data_size);
                if (hash != NULL) {
                    // Call add_image_entry without mod_date
                    add_image_entry(full_path, hash);
                    free(hash); // Free the hash string
                }
                else {
                    fprintf(stderr, "Could not calculate hash for %s (in-memory). Skipping.\n", full_path);
                }

                // Clean up allocated memory
                stbi_image_free(img_data);
                free(resized_data);
                // Removed: free(mod_date);
            }
        }
    } while (FindNextFileA(h_find, &find_data) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        fprintf(stderr, "Error during file enumeration in %s (Error Code: %lu)\n", current_dir_path, GetLastError());
    }
    printf("Finished scanning directory: %s\n", current_dir_path); // User feedback

    FindClose(h_find);
}

int main() {
    FILE* csv_file;

    printf("Starting JPEG image database generation...\n"); // Initial user feedback

    // Open the CSV file in write mode ("w") using fopen_s
    if (fopen_s(&csv_file, "imageDB.csv", "w") != 0) {
        perror("Error opening imageDB.csv");
        return 1;
    }

    // Write the CSV header row - Now only FilePath, Hash, DuplicateOf
    fprintf(csv_file, "FilePath,Hash,DuplicateOf\n");

    // Start processing from the current directory
    process_directory(".\\");

    printf("\nImage scanning complete. Starting duplicate detection...\n"); // User feedback

    // --- Duplicate Detection Logic ---
    // After collecting all image entries, find duplicates
    for (int i = 0; i < g_image_count; i++) {
        char duplicate_indices[256] = ""; // To store comma-separated duplicate indices
        int has_duplicates = 0;

        for (int j = i + 1; j < g_image_count; j++) {
            if (strcmp(g_image_entries[i].hash, g_image_entries[j].hash) == 0) {
                // Found a duplicate
                has_duplicates = 1;
                // Append the duplicate's index to the string
                if (strlen(duplicate_indices) > 0) {
                    strncat_s(duplicate_indices, sizeof(duplicate_indices), ",", _TRUNCATE);
                }
                char index_str[20];
                // Add +2 to the 0-based index to match 1-based CSV line number (including header)
                snprintf(index_str, sizeof(index_str), "%d", g_image_entries[j].index + 2);
                strncat_s(duplicate_indices, sizeof(duplicate_indices), index_str, _TRUNCATE);
            }
        }

        // Now, write to CSV, including hash and duplicate info - Removed modification date
        fprintf(csv_file, "\"%s\",\"%s\",\"%s\"\n",
            g_image_entries[i].filepath, // FilePath is now the first column
            g_image_entries[i].hash,
            has_duplicates ? duplicate_indices : "");
    }

    // Free the dynamically allocated image entries list
    if (g_image_entries != NULL) {
        free(g_image_entries);
    }

    // Close the CSV file
    fclose(csv_file);

    printf("Duplicate detection and CSV writing complete. Image database (imageDB.csv) created successfully.\n"); 
    printf("Performance Note: Temporary files for hashing have been eliminated by processing images in memory.\n");
    printf("NOTE: This program includes stb_image.h source directly for image processing, requiring no external installations.\n");

    return 0;
}