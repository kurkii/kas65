
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kas65.h"

int main(int argc, char **argv){

    FILE *read = fopen(argv[1], "r");

    if(!read){
        int error = errno;
        printf("kas65: %s: %s", argv[1], strerror(error));
        return 1;
    }

    FILE *binary = fopen(argv[2], "w+");

    if(!binary){
        int error = errno;
        printf("kas65: %s: %s", argv[2], strerror(error));
        return 1;
    }

    kas65_directive *directives = malloc(sizeof(kas65_directive) * MAX_DIRECTIVE);

    memset(directives, 0, sizeof(kas65_directive) * MAX_DIRECTIVE);

    int directive_num = 0;      // total number of directives, also plays as an index in the below while loop
    int address_counter = 0;    // current address counter, used for labels
    
    /* Line buffer */
    char *line = malloc(sizeof(char) * MAX_LINE_LENGTH);

    /* Actual array of lines */
    char **buffer = malloc(sizeof(char**) * MAX_FILE_LINES);
    for(size_t i = 0; i < MAX_FILE_LINES; i++){
        buffer[i] = malloc(sizeof(char) * MAX_LINE_LENGTH);
        memset(buffer[i], 0, MAX_LINE_LENGTH);
    }

    /* Index for lines */
    int line_count = 0;

    /* Zeroth pass - put the entire file into `buffer` */
    while(fgets(line, MAX_LINE_LENGTH, read) != NULL){
        memcpy(buffer[line_count], line, MAX_LINE_LENGTH);
        printf("buffer: %s\n", buffer[line_count]);
        line_count++;
    }

    /* Counter for the line of the file */
    int line_num = 1;

    printf("hi\n");

    /* First pass - catch all directives and symbols and put them in a table */
    for(size_t i = 0; i < line_count; i++){
        printf("i: %zu\n", i);
        printf("buffer[%zu]: %s\n", i, buffer[i]);

        kas65_line info = parse_line(directives, buffer, &directive_num, i, address_counter, true);

        int ret = parse_labels_constants(&address_counter, &directive_num, directives, info.opcode, info.operand1, info.operand2);

        if(ret == -2){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }else if(ret != -1){
            directive_num++; // Directive found, increase the index and directive counter
        }

        /* If opcode is NULL (comment) then continue */
        if(!info.opcode){
            continue;
        }

        /* Error occured */
        if(!strcmp(info.opcode, "err")){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }

        /* Get current instruction size so we can add it do address_counter */
        int size = get_instruction_size(info.opcode, info.operand1, info.operand2);

        if(size < 0){
            kas65_log(LOG_ERROR, "Failed to get instruction size");
            return 1;
        }

        address_counter += size;

        printf("address: %d\n", address_counter);

        printf("eeey\n");

    }
    
    /* Second pass - read the source file and output machine code */
    for(size_t i = 0; i < line_count; i++){

        printf("hey\n");

        printf("iteration: %d\n", line_num);
        
        kas65_line info = parse_line(directives, buffer, &directive_num, i, address_counter, false);


        /* If opcode is NULL (comment) then continue */
        if(!info.opcode){
            continue;
        }

        if(!strcmp(info.opcode, "err")){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }

        printf("opcode: %s\noperand1: %s\noperand2: %s\n", info.opcode, info.operand1, info.operand2);

        int ret = check_instruction(info.opcode, info.operand1, info.operand2);

        if(ret == -2){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }else if(ret == -1){
            line_num++;
            continue;
        }

        int instruction = get_instruction_binary(directives, address_counter, info.opcode, info.operand1, info.operand2);

        if(instruction == -1){
            kas65_log(LOG_ERROR, "Encountered error on second pass, stopping");
            return 1;
        }

        printf("instruction: 0x%x\n", instruction);

        if(instruction > 0xFFFF){
            /* Swap endianess */
            uint64_t new = (instruction & 0x0000FF) << 16 | (instruction & 0x00FF00) | (instruction & 0xFF0000) >> 16;
            printf("instruction & 0xff: %x\ninstruction & 0xff00: %x\ninstruction & 0xff0000: %x\n", (instruction & 0x0000FF), (instruction & 0x00FF00), (instruction & 0xFF0000));
            printf("instruction swap: 0x%lx\n", new);
            fwrite(&new, 1, 3, binary);
        }else if (instruction > 0xFF) {
            uint16_t new = __bswap_16(instruction);
            printf("new: 0x%x\n", new);
            fwrite(&new, 1, 2, binary);
        }else {
            fwrite(&instruction, 1, 1, binary);
        }

        line_num++;
    }
}