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

kas65_line parse_line(kas65_directive *list, char **buffer, int *directive_count, int current_line, int current_address, bool directive_parsing){
    remove_whitespace(buffer[current_line]);

    printf("buffer: %s\n", buffer[current_line]);

    if(buffer[current_line][0] == ';'){
        /* Comment */
        return (kas65_line){NULL, NULL, NULL};
    }

    char *opcode = malloc(sizeof(char) * OPCODE_CHAR+1);
    char *operand1 = malloc(sizeof(char) * OPERAND_CHAR+1);
    char *operand2 = malloc(sizeof(char) * OPERAND_CHAR+1);
    
    memcpy(opcode, buffer[current_line], OPCODE_CHAR);

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

    while((buffer[current_line][i] != '\n' || buffer[current_line][i] != '\n') && i < 4096){

        /* ; means comment, so skip anything after the comment */
        if(buffer[current_line][i] == ';'){
            break;
        }

        /* NUL terminator */
        if(buffer[current_line][i] == 0){
            break;
        }

        /* If there is a , that means that there is a second operand */
        if(buffer[current_line][i] == ','){
            second_operand = true;
            i++;
            continue;
        }

        
        if(directive_parsing){
            /* {} is for dereferencing a directive */
            if(buffer[current_line][i] == '{'){
                int x = i + 1;  // Buffer[current_line] index (starts at i + 1 for the { opening bracket)
                int y = 0;  // Name index
                char *directive_name = malloc(DIRECTIVE_CHAR * sizeof(char));
                memset(directive_name, 0, DIRECTIVE_CHAR * sizeof(char));
                while (buffer[current_line][x] != '}') {

                    if(buffer[current_line][x] == '\0'){
                        kas65_log(LOG_ERROR, "Directive dereference is not ended (missing })");
                        return (kas65_line){"err", "err", "err"};
                    }

                    if(y > DIRECTIVE_CHAR){
                        printf("kas65: Directive name is too long (over %d characters)\n", DIRECTIVE_CHAR);
                        return (kas65_line){"err", "err", "err"};                    
                    }

                    directive_name[y] = buffer[current_line][x];

                    y++;
                    x++;
                }

                printf("got name: %s\n", directive_name);

                int z = 0; // Directive index

                for(int i = 0; i < MAX_DIRECTIVE; i++){
                    printf("list[%d].name: %s\n", i, list[i].name);
                }

                bool found = false;
                while(list[z].name){
                    if(!strcmp(directive_name, list[z].name)){
                        /* Set that part of the buffer[current_line] to zero */
                        memset(buffer[current_line] + i, 0, y);

                        char *str = malloc(sizeof(char) * 32);

                        /* Convert into a string */
                        snprintf(str, 32, "0x%x", list[z].value);

                        /* Then overwrite the name with the constant */
                        memcpy(buffer[current_line] + i, str, 32);

                        free(str);

                        found = true;
                    }
                    z++;
                }

                if(!found){
                    printf("kas65: Undefined directive '%s'\n", directive_name);
                    return (kas65_line){"lbl", "err", "err"};
                }
            }

            /* [] is for dereferencing a label */
            if(buffer[current_line][i] == '['){
                int x = i + 1;  // Buffer[current_line] index (starts at i + 1 for the [ opening bracket)
                int y = 0;  // Name index
                char *label_name = malloc(DIRECTIVE_CHAR * sizeof(char));
                memset(label_name, 0, DIRECTIVE_CHAR * sizeof(char));
                while (buffer[current_line][x] != ']') {

                    if(buffer[current_line][x] == '\0'){
                        kas65_log(LOG_ERROR, "Label dereference is not ended (missing ])");
                        return (kas65_line){"err", "err", "err"};
                    }

                    if(y > DIRECTIVE_CHAR){
                        printf("kas65: Label name is too long (over %d characters)\n", DIRECTIVE_CHAR);
                        return (kas65_line){"err", "err", "err"};                    
                    }

                    label_name[y] = buffer[current_line][x];

                    y++;
                    x++;
                }
                printf("name:\n");
                for(int i = 0; i < strlen(label_name); i++){
                    printf("%d: %d\n", i, label_name[i]);
                }
                printf("got name: %s\n", label_name);

                int z = 0; // Directive index
                bool found = false;
/*                 for(int i = 0; i < 100; i++){
                    printf("list[%d].name: %s\n", i, list[i].name);
                } */
                while(list[z].name){
                    if(!strcmp(label_name, list[z].name)){
                        /* Set that part of the buffer[current_line] to zero */
                        memset(buffer[current_line] + i, 0, y);

                        char *str = malloc(sizeof(char) * 32);

                        /* Convert into a string */
                        snprintf(str, 32, "$0x%x", list[z].value);

                        /* Then overwrite the name with the address */
                        memcpy(buffer[current_line] + i, str, 32);

                        free(str);

                        found = true;
                    }
                    z++;
                }

                if(!found){
                    int x = current_line + 1;
                    int addr = current_address;
                    kas65_line info = {0};
                    bool found_2 = false;
                    /* Check the rest of the file for the label */
                    while (x < MAX_FILE_LINES) {
                        info = parse_line(list, buffer, directive_count, x, addr, false);

                        if(!strcmp(info.opcode, "")){
                            break;
                        }

                        if(buffer[x][0] == '.'){
                            if(!info.operand1){
                                continue;
                            }

                            /* Temporary string for manipulation */
                            char *temp_operand1 = malloc(sizeof(char) * OPERAND_CHAR);

                            memcpy(temp_operand1, info.operand1, OPERAND_CHAR);

                            /* Remove the : at the end */
                            temp_operand1[strlen(temp_operand1) - 1] = 0;

                            printf("temp_operand1: %s\noperand1: %s\n", temp_operand1, info.operand1);

                            if(!strcmp(label_name, temp_operand1)){
                                parse_labels_constants(&addr, directive_count, list, info.opcode, info.operand1, info.operand2);
                                int y = 0;
                                while (list[y].name) {
                                    printf("directive: %s\naddr: %d\nvalue: %d\n", list[y].name, list[y].addr, list[y].value);
                                    y++;
                                }

                                while(list[z].name){
                                    if(!strcmp(label_name, list[z].name)){
                                        /* Set that part of the buffer[current_line] to zero */
                                        memset(buffer[current_line] + i, 0, y);

                                        char *str = malloc(sizeof(char) * 32);

                                        /* Convert into a string */
                                        snprintf(str, 32, "$0x%x", list[z].value);

                                        /* Then overwrite the name with the address */
                                        memcpy(buffer[current_line] + i, str, 32);

                                        free(str);

                                        found = true;
                                    }
                                    z++;
                                }

                                found_2 = true;

                            }
                        }

                        printf("opcode:%s\nop1:%s\nop2:%s\n", info.opcode, info.operand1, info.operand2);

                        int ret = get_instruction_size(info.opcode, info.operand1, info.operand2);

                        if(ret == -1){
                            kas65_log(LOG_ERROR, "Failed to get instruction size");
                            return (kas65_line){"err", "err", "err"};
                        }

                        addr += ret;
                        x++;
                    }

                    if(!found_2){
                        printf("kas65: Undefined label '%s'\n", label_name);
                        return (kas65_line){"err", "err", "err"};
                    }
                }
            }
        }

        if(second_operand == false){

            if(j > OPERAND_CHAR){
                printf("kas65: First operand length is over limit (%d)\n", OPERAND_CHAR);
                return (kas65_line){"err", "err", "err"};
            }

            operand1[j] = buffer[current_line][i];
            j++;

        }else{

            if(k > OPERAND_CHAR){
                printf("kas65: Second operand length is over limit (%d)\n", OPERAND_CHAR);
                return (kas65_line){"err", "err", "err"};
            }

            operand2[k] = buffer[current_line][i];

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

    printf("opcode: %s\noperand1: %s\noperand2: %s\n", opcode, operand1, operand2);

    return (kas65_line){opcode, operand1, operand2};
}

/* Checks instruction for syntax errors */
int check_instruction(char *opcode, char *operand1, char *operand2){

    if(opcode[0] == '%' || opcode[0] == '.'){
        return -1;
    }

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

            if(temp_operand1[0] != '$' && temp_operand1[0] != '(' && opcode_array[i].implied == 0 && opcode_array[i].relative == 0){
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
                                            ||   opcode_array[i].absoluteY != 0
                                            ||   opcode_array[i].zeropage != 0 || opcode_array[i].zeropageX != 0
                                            ||   opcode_array[i].zeropageY != 0)) && opcode_array[i].immediate == 0 && opcode_array[i].indirect == 0 && opcode_array[i].relative == 0){

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

            if(temp_operand1[0] != '@' && opcode_array[i].relative != 0){
                kas65_log(LOG_ERROR, "Expected @ to denote relative addressing");
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
        return -2;
}

/* Returns the instruction in machine code */
int get_instruction_binary(kas65_directive *directives, int current_address, char *opcode, char *operand1, char *operand2){
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

            if(opcode_array[i].relative != 0 && operand1 && operand1[0] == '@'){

                int result = parse_number(operand1, 2, 16);

                if(result < -128){
                    kas65_log(LOG_ERROR, "Operand too big");
                    printf("kas65: Expected operand no less than -128, got %d\n", result);
                }else if(result > 127){
                    kas65_log(LOG_ERROR, "Operand too big");
                    printf("kas65: Expected operand no more than 127, got %d\n", result); 
                }

                int ret = opcode_array[i].relative << 8;

                int8_t resulter = result - current_address;

                printf("resulter: 0x%hhx\n", resulter);

                /* Make sure signed representation is correct */
                uint64_t le = ret | (uint8_t)resulter;

                return le;

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

                int result = parse_number(temp_operand1, 2, 16);

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

                int result = parse_number(operand1, 2, 16);

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
                int result = parse_number(operand1, 1, 16);

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
                    if(opcode_array[i].zeropage != 0 && !operand2 && operand1[0] == '%'){
                        int result = parse_number(operand1, 1, 16);

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
                    }else if((opcode_array[i].zeropageX != 0 || opcode_array[i].zeropageY != 0) && operand1[0] == '%'){
                        if(!operand2){
                            kas65_log(LOG_ERROR, "Expected second operand");
                            return -1;
                        }

                        int result = parse_number(operand1, 1, 16);

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
                    }else if(opcode_array[i].absolute != 0 && operand1[0] != '%'){
                        /* One byte absolute addressing */
                        int ret = 0;

                        if(result < 0){
                            kas65_log(LOG_ERROR, "Invalid negative address");
                            printf("kas65: Valid addresses are from 0-255, got %d\n", result);
                            return -1;
                        }else if(result > 255){
                            kas65_log(LOG_ERROR, "Operand too big");
                            printf("kas65: Expected operand no more than 255, got %d\n", result); 
                        }

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
                        kas65_log(LOG_ERROR, "Unexpected one byte operand, expected atleast two bytes");
                        return -1;
                    }
                }
            }
        }
    }
    kas65_log(LOG_ERROR, "Invalid opcode");
    return -1;
}

