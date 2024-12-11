#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "as65.h"

#define OPCODE_CHAR     3   
#define OPERAND_CHAR    9

void remove_whitespace(char *buffer){ 

    for(size_t i = 0; i < strlen(buffer); i++){
        if((isspace(buffer[i]) && buffer[i] != '\n')){
            size_t j;
            for(j = i; j < strlen(buffer); j++){ // shift the string
                buffer[j] = buffer[j+1];
            }
        }
    }
}

as65_line parse_line(char *buffer){
    remove_whitespace(buffer);

    printf("buffer: %s\n", buffer);

    char *opcode = malloc(sizeof(char) * OPCODE_CHAR+1);
    char *operand1 = malloc(sizeof(char) * OPERAND_CHAR+1);
    char *operand2 = malloc(sizeof(char) * OPERAND_CHAR+1);
    
    memcpy(opcode, buffer, OPCODE_CHAR);
    
    memset(operand1, 0, OPERAND_CHAR);
    memset(operand2, 0, OPERAND_CHAR);

    opcode[3] = '\0';

    int i = OPCODE_CHAR;
    int j = 0; /* Index for first operand */
    int k = 0; /* Index for second operand */

    bool second_operand = false;

    while((buffer[i] != '\n' || buffer[i] != '\n') && i < 4096){

        /* ; means comment, so skip anything after the comment */
        if(buffer[i] == ';'){
            break;
        }

        /* NUL terminator */
        if(buffer[i] == 0){
            break;
        }

        /* If there is a , that means that there is a second operand */
        if(buffer[i] == ','){
            second_operand = true;
            i++;
            continue;
        }

        if(second_operand == false){

            if(j > OPERAND_CHAR){
                printf("as65: 1. Operand length is over limit (%d)\n", OPERAND_CHAR);
                break;
            }

            operand1[j] = buffer[i];
            j++;

        }else{

            if(k > OPERAND_CHAR){
                printf("as65: 2. Operand length is over limit (%d)\n", OPERAND_CHAR);
                break;
            }

            operand2[k] = buffer[i];

            k++;
        }
        i++;
    }

    /* Add null terminator */
    operand1[j] = '\0';
    operand2[k] = '\0';

    remove_whitespace(operand1);
    remove_whitespace(operand2);

    /* If the first operand doesn't contain anything then NULL it */
    if(!strcmp(operand1, "")){
        free(operand1);
        operand1 = NULL;
    }

    /* Do the same for the second operand */
    if(second_operand == false){
        free(operand2);
        operand2 = NULL;
    }

    


    return (as65_line){opcode, operand1, operand2};
}

/* Checks instruction for syntax errors */
int check_instruction(char *opcode, char *operand1, char *operand2){


    printf("inside operand1: %s\n", operand1);

    bool operand1_exists = true;
    bool operand2_exists = true;

    bool found = false;

    /* Temporary strings */
    char *temp_operand1 = malloc(sizeof(char) * OPERAND_CHAR);
    char *temp_operand2 = malloc(sizeof(char) * OPERAND_CHAR); 

    memset(temp_operand1, 0, sizeof(char) * OPERAND_CHAR);

    if(operand1 == NULL){
        operand1_exists = false;
    }else{
        memcpy(temp_operand1, operand1, sizeof(char) * OPERAND_CHAR);
    }

    memset(temp_operand2, 0, sizeof(char) * OPERAND_CHAR);

    if(operand2 == NULL){
        operand2_exists = false;
    }else{
        memcpy(temp_operand2, operand2, sizeof(char) * OPERAND_CHAR);
    }

    for(int i = 0; i < INSTRUCTION_COUNT; i++){
        if(!strcmp(opcode, opcode_array[i].str)){

            found = true;

            if((opcode_array[i].implied == 0) && !operand1_exists){
                as65_log(LOG_ERROR, "Expected operand");
                goto error;
            }

            if(opcode_array[i].implied != 0 && operand1_exists){
                as65_log(LOG_ERROR, "Operand not expected");
                goto error;
            }

            if((opcode_array[i].zeropageX == 0 && opcode_array[i].absoluteX == 0 && opcode_array[i].indirectX == 0) && tolower(temp_operand2[0]) == 'x'){
                as65_log(LOG_ERROR, "X not expected as operand");
                goto error;
            }

            if((opcode_array[i].zeropageY == 0 && opcode_array[i].absoluteY == 0 && opcode_array[i].indirectY == 0) && tolower(temp_operand2[0]) == 'y'){
                as65_log(LOG_ERROR, "Y not expected as operand");
                goto error;
            }

            if(temp_operand1[0] == '#' && opcode_array[i].immediate == 0){
                as65_log(LOG_ERROR, "Literal not expected (#)");
                goto error;
            }
        }
    }

    /* Return 0 if opcode exists, otherwise print error */
    if(found){
        return 0;
    }else{
        as65_log(LOG_ERROR, "Invalid opcode"); 
    }

    error:
        free(temp_operand1);
        free(temp_operand2);
        return -1;
}

/* Returns the instruction in machine code */
int get_instruction_binary(char *opcode, char *operand1, char *operand2){
    for(size_t i = 0; i < INSTRUCTION_COUNT; i++){
        if(!strcmp(opcode, opcode_array[i].str)){

            if(!operand1 && !operand2){
                return opcode_array[i].implied;
            }

            if(operand1[0] == '#'){
                return opcode_array[i].immediate;
            }

            if(operand1[0] == '$'){
                char *nptr = malloc(sizeof(char) * OPERAND_CHAR);

                char **endptr = malloc(sizeof(char**));
                *endptr = malloc(sizeof(char*));

                memset(nptr, 0, sizeof(char) * OPERAND_CHAR);

                /* Copy everything after the $ */
                memcpy(nptr, operand1 + 1, sizeof(char) * OPERAND_CHAR - 1);

                int result = strtol(nptr, endptr, 0);

                /* If not parsed correctly then throw error */
                if(!(**endptr == 0 && result != 0)){
                    as65_log(LOG_ERROR, "Expected number");
                    printf("as65: Unexpected character: %c\n", **endptr);
                    return -1;
                }

                
            }
        }
    }

    return -1;
}