#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <byteswap.h>
#include "kas65.h"

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

kas65_line parse_line(char *buffer){
    remove_whitespace(buffer);

    printf("buffer: %s\n", buffer);

    if(buffer[0] == ';'){
        /* Comment */
        return (kas65_line){NULL, NULL, NULL};
    }

    char *opcode = malloc(sizeof(char) * OPCODE_CHAR+1);
    char *operand1 = malloc(sizeof(char) * OPERAND_CHAR+1);
    char *operand2 = malloc(sizeof(char) * OPERAND_CHAR+1);
    
    memcpy(opcode, buffer, OPCODE_CHAR);

    if(opcode[0] == 10){
        /* Empty line */
        return (kas65_line){NULL, NULL, NULL};
    }
    
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
                printf("kas65: First operand length is over limit (%d)\n", OPERAND_CHAR);
                return (kas65_line){"err", "err", "err"};
            }

            operand1[j] = buffer[i];
            j++;

        }else{

            if(k > OPERAND_CHAR){
                printf("kas65: Second operand length is over limit (%d)\n", OPERAND_CHAR);
                return (kas65_line){"err", "err", "err"};
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

    str_to_lower(opcode);
    str_to_lower(operand1);
    str_to_lower(operand2);

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

    return (kas65_line){opcode, operand1, operand2};
}

/* Checks instruction for syntax errors */
int check_instruction(char *opcode, char *operand1, char *operand2){


    printf("inside operand1: %s\n", operand1);

    bool found = false;

    /* Temporary strings */
    char *temp_operand1 = malloc(sizeof(char) * OPERAND_CHAR);
    char *temp_operand2 = malloc(sizeof(char) * OPERAND_CHAR); 

    memset(temp_operand1, 0, sizeof(char) * OPERAND_CHAR);

    if(operand1 != NULL){
        memcpy(temp_operand1, operand1, sizeof(char) * OPERAND_CHAR);
    }

    memset(temp_operand2, 0, sizeof(char) * OPERAND_CHAR);

    if(operand2 != NULL){
        memcpy(temp_operand2, operand2, sizeof(char) * OPERAND_CHAR);
    }

    for(int i = 0; i < INSTRUCTION_COUNT; i++){
        if(!strcmp(opcode, opcode_array[i].str)){

            found = true;

            if((opcode_array[i].implied == 0) && !operand1){
                kas65_log(LOG_ERROR, "Expected operand");
                goto error;
            }

            if(opcode_array[i].implied != 0 && operand1){
                kas65_log(LOG_ERROR, "Operand not expected");
                goto error;
            }

            if(temp_operand1[0] != '$' && temp_operand1[0] != '(' && opcode_array[i].implied == 0){
                kas65_log(LOG_ERROR, "Unexpected operand value");
                printf("kas65: Unexpected character '%c' as operand\n", temp_operand1[0]);
                goto error;
            }

            if(temp_operand1[0] == '(' && (opcode_array[i].indirect == 0 || opcode_array[i].indirectX == 0 || opcode_array[i].indirectY == 0) == 0){
                kas65_log(LOG_ERROR, "Unexpected indirect addressing");
                printf("kas65: Unexpected '(' in operand\n");
                goto error;
            }else if(temp_operand1[0] == '('){
                /* Indirect and Indirect Y */
                if((opcode_array[i].indirectY != 0 || opcode_array[i].indirect != 0) && (temp_operand2[0] == 'y')){
                    int i = 0;
                    bool ok = false;
                    while (temp_operand1[i] != '\0' && i < OPERAND_CHAR) {
                        if(temp_operand1[i] == ')' && temp_operand1[i+1] == '\0'){
                            ok = true;
                        }
                        i++;
                    }
                    /* Didn't find closing bracket */
                    if(!ok){
                        kas65_log(LOG_ERROR, "Expected closing bracket in first operand");
                        goto error;
                    }
                }else if(opcode_array[i].indirectX != 0 && temp_operand2[0] == 'x'){
                    int i = 0;
                    bool ok = false;
                    while (temp_operand2[i] != '\0') {
                        if(temp_operand2[i] == ')' && temp_operand2[i+1] == '\0'){
                            ok = true;
                        }
                        i++;
                    }
                    /* Didn't find closing bracket */
                    if(!ok){
                        kas65_log(LOG_ERROR, "Expected closing bracket in second operand");
                        goto error;
                    }   
                }

            }

            if(temp_operand1[0] == '(' && (opcode_array[i].indirectX != 0 && opcode_array[i].indirectY != 0) && !operand2){
                kas65_log(LOG_ERROR, "Expected either X or Y as operand");
                goto error;           
            }

            if(temp_operand1[0] != '$' && ((opcode_array[i].absolute != 0 || opcode_array[i].absoluteX != 0
                                            ||   opcode_array[i].absoluteY != 0 || opcode_array[i].relative != 0
                                            ||   opcode_array[i].zeropage != 0 || opcode_array[i].zeropageX != 0
                                            ||   opcode_array[i].zeropageY != 0)) && opcode_array[i].immediate == 0 && opcode_array[i].indirect == 0){

                kas65_log(LOG_ERROR, "Expected address");
                printf("kas65: Expected '$', got '%c'\n", temp_operand1[0]);
                goto error;                        
            }

            if(temp_operand1[1] == '#' && opcode_array[i].immediate == 0){
                kas65_log(LOG_ERROR, "Instruction does not take in literal (#)");
                goto error;
            } 

            if((opcode_array[i].zeropageX == 0 && opcode_array[i].absoluteX == 0 && opcode_array[i].indirectX == 0) && tolower(temp_operand2[0]) == 'x'){
                kas65_log(LOG_ERROR, "X not expected as operand");
                goto error;
            }

            if((opcode_array[i].zeropageY == 0 && opcode_array[i].absoluteY == 0 && opcode_array[i].indirectY == 0) && tolower(temp_operand2[0]) == 'y'){
                kas65_log(LOG_ERROR, "Y not expected as operand");
                goto error;
            }

            if(tolower(temp_operand2[0]) == 'a' && opcode_array[i].accumulator == 0){
                kas65_log(LOG_ERROR, "A not expected as operand");
                goto error;
            }

            if(temp_operand1[0] == '(' && (opcode_array[i].indirect == 0 && opcode_array[i].indirectX == 0 && opcode_array[i].indirectY == 0)){
                kas65_log(LOG_ERROR, "Indirect addressing not expected (brackets)");
                goto error;
            }
        }
    }

    /* Return 0 if opcode exists, otherwise print error */
    if(found){
        return 0;
    }else{
        printf("kas65: Invalid opcode '%s'\n", opcode); 
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

            /* Implied */
            if(!operand1 && !operand2){
                
                /* Work around for `brk`, which has opcode 00 */
                if(opcode_array[i].implied == 0xFF){
                    return 0;
                }

                return opcode_array[i].implied;
            }

            /* Accumulator */
            if(tolower(operand1[0]) == 'a'){
                return opcode_array[i].accumulator;
            }

            /* Indirect */
            if(operand1[0] == '('){
                char *temp_operand1 = malloc(sizeof(char) * OPERAND_CHAR);
                int ret = -1;

                memcpy(temp_operand1, operand1, OPERAND_CHAR);

                /* Remove the closing bracket so we can parse the operand */
                temp_operand1[strlen(temp_operand1) - 1] = 0;

                int result = parse_number(temp_operand1, 2);

                if(!operand2){
                    if(result < 0){
                        kas65_log(LOG_ERROR, "Invalid negative address");
                        printf("kas65: Valid addresses are from 0-65536, got %d\n", result);
                        return -1;
                    }else if(result > 0xffff){
                        kas65_log(LOG_ERROR, "Operand too big");
                        printf("kas65: Expected operand no more than 0xffff, got 0x%x\n", result); 
                    }

                    ret = opcode_array[i].indirect << 16;

                    ret |= result;
                }else if(operand2[0] == 'x'){
                    if(result < 0){
                        kas65_log(LOG_ERROR, "Invalid negative address");
                        printf("kas65: Valid addresses are from 0-255, got %d\n", result);
                        return -1;
                    }else if(result > 0xff){
                        kas65_log(LOG_ERROR, "Operand too big");
                        printf("kas65: Expected operand no more than 255, got %d\n", result); 
                    }

                    ret = opcode_array[i].indirectX << 8;

                    printf("ret: %d\n", ret);

                    ret |= result;                    
                }else if(operand2[0] == 'y'){
                    if(result < 0){
                        kas65_log(LOG_ERROR, "Invalid negative address");
                        printf("kas65: Valid addresses are from 0-255, got %d\n", result);
                        return -1;
                    }else if(result > 0xff){
                        kas65_log(LOG_ERROR, "Operand too big");
                        printf("kas65: Expected operand no more than 255, got %d\n", result); 
                    }

                    ret = opcode_array[i].indirectY << 8;

                    ret |= result;                        
                }else{
                    kas65_log(LOG_ERROR, "Unexpected character as second operand");
                    printf("Expected either X or Y, got %c\n", operand2[0]);
                }

                return ret;

            }

            /* Immediate */
            if(opcode_array[i].immediate != 0 && operand1[0] == '$' && operand1[1] == '#'){

                int result = parse_number(operand1, 2);

                if(result < 0){
                    kas65_log(LOG_ERROR, "Invalid negative value");
                    printf("kas65: Valid values are from 0-255, got %d\n", result);
                    return -1;
                }else if(result > 255){
                    kas65_log(LOG_ERROR, "Operand too big");
                    printf("kas65: Expected operand no more than 255, got %d\n", result); 
                }

                int ret = opcode_array[i].immediate << 8;

                ret |= result;

                return ret;  
            }

            if(operand1[0] == '$'){
                int result = parse_number(operand1, 1);

                printf("result: %d\n", result);

                if(result > 0xFFFF){
                    kas65_log(LOG_ERROR, "Operand too big");
                    printf("kas65: Expected operand less than 0xffff, got 0x%x\n", result);
                    return -1;
                }else if(result < -128){
                    kas65_log(LOG_ERROR, "Operand too big");
                    printf("kas65: Expected operand more than -128, got %d\n", result);   
                    return -1;                 
                }

                /* Bigger than 1 byte, either absolute or indirect addressing */
                if(result > 0xFF){

                    if(opcode_array[i].indirect > 0){
                        /* Indirect addressing */

                        /* Shift the opcode up two bytes */
                        int ret = (opcode_array[i].indirect << 16);

                        /* Swap byte order */
                        int swap = __bswap_16(result);

                        ret |= swap;

                        return ret;


                    }else if(opcode_array[i].absolute > 0){
                        /* Absolute addressing */

                        int ret = 0;

                        if(operand2){
                            if(!strcmp(operand2, "x")){
                                /* Shift the opcode up two bytes */
                                ret = opcode_array[i].absoluteX << 16;
                            }else if(!strcmp(operand2, "y")){
                                ret = opcode_array[i].absoluteY << 16;
                            }else{
                                kas65_log(LOG_ERROR, "Expected either x or y as operand");
                                return -1;
                            }
                        }else{
                            ret = opcode_array[i].absolute << 16;
                        }

                        /* Swap byte order */
                        int swap = __bswap_16(result);

                        ret |= swap;

                        return ret;
                    }else{
                        kas65_log(LOG_ERROR, "Unexpected two byte operand");
                        return -1;
                    }

                }else{ /* Operand is 1 byte */
                    if(opcode_array[i].relative != 0){

                        int result = parse_number(operand1, 1);

                        if(result < -128){
                            kas65_log(LOG_ERROR, "Operand too big");
                            printf("kas65: Expected operand no less than -128, got %d\n", result);
                        }else if(result > 127){
                            kas65_log(LOG_ERROR, "Operand too big");
                            printf("kas65: Expected operand no more than 127, got %d\n", result); 
                        }

                        int ret = opcode_array[i].relative << 8;

                        ret |= result;

                        return ret;
                        
                    }else if(opcode_array[i].zeropage != 0 && !operand2){
                        int result = parse_number(operand1, 1);

                        if(result < 0){
                            kas65_log(LOG_ERROR, "Invalid negative address");
                            printf("kas65: Valid addresses are from 0-255, got %d\n", result);
                            return -1;
                        }else if(result > 255){
                            kas65_log(LOG_ERROR, "Operand too big");
                            printf("kas65: Expected operand no more than 255, got %d\n", result); 
                        }

                        int ret = opcode_array[i].zeropage << 8;

                        ret |= result;

                        return ret;
                    }else if(opcode_array[i].zeropageX != 0 || opcode_array[i].zeropageY != 0){
                        if(!operand2){
                            kas65_log(LOG_ERROR, "Expected second operand");
                            return -1;
                        }

                        int result = parse_number(operand1, 1);

                        if(result < 0){
                            kas65_log(LOG_ERROR, "Invalid negative address");
                            printf("kas65: Valid addresses are from 0-255, got %d\n", result);
                            return -1;
                        }else if(result > 255){
                            kas65_log(LOG_ERROR, "Operand too big");
                            printf("kas65: Expected operand no more than 255, got %d\n", result); 
                        }

                        int ret = 0;

                        switch (operand2[0]) {
                            case 'x':
                                ret = opcode_array[i].zeropageX << 8;
                                break;
                            case 'y':
                                ret = opcode_array[i].zeropageY << 8;
                                break;
                            default:
                                kas65_log(LOG_ERROR, "Unexpected operand");
                                printf("kas65: Expected either X or Y as second operand, got %c\n", operand2[0]);
                                return -1;             
                        }

                        printf("hi\n");
                        
                        ret |= result;

                        return ret;                      
                    }else{
                        kas65_log(LOG_ERROR, "Unexpected one byte operand, expected atleast two bytes");
                        return -1;
                    }
                }
            }
        }
    }

    return -1;
}