/* Parses number from the buffer. Returns -1 if there is an error.
   `index` determines at which index in the buffer it should start parsing */
int parse_number(char *buffer, int index, int radix){
    char *nptr = malloc(sizeof(char) * OPERAND_CHAR);

    char **endptr = malloc(sizeof(char**));

    memset(nptr, 0, sizeof(char) * OPERAND_CHAR);

    /* Copy everything after the index */
    memcpy(nptr, buffer + index, sizeof(char) * OPERAND_CHAR - index);

    printf("buffer: %s\n", buffer + index);

    int result = strtol(nptr, endptr, radix);

    if(buffer[0+index] == '0' && buffer[1+index] != 'x' && radix == 0){
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

/* Parses all the labels and constants and puts them in `list` */
int parse_labels_constants(int *current_address, int *index, kas65_directive *list, char *opcode, char *operand1, char *operand2){
    if(!opcode){
        return -1;
    }

    if(opcode[0] != '%' && opcode[0] != '.'){
        return -1;
    }

    /* Check if its a directive with no name (db directive)*/
    if(!strcmp(opcode + 1, "db")){
        return -1;
    }

    /* Label */
    if(opcode[0] == '.'){
        /* Check if this is a label directive */
        if(!strcmp(opcode + 1, "lb")){
            operand1[strlen(operand1) - 1] = 0; // remove the ':' at the end of the name
            printf("hi name: %s\n", operand1);
            int z = 0;
            /* Make sure the label doesn't already exist */
            while (list[z].name) {
                if(!strcmp(list[z].name, operand1)){
                    return 0;
                }
                z++;
            }
            list[*index].name = operand1;
            list[*index].value = *current_address;
            list[*index].addr = *current_address;
        }else{
            kas65_log(LOG_ERROR, "Invalid label statement");
            printf("kas65: Expected 'lb', got '%s'\n", opcode + 1);
            return -2;
        }
    }else{
        /* Directives */
        if(!strcmp(opcode + 1, "bc")){
            /* Byte constant */
            list[*index].name = operand1;
            list[*index].addr = *current_address;
            int value = parse_number(operand2, 0, 0);
            if(value == -1){
                return -2;
            }

            if(value > 0xff){
                kas65_log(LOG_ERROR, "Byte value too big");
                printf("kas65: Expected value no more than 0xff, got 0x%x\n", value);
                return -2;
            }

            list[*index].value = value;
        }else if(!strcmp(opcode + 1, "wc")){
            /* Word constant */
            list[*index].name = operand1;
            list[*index].addr = *current_address;
            int value = parse_number(operand2, 0, 0);
            if(value == -1){
                return -2;
            }

            if(value > 0xffff){
                kas65_log(LOG_ERROR, "Word value too big");
                printf("kas65: Expected value no more than 0xffff, got 0x%x\n", value);
                return -2;
            }

            list[*index].value = value;
        }else if(!strcmp(opcode + 1, "tc")){
            /* Tribyte constant */
            list[*index].name = operand1;
            list[*index].addr = *current_address;
            int value = parse_number(operand2, 0, 0);
            if(value == -1){
                return -2;
            }

            if(value > 0xffffff){
                kas65_log(LOG_ERROR, "Tribyte value too big");
                printf("kas65: Expected value no more than 0xffffff, got 0x%x\n", value);
                return -2;
            }

            list[*index].value = value;
        }else if(!strcmp(opcode + 1, "og")){
            /* Origin directive - doesn't get put into the list */
            int value = parse_number(operand1, 0, 0);

            if(value == -1){
                return -2;
            }

            if(value > 0xffff){
                kas65_log(LOG_ERROR, "Origin value too big");
                printf("kas65: Expected value no more than 0xffff, got 0x%x\n", value);
                return -2;
            }else if(value < 0){
                kas65_log(LOG_ERROR, "Invalid origin value");
                printf("kas65: Expected positive value, got %d\n", value);
                return -2;              
            }

            *current_address = value;
        }else{
            printf("kas65: Invalid directive '%s'\n", opcode + 1);
            return -2;
        }
    }
    return 0;
}

/* Returns the size of an instruction in bytes */
int get_instruction_size(char *opcode, char *operand1, char *operand2){

    if(opcode[0] == '.'){
        return 0;
    }

    if(opcode[0] == '%'){
        if(!strcmp("db", opcode + 1)){
            int num = parse_number(operand2, 0, 0);

            if(num == -1){
                kas65_log(LOG_ERROR, "Failed to parse db operand\n");
                return -1;
            }

            if(num > 0xFFFF){
                return 3;
            }else if(num > 0xFF){
                return 2;
            }else{
                return 1;
            }
        }else{
            return 0;
        }
    }

    /* Implied */
    if(!operand1){
        return 1;
    }

    /* Accumulator */
    if(operand1[0] == 'a'){
        return 1;
    }

    /* Immediate */
    if(operand1[0] == '#'){
        return 2;
    }

    /* Absolute */
    if(operand1[0] == '$' || operand1[0] == '['){
        return 3;
    }

    if(operand1[0] == '(' && operand2){
        /* Indirect x, y indexed*/
        return 2;
    }else if(operand1[0] == '('){
        /* Indirect */
        return 3;
    }

    /* Relative */
    if(operand1[0] == '@'){
        return 2;
    }

    /* Zeropage */
    if(operand1[0] == '%'){
        return 2;
    }

    /* Not found */
    return -1;
}