/*
 * Copyright (c) 2024 Rodrigo Vigna
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include "blake3.h"

#define DEFAULT_HASH_SIZE BLAKE3_OUT_LEN  /*  Default digest size */

#define MAX_HASH_SIZE 256                 /*  Maximum hash size is limited
                                           *  to cryptsetup 2.4.3 max
                                           *  password size */

#define CHUNK_SIZE 32768                  /*  Read input in 32KB chunks */

// Convert hash to a hexadecimal string
void print_hex_to_var(const uint8_t *hash, size_t len, char *output) {
    for (size_t i = 0; i < len; i++) {
        sprintf(&output[i * 2], "%02x", hash[i]);
    }
    output[len * 2] = '\0';  // Null-terminate the string
}

// Display usage information
void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [-s digest_size] [-r repeat_count]\n", program_name);
    fprintf(stderr, "  -s digest_size: Specify the output digest size (default 32 bytes).\n");
    fprintf(stderr, "  -r repeat_count: Specify the number of hash iterations (>= 1, default 1).\n");
}

int main(int argc, char *argv[]) {
    int opt;
    size_t digest_size = DEFAULT_HASH_SIZE;   // Default digest size
    int repeat_count = 1;                     // Default repeat count

    // Parse command-line options
    while ((opt = getopt(argc, argv, "s:r:")) != -1) {
        switch (opt) {
            case 's': {
                digest_size = atoi(optarg);
                if (digest_size < DEFAULT_HASH_SIZE || digest_size > MAX_HASH_SIZE) {
                    fprintf(stderr, "Error: -s (digest size) must be between %d and %d.\n", DEFAULT_HASH_SIZE, MAX_HASH_SIZE);
                    return 1;
                }
                break;
            }
            case 'r': {
                int parsed_count = atoi(optarg);
                if (parsed_count <= 0) {
                    fprintf(stderr, "Error: -r (repeat count) must be a positive integer.\n");
                    return 1;
                }
                repeat_count = parsed_count;
                break;
            }
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Allocate memory for input data
    char *message = malloc(CHUNK_SIZE);
    if (!message) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    size_t total_length = 0;
    size_t buffer_size = CHUNK_SIZE;

    // Read input from stdin
    while (!feof(stdin)) {
        size_t bytes_read = fread(message + total_length, 1, CHUNK_SIZE, stdin);
        total_length += bytes_read;

        // Resize buffer if necessary
        if (total_length + CHUNK_SIZE > buffer_size) {
            buffer_size *= 2;
            char *new_message = realloc(message, buffer_size);
            if (!new_message) {
                fprintf(stderr, "Error: Memory allocation failed.\n");
                free(message);
                return 1;
            }
            message = new_message;
        }
    }

    // Check for read errors
    if (ferror(stdin)) {
        fprintf(stderr, "Error: Failed to read input.\n");
        free(message);
        return 1;
    }

    // Allocate memory for hash and output
    uint8_t *hash = malloc(digest_size);
    char *finalhash = malloc(digest_size * 2 + 1);
    if (!hash || !finalhash) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        free(message);
        free(hash);
        free(finalhash);
        return 1;
    }

    // Initialize BLAKE3 hasher and compute hash
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, message, total_length);
    blake3_hasher_finalize(&hasher, hash, digest_size);

    // Convert hash to hexadecimal string
    print_hex_to_var(hash, digest_size, finalhash);

    // Perform additional hash iterations if requested
    for (int i = 2; i <= repeat_count; i++) {
        size_t finalhash_len = strlen(finalhash);
        memcpy(message, finalhash, finalhash_len + 1);
        blake3_hasher_init(&hasher);
        blake3_hasher_update(&hasher, message, finalhash_len);
        blake3_hasher_finalize(&hasher, hash, digest_size);
        print_hex_to_var(hash, digest_size, finalhash);
    }

    // Output the final hash
    printf("%s", finalhash);

    // Clean up allocated resources
    free(message);
    free(hash);
    free(finalhash);

    return 0;
}
