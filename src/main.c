


#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "as65.h"

/* Returns instruction */
as65_instruction_info get_instruction(char *str){
    for(size_t i = 0; i < INSTRUCTION_COUNT; i++){
        if(!strcmp(opcode_array[i].str, str)){
            return opcode_array[i];
        }
    }

    return (as65_instruction_info){NULL, 0};
}




int main(int argc, char **argv){

    FILE *read = fopen(argv[1], "r");

    if(!read){
        int error = errno;
        printf("as65: %s: %s", argv[1], strerror(error));
        return 1;
    }

    FILE *binary = fopen(argv[2], "w+");

    if(!binary){
        int error = errno;
        printf("as65: %s: %s", argv[2], strerror(error));
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
        
        as65_line info = parse_line(line);

        printf("opcode: %s\noperand1: %s\noperand2: %s\n", info.opcode, info.operand1, info.operand2);

        if(check_instruction(info.opcode, info.operand1, info.operand2) == -1){
            printf("as65: error at line %d\n", line_num);
        }

        get_instruction_binary(info.opcode, info.operand1, info.operand2);

        line_num++;

    }



}