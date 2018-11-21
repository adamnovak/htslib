#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <zlib.h>
#include <libdeflate.h>

int main(int acrc, char** argv) {
    // Open the input file
    FILE* in_file;
    in_file = fopen("badblock.dat", "rb");
    
    size_t INPUT_SIZE = 19909;
    
    // Set up a buffer to hold it
    uint8_t* input_buffer = (uint8_t*) malloc(INPUT_SIZE);
    assert(input_buffer != NULL);
    
    // Read it in
    int c;
    int next = 0;
    while (1) {
        c = fgetc(in_file);
        if (c == EOF) {
            break;
        }
        input_buffer[next] = c;
        next++;
    }
    // Make sure we got it all
    assert(next == INPUT_SIZE);
    
    // Decompress with zlib
    size_t output_buffer_size = 1;
    
    while(1) {
        if (output_buffer_size > 1073741824) {
            printf("Refusing to decompress more than 1 GB of data from our tiny test input with zlib\n");
            return 1;
        }
        
        // Allocate output buffer
        uint8_t* output_buffer = (uint8_t*) malloc(output_buffer_size);
        
        // Set up the decompressor
        z_stream zs;
        zs.zalloc = NULL;
        zs.zfree = NULL;
        zs.msg = NULL;
        zs.next_in = (Bytef*) input_buffer;
        zs.avail_in = INPUT_SIZE;
        zs.next_out = (Bytef*) output_buffer;
        zs.avail_out = output_buffer_size;
        
        // Do the initialization
        assert(inflateInit2(&zs, -15) == Z_OK);
        
        // Do decompression
        int ret = inflate(&zs, Z_FINISH);
        
        size_t extracted = output_buffer_size - zs.avail_out;
        size_t consumed = INPUT_SIZE - zs.avail_in;
        
        if (ret == Z_STREAM_END) {
            // We decompressed to the end of the stream
            assert(inflateEnd(&zs) == Z_OK);
            free((void*) output_buffer);
            
            printf("Successfully decompressed %zu bytes with zlib\n", extracted);
            break;
        } else if (ret != Z_BUF_ERROR && ret != Z_OK) {
            printf("zlib decompression failed with other error %d\n", ret);
            inflateEnd(&zs);
            free((void*) output_buffer);
            return 1;
        }
        
        // Otherwise we ran out of space (didn't make it to the end of the stream)
        printf("%zu was not enough space for zlib result; zlib said %d and extracted %zu from %zu; trying %zu\n", output_buffer_size, ret, extracted, consumed, output_buffer_size * 2);
        output_buffer_size *= 2;
        
        inflateEnd(&zs);
        free((void*) output_buffer);
    }
    
    // Decompress with libdeflate
    output_buffer_size = 1;
    while(1) {
        if (output_buffer_size > 1073741824) {
            printf("Refusing to decompress more than 1 GB of data from our tiny test input with libdeflate\n");
            return 1;
        }
    
        // Allocate output buffer
        uint8_t* output_buffer = (uint8_t*) malloc(output_buffer_size);
        
        // Set up the decompressor
        struct libdeflate_decompressor *z = libdeflate_alloc_decompressor();
        
        // Allocate a place to read back the actual size
        size_t actual_size = 0;
        
        // Decompress
        int ret = libdeflate_deflate_decompress(z, input_buffer, INPUT_SIZE, output_buffer, output_buffer_size, &actual_size);
        
        // Clean up
        libdeflate_free_decompressor(z);
        free((void*) output_buffer);
        
        if (ret == LIBDEFLATE_SUCCESS) {
            printf("Successfully decompressed %zu bytes with libdeflate\n", actual_size);
            break;
        } else if (ret != LIBDEFLATE_INSUFFICIENT_SPACE) {
            printf("libdeflate decompression failed with other error %d\n", ret);
            return 1;
        }
        
        // Otherwise we had insufficient space
        printf("%zu was not enough space for libdeflate result; trying %zu\n", output_buffer_size, output_buffer_size * 2);
        output_buffer_size *= 2;
    }
    
    free((void*) input_buffer);
    return 0;
}
