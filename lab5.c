/**
 * This program reads a WAV audio file and prints statistics about the audio samples. The file name 
 * is provided using command line arguments. If the file name is not provided or the file is not readable, 
 * the program will exit and provide an error message.
 *
 * @author Bobby Wiles {@literal <pluf@wfu.edu>}
 * @date Mar. 14, 2021
 * @assignment Lab 4  
 * @course CSC 250
 **/

#include "get_wav_args.h"
#include <math.h>  
#include <stdio.h> 
#include <stdlib.h>  
#include <string.h>  


int read_wav_header(FILE* in_file, short *sample_size_ptr, int *num_samples_ptr, int *sample_rate_ptr, short *num_channels_ptr);
int read_wav_data(FILE* in_file, FILE* out_file, short sample_size, int num_samples, short num_channels, int bit);


int main(int argc, char *argv[]) 
{
    FILE *in_file; 
    FILE *out_file;     /* WAV file */
    short sample_size = 0;  /* size of an audio sample (bits) */
    int sample_rate = 0;    /* sample rate (samples/second) */
    int num_samples = 0;    /* number of audio samples */ 
    int wav_ok = 0;     /* 1 if the WAV file si ok, 0 otherwise */
    short num_channels = 0;
    int bit;
    char wav_file_name[256];
    char text_file_name[256];
    int args_ok;

    args_ok = get_wav_args(argc, argv, &bit, wav_file_name, text_file_name);

    if (!args_ok) {
        printf("Not good args.\n");
        return 1;
    }

    in_file = fopen(wav_file_name, "rbe"); 
    out_file = fopen(text_file_name,"w+e");
    if (!in_file || !out_file) {
        printf("could not open wav file %s \n", wav_file_name);
        return 2;
    }

    wav_ok = read_wav_header(in_file, &sample_size, &num_samples, &sample_rate, &num_channels);
    if (!wav_ok) {
       printf("wav file %s has incompatible format \n", argv[1]);   
       return 3;
    }
    
    read_wav_data(in_file, out_file, sample_size, num_samples, num_channels, bit);

    if (in_file) {
        fclose(in_file);
    } 
    if (out_file) {
        fclose(out_file);
    }

    return 0;
}


/**
 *  function reads the RIFF, fmt, and start of the data chunk. 
 */
int read_wav_header(FILE* in_file, short *sample_size_ptr, int *num_samples_ptr, int *sample_rate_ptr, short *num_channels_ptr) {
    char chunk_id[] = "    ";  /* chunk id, note initialize as a C-string */
    char data[] = "    ";      /* chunk data */
    int chunk_size = 0;        /* number of bytes remaining in chunk */
    short audio_format = 0;    /* audio format type, PCM = 1 */
    short num_channels = 0;    /* number of audio channels */ 
    int byte_rate = 0;
    short block_align_rate = 0;

    /* first chunk is the RIFF chunk, let's read that info */  
    fread(chunk_id, 4, 1, in_file);
    fread(&chunk_size, 4, 1, in_file);
    /*printf("chunk: %s \n", chunk_id);*/
    fread(data, 4, 1, in_file);
    /*printf("  data: %s \n", data);*/

    /* let's try to read the next chunk, it always starts with an id */
    fread(chunk_id, 4, 1, in_file);
    /* if the next chunk is not "fmt " then let's skip over it */  
    while (strcmp(chunk_id, "fmt ") != 0) {
        fread(&chunk_size, 4, 1, in_file);
        /* skip to the end of this chunk */  
        fseek(in_file, chunk_size, SEEK_CUR);
        /* read the id of the next chuck */  
        fread(chunk_id, 4, 1, in_file);
    }

    /* if we are here, then we must have the fmt chunk, now read that data */  
    fread(&chunk_size, 4, 1, in_file);
    fread(&audio_format, sizeof(audio_format), 1, in_file);
    fread(&num_channels, sizeof(num_channels), 1, in_file);
    fread(&*sample_rate_ptr, sizeof(*sample_rate_ptr), 1, in_file);
    fread(&byte_rate, sizeof(byte_rate), 1, in_file);
    fread(&block_align_rate, sizeof(block_align_rate), 1, in_file);
    fread(&*sample_size_ptr, sizeof(*sample_size_ptr), 1, in_file);
    /* you'll need more reads here, hear? */  

    /* read the data chunk next, use another while loop (like above) */
    /* visit http://goo.gl/rxnHB1 for helpful advice */

    fread(&chunk_id, 4, 1, in_file);

    while (strcmp(chunk_id, "data") != 0) {
        fread(&chunk_size, 4, 1, in_file);
        fseek(in_file, chunk_size, SEEK_CUR);
        fread(chunk_id, 4, 1, in_file);
    }

    fread(&chunk_size, 4, 1, in_file);
    
    *num_samples_ptr = chunk_size;

    *num_channels_ptr = num_channels;
    return (audio_format == 1);
}


/**
 *  function reads the WAV audio data (last part of the data chunk)
 */
int read_wav_data(FILE* in_file, FILE* out_file, short sample_size, int num_samples, short num_channels_ptr, int bit) {
    int i = 0;
    int x = 0;
    int p = 0;
    short sample_size_bytes = (sample_size/8);
    unsigned int sample = 0;
    unsigned char ch = 0;
    unsigned int mask = 0;
    unsigned int mask4 = 0x0f;
    unsigned int mask2 = 0x03;
    unsigned int mask1 = 0x01;
    int character_counter = 0;
    int sample_counter = 0;
    int smileTrigger = 0;


    num_samples = num_samples/(num_channels_ptr*(sample_size/8));
    if (bit == 1) {
        mask = mask1;
    } else if (bit == 2) {
        mask = mask2;
    } else if (bit == 4) {
        mask = mask4;
    }


    for (i = 0; i < num_samples; i++) {
        for (x = 0; x < 8/bit; x++) {
            fread(&sample, sample_size_bytes, 1, in_file);
            
            if (x == 0) {
                ch = (sample & mask) << (unsigned int) bit;
            } else if (x == (8/bit)-1) {
                ch = ch | (sample & mask);
                character_counter += 1;
                fputc(ch, out_file);
            } else {
                ch = (ch | (sample & mask)) << (unsigned int) bit;
            }
            sample_counter += 1;

        }

        if (ch == ':' || ch == ')') {
            if (p == 1) {
                i = num_samples;
                smileTrigger = 1;
            }
            p += 1;
        } else {
            p = 0;
        }
    }

    printf("%d characters recovered from %d samples", character_counter, sample_counter);

    if (!smileTrigger) {
        fputs("\n:)", out_file);
    }
    
    printf("\n");

   return 1;
}