/* Parses number from the buffer. Returns -1 if there is an error.
   `index` determines at which index in the buffer it should start parsing */
int parse_number(char *buffer, int index){
    char *nptr = malloc(sizeof(char) * OPERAND_CHAR);

    char **endptr = malloc(sizeof(char**));
    //*endptr = malloc(sizeof(char*));

    memset(nptr, 0, sizeof(char) * OPERAND_CHAR);

    /* Copy everything after the index */
    memcpy(nptr, buffer + index, sizeof(char) * OPERAND_CHAR - index);

    printf("buffer: %s\n", buffer + index);

    int result = strtol(nptr, endptr, 0);

    if(buffer[0+index] == '0' && buffer[1+index] != 'x'){
        kas65_log(LOG_WARN, "0 at the beginning of operand, interpreting as octal\n");
    }

    /* If not parsed correctly then throw error */
    if(!(**endptr == 0 && result != 0)){
        kas65_log(LOG_ERROR, "Expected number");
        printf("kas65: Unexpected character '%c'\n", **endptr);

        result = -1;

    }

    free(nptr);
    free(endptr);

    return result;

}

void str_to_lower(char *str){
    int i = 0;
    while(str[i] != 0){
        str[i] = tolower(str[i]);
        i++;
    }
}

/* Checks if a instruction is a builtin, if it is then parse it and change
    program state depending on that */
int parse_builtin(char *opcode, char *operand1, char *operand2){

}