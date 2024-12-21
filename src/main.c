


#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kas65.h"

/* Returns instruction */
kas65_instruction_info get_instruction(char *str){
    for(size_t i = 0; i < INSTRUCTION_COUNT; i++){
        if(!strcmp(opcode_array[i].str, str)){
            return opcode_array[i];
        }
    }

    return (kas65_instruction_info){NULL, 0};
}




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

    /* Buffer */
    char line[MAX_LINE_LENGTH];

    /* Counter for the line of the file */
    int line_num = 1;

    char *opcode = malloc(sizeof(char) * MAX_LINE_LENGTH);
    char *operand = malloc(sizeof(char) * MAX_LINE_LENGTH);

    memset(opcode, 0, sizeof(char) * MAX_LINE_LENGTH);
    memset(operand, 0, sizeof(char) * MAX_LINE_LENGTH);

    while(fgets(line, MAX_LINE_LENGTH, read) != NULL){

        printf("iteration: %d\n", line_num);
        
        kas65_line info = parse_line(line);

        /* If opcode is NULL (comment) then continue */
        if(!info.opcode){
            continue;
        }

        if(!strcmp(info.opcode, "err")){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }

        printf("opcode: %s\noperand1: %s\noperand2: %s\n", info.opcode, info.operand1, info.operand2);

        if(check_instruction(info.opcode, info.operand1, info.operand2) == -1){
            printf("kas65: error at line %d\n", line_num);
            return 1;
        }

        int instruction = get_instruction_binary(info.opcode, info.operand1, info.operand2);

        if(instruction == -1){
            kas65_log(LOG_ERROR, "Encountered error, stopping");
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
            fwrite(&new, 1, 2, binary);
        }else {
            fwrite(&instruction, 1, 1, binary);
        }

        line_num++;

    }



}