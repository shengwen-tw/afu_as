#include <stdio.h>
#include <string.h>

#include "isa.h"

int generate_binary(char *source_code, char *binary);
int parse_instruction(char *line_start, char *line_end, char *binary_code);

/* Instruction handlers */
int add_handler(instruction_arg_t *args, int arg_cnt, char *machine_code);
int sub_handler(instruction_arg_t *args, int arg_cnt, char *machine_code);
int mov_handler(instruction_arg_t *args, int arg_cnt, char *machine_code);
int int_handler(instruction_arg_t *args, int arg_cnt, char *machine_code);

/* Define supported instruction for this assembler */
instruction_list_t instruction_list[INSTRUCTION_CNT] = {
	DEF_INSTRUCTION(add),
	DEF_INSTRUCTION(sub),
	DEF_INSTRUCTION(mov),
	DEF_INSTRUCTION(int)	
};

/* Define supported register */
reg_t reg_list[] = {
	DEF_REGISTER(ah, 4),
	DEF_REGISTER(al, 0),
	DEF_REGISTER(ax, 0),

	DEF_REGISTER(bh, 7),
	DEF_REGISTER(bl, 3),
	DEF_REGISTER(bx, 3),

	DEF_REGISTER(ch, 5),
	DEF_REGISTER(cl, 1),
	DEF_REGISTER(cx, 1),

	DEF_REGISTER(dh, 6),
	DEF_REGISTER(dl, 2),
	DEF_REGISTER(dx, 2),

	DEF_REGISTER(bp, 5),
	DEF_REGISTER(si, 6),
	DEF_REGISTER(di, 7),
	DEF_REGISTER(sp, 4)
};

int main(int argc, char **argv)
{
	char src_memory_pool[4096] = {'\0'};
	char bin_memory_pool[512] = {'\0'};

	if(argc != 3) {
		printf("afu as: simple 16-bit x86 assembler\n"
			"===================================\n"
			"Usage: ./afu_as source binary\n"
			"===================================\n"
			"Support instructions:\n"
			"add, dec, mov, push, pop, int\n"
			"Support registers:\n"
			"ax, bx, cx, dx, bp, si, di, sp\n"
		);
		return 0;
	}

	FILE *source = fopen(argv[1], "r");
	if(source == NULL) {
		printf("afu_as: error: no such file\n");
		return 0;
	}

	fseek(source, 0, SEEK_END); //Seek to end and get char size
	long size = ftell(source);
	fseek(source, 0L, SEEK_SET); //Seek back to begin

	//Read assembly language source code
	fread(src_memory_pool, sizeof(char), size, source);

	size_t write_size = generate_binary(src_memory_pool, bin_memory_pool);
	if(write_size != -1) {
		//Succeed to generate the binary file
		FILE *binary = fopen(argv[2], "wb");
		fwrite(bin_memory_pool, sizeof(char), 512/*write_size*/, binary);
		fclose(binary);
	}

	fclose(source);

	return 0;
}

int generate_binary(char *source_code, char *binary)
{
	int bin_pos = 0; //The position to write the machine code
	char *line_start = source_code;
	char *line_end = source_code;

	while(1) {
		/* Search the end of code line */
		char *line_end = strchr(line_start, '\n');
		char machine_code[15]; //Max machine code size for x86 is 15

		//Skip empty line
		if((line_end - line_start) == 0) {
			line_start++;
			continue;
		}

		/* According to POSIX standard, the last byte before EOF
		   should be \n indicate it is not a binary file */
		if(line_end == NULL) {
			//Reach the end of the source code, leave
			break;
		}

		*line_end = '\0';

		/* Parse instruction and generate machine code */
		int bin_append_size = parse_instruction(line_start, line_end, machine_code);
		if(bin_append_size == -1) {
			//Error occured, leave
			return -1;
		}

		/* Append the machine code to the binary file */
		strncpy(binary + bin_pos, machine_code, bin_append_size);
		bin_pos += bin_append_size;

		line_start = line_end + 1;
	}

	/* Add boot signature */
	binary[510] = 0x55;
	binary[511] = 0xaa;

	return bin_pos;
}

/* Split a token of the instruction and return the address of next token */
static char *split_token(char *token, char *instruction, int *size)
{
	int i = 0;

	/* Ignore spaces before first token */
	while(instruction[i] == ' ') {
		i++;
	}

	instruction += i;

	/* Start to split one token */
	for(i = 0; i < *size; i++) {
		if(instruction[i] == ' ') {
			strncpy(token, instruction, i);
			token[i + 1] = '\0';

			//Read all the space until next token
			while(instruction[i + 1] == ' ' && i < *size) {
				i++;
			}

			*size -= i + 1;

			return instruction + i + 1;
		}
	}

	//There is only one token in this instruction
	strncpy(token, instruction, i);
	token[i + 1] = '\0';
}

static void remove_comment_and_space(char *old_str, char *new_str)
{
	int new_str_len = 0;

	int i;
	for(i = 0; i < strlen(old_str); i++) {
		//Ignore space
		if(old_str[i] != ' ') {
			//Ignore comment and return the new string
			if(old_str[i] == '#') {
				new_str[new_str_len] = '\0';
				return;
			} else {
				new_str[new_str_len] = old_str[i];
				new_str_len++;
			}
		}
	}

	new_str[new_str_len] = '\0';
}

static int split_arguments(char *str, char (*args)[MAX_CHAR_LINE], int *arg_cnt)
{
	char new_str[MAX_CHAR_LINE];
	remove_comment_and_space(str, new_str);

	size_t str_len = strlen(new_str);

	if(str_len == 0) {
		*arg_cnt = 0;
		return 0;
	}

	*arg_cnt = 0;
	int cur_i = 0; //Current argument's index

	int i;
	for(i = 0; i < str_len; i++) {
		if(new_str[i] == ',') {
			args[*arg_cnt][cur_i] = '\0';
			(*arg_cnt)++;
			cur_i = 0;
		} else {
			args[*arg_cnt][cur_i] = new_str[i];
			cur_i++;
		}
	}

	args[*arg_cnt][cur_i] = '\0'; //Padding end null symbol  for last arument

	(*arg_cnt)++;
}

static int parse_arguments_str(char (*args_in_str)[MAX_CHAR_LINE],
	instruction_arg_t *args, int arg_cnt)
{
	int i;
	for(i = 0; i < arg_cnt; i++) {
		if(args_in_str[i][0] == '$') {
			//Direct value
			args[i].type = DIRECT_VALUE;

			int decimal_number;

			/* Check if it is char */
			if(args_in_str[i][1] == '\'') {
				if(args_in_str[i][2] == '\\') {
					if(args_in_str[i][3] == 'n') {
						args[i].value = (int)'\n';
						continue;
					} else if(args_in_str[i][3] == 'r') {
						args[i].value = (int)'\r';
						continue;
					} else {
						printf("afu_as: error: unsupported escape character");
						return 1;
					}
				} else if(args_in_str[i][3] == '\'') {
					args[i].value = (int)args_in_str[i][2];
					continue;
				}
			}

			if(args_in_str[i][1] == '0') {
				if(args_in_str[i][2] == 'x' || args_in_str[i][2] == 'X') {
					/* Is hexadecimal number (0x) */
					if(sscanf(args_in_str[i] + 1, "%x", &decimal_number) != EOF) {
						//Hexadecimal number
						args[i].value = decimal_number;
						continue;
					}
				} else {
					/* Is octal number (0) */
					if(sscanf(args_in_str[i] + 1, "%o", &decimal_number) != EOF) {
						//Octal number
						args[i].value = decimal_number;
						continue;
					}
				}
			}

			if(sscanf(args_in_str[i] + 1, "%d", &decimal_number) != EOF) {
				//Is decimal number
				args[i].value = decimal_number;
				continue;
			}

			//Unknown type value
			printf("afu_as: error: invalid number\n");
			return 1;			
		} else if(args_in_str[i][0] == '%') {
			//Register
			args[i].type = REGISTER;

			int match = 0;

			/* Parse register name */
			int j;
			for(j = 0; j < REG_CNT; j++) {
				if(strcmp(reg_list[j].name, args_in_str[i] + 1) == 0) {
					args[i].value = j;
					match = 1;
				}
			}

			/* Unknown register name */
			if(match == 0) {
				printf("afu_as: error: unknown register name\n");
				return 1;
			}
		} else {
			//Unknown
			printf("afu_as: error: unknown instruction argument\n");

			return 1;
		}
	}

	return 0;
}

#if USE_DEBUG_PRINT
static void instruction_debug_print(
	char *name, char (*args)[MAX_CHAR_LINE],
	instruction_arg_t *instruction_args,
	int arg_cnt, char *machine_code,
	int machine_code_size)
{
	char buf[4096];
	int offset = 0;
	int i;

#if 1   /* Print instruction and machine code*/

	//Instruction
	offset += sprintf(buf, "%s ", name);
	for(i = 0; i < arg_cnt; i++) {
		offset += sprintf(buf + offset, "%s", args[i]);
		if(i + 1 < arg_cnt) {
			offset += sprintf(buf + offset, ",");
		}
	}

	//Machine code
	offset += sprintf(buf + offset, " \t");

	for(i = 0; i < machine_code_size; i++) {
		offset += sprintf(buf + offset, " %02x", machine_code[i] & 0xff); //0xff, cast to int
	}

	printf("%s\n", buf);
#endif

#if 0   /* Print argument's type and value */
	offset = 0;

	offset += sprintf(buf, " |_ [%s]", name);

	if(arg_cnt == 0) {
		offset += sprintf(buf + offset, " no argument");
	}

	//Arguments
	for(i = 0; i < arg_cnt; i++) {
		offset += sprintf(buf + offset, "(type:%d, value:%d)",
			instruction_args[i].type, instruction_args[i].value);
	}

	printf("%s\n", buf);
#endif
}
#endif

int is_8bit_reg(int reg)
{
	if(reg == ah || reg == al || reg == bh || reg == bl ||
	   reg == ch || reg == cl || reg == dh || reg == dl) {
		return 1;
	}

	return 0;
}

int is_16bit_reg(int reg)
{
	if(reg == ax || reg == bx || reg == cx || reg == dx ||
	   reg == bp || reg == si || reg == di || reg == sp) {
		return 1;
	}

	return 0;
}

/* Parse one line of assembly code and generate the machine code, the function
   returns the machine code size */
int parse_instruction(char *line_start, char *line_end, char *machine_code)
{
	char *line_ptr = line_start;
	char first_token[MAX_CHAR_LINE] = {'\0'};
	int line_size = line_end - line_start;

	line_ptr = split_token(first_token, line_start, &line_size);

	int arg_cnt = 0;
	char splited_args[MAX_CHAR_LINE][MAX_CHAR_LINE];
	instruction_arg_t instruction_args[MAX_ARGS];

	/* Identify the instruction type and call its handler */
	int i = 0;
	for(i = 0; i < INSTRUCTION_CNT; i++) {
		if(strcmp(instruction_list[i]._name, first_token) == 0) {
			//Split and parse arguments
			split_arguments(line_ptr, splited_args, &arg_cnt);
			if(parse_arguments_str(splited_args, instruction_args, arg_cnt)) {
				return -1; //Failed to parse to argument
			}

			/* Call instruction handler and pass the argument, the handler will generate
			   the machine code and return its size */
			int size = instruction_list[i].func(instruction_args, arg_cnt, machine_code);

			//Debug print
			if(size != -1) {
				instruction_debug_print(first_token, splited_args,
					instruction_args, arg_cnt, machine_code, size);
			}

			return size;
		}
	}

	if(first_token[0] == '#') {
		return 0; //Ignore this line, it is a comment
	}

	printf("afu_as: error: invaild instruction \"%s\"\n", first_token);

	return -1;
}

int add_handler(instruction_arg_t *args, int arg_cnt, char *machine_code)
{
	if(arg_cnt > 2) {
		printf("afu_as: error: too many argument for \"add\" instruction\n");
		return -1;
	} else if (arg_cnt < 2) {
		printf("afu_as: error: too few argument for \"add\" instruction\n");
		return -1;
	}

	/* Register addressing mode */
	if(args[0].type == REGISTER & args[1].type == REGISTER) {
		/* Decide opcode */
		if(is_8bit_reg(args[0].value) && is_8bit_reg(args[1].value)) {
			//8-bit size data
			machine_code[0] = ADD8; //Opcode, s-bit = 1
		} else if(is_16bit_reg(args[0].value) && is_16bit_reg(args[1].value)) {
			//16-bit size data
			machine_code[0] = ADD16; //Opcode, s-bit = 0
		} else {
			printf("afu_as: error: operands size are different passed"
				"through \"add\" instruction\n");
			return -1;
		}

		machine_code[1] = 0;
		machine_code[1] |= (char)REG_ADDR_MOD; //Set addressing mode
		machine_code[1] |= (char)reg_list[args[1].value].value; //Set destination register
		machine_code[1] |= (char)reg_list[args[0].value].value << 3; //Set source register

		return 2;
	/* Immediate value */
	} else if(args[0].type == DIRECT_VALUE && args[1].type == REGISTER) {
		if(args[1].value == al) {
			//8-bits size data
			machine_code[0] = ADD_IMM8;

			//Only 8-bit
			machine_code[1] = (char)args[0].value;

			return 2;
		} else if(args[1].value == ax) {
			//16-bit size data
			machine_code[0] = ADD_IMM16;

			/* Move 16-bits data in little endian */
			machine_code[1] = args[0].value & 0xff; //Lowest 8-bits
			machine_code[2] = (args[0].value >> 8) & 0xff; //Highest 8 byte

			return 3;
		}

		printf("afu_as: error: \"add\" instructuion can only subtract an immediate value"
			" to al/ax register\n");

		return -1;
	}

	printf("afu_as: error: unsupported or invalid instruction operand for \"add\" instruction\n");
	return -1;
}

int sub_handler(instruction_arg_t *args, int arg_cnt, char *machine_code)
{
	if(arg_cnt > 2) {
		printf("afu_as: error: too many argument for \"mov\" instruction\n");
		return -1;
	} else if (arg_cnt < 2) {
		printf("afu_as: error: too few argument for \"mov\" instruction\n");
		return -1;
	}

	/* Register addressing mode */
	if(args[0].type == REGISTER && args[1].type == REGISTER) {
		/* Decide opcode */
		if(is_8bit_reg(args[0].value) && is_8bit_reg(args[1].value)) {
			//8-bit size data
			machine_code[0] = SUB8; //Opcode, s-bit = 1
		} else if(is_16bit_reg(args[0].value) && is_16bit_reg(args[1].value)) {
			//16-bit size data
			machine_code[0] = SUB16; //Opcode, s-bit = 0
		} else {
			printf("afu_as: error: operands size are different passed"
				"through \"sub\" instruction\n");
			return -1;
		}

		machine_code[1] = 0;
		machine_code[1] |= (char)REG_ADDR_MOD; //Set addressing mode
		machine_code[1] |= (char)reg_list[args[1].value].value; //Set destination register
		machine_code[1] |= (char)reg_list[args[0].value].value << 3; //Set source register

		return 2;
	/* Immediate value */
	} else if(args[0].type == DIRECT_VALUE && args[1].type == REGISTER) {
		if(args[1].value == al) {
			//8-bits size data
			machine_code[0] = SUB_IMM8;

			//Only 8-bit
			machine_code[1] = (char)args[0].value;

			return 2;
		} else if(args[1].value == ax) {
			//16-bit size data
			machine_code[0] = SUB_IMM16;

			/* Move 16-bits data in little endian */
			machine_code[1] = args[0].value & 0xff; //Lowest 8-bits
			machine_code[2] = (args[0].value >> 8) & 0xff; //Highest 8 byte

			return 3;
		}

		printf("afu_as: error: \"sub\" instructuion can only subtract an immediate value to"
			" al/ax register\n");

		return -1;
	}

	printf("afu_as: error: unsupported or invalid instruction operand for \"sub\" instruction\n");
	return -1;
}

int mov_handler(instruction_arg_t *args, int arg_cnt, char *machine_code)
{
	if(arg_cnt > 2) {
		printf("afu_as: error: too many argument for \"mov\" instruction\n");
		return -1;
	} else if (arg_cnt < 2) {
		printf("afu_as: error: too few argument for \"mov\" instruction\n");
		return -1;
	}

	/* Register addressing mode */
	if(args[0].type == REGISTER && args[1].type == REGISTER) {
		/* Decide opcode */
		if(is_8bit_reg(args[0].value) && is_8bit_reg(args[1].value)) {
			//8-bit size data
			machine_code[0] = MOV8; //Opcode, s-bit = 1
		} else if(is_16bit_reg(args[0].value) && is_16bit_reg(args[1].value)) {
			//16-bit size data
			machine_code[0] = MOV16; //Opcode, s-bit = 0
		} else {
			printf("afu_as: error: operands size are different passed"
				"through \"add\" instruction\n");
			return -1;
		}

		machine_code[1] = 0;
		machine_code[1] |= (char)REG_ADDR_MOD; //Set addressing mode
		machine_code[1] |= (char)reg_list[args[1].value].value; //Set destination register
		machine_code[1] |= (char)reg_list[args[0].value].value << 3; //Set source register

		return 2;
	/* Immediate value */
	} else if(args[0].type == DIRECT_VALUE && args[1].type == REGISTER) {
		if(is_8bit_reg(args[1].value)) {
			//w-bit = 0
			machine_code[0] = MOV_IMM16 | reg_list[args[1].value].value;

			/* Only 8-bits data */
			machine_code[1] = (char)args[0].value;

			return 2;
		} else if(is_16bit_reg(args[1].value)) {
			//w-bit = 1
			machine_code[0] = MOV_IMM16 | reg_list[args[1].value].value | 0x8;

			/* Move 16-bits data in little endian */
			machine_code[1] = args[0].value & 0xff; //Lowest 8-bits
			machine_code[2] = (args[0].value >> 8) & 0xff; //Highest 8 byte

			return 3;
		} else {
			printf("afu_as: error: invalid data size\n");
			return -1;
		}
	}

	printf("afu_as: error: unsupported or invalid instruction operand for \"mov\" instruction\n");
	return -1;
}

int int_handler(instruction_arg_t *args, int arg_cnt, char *machine_code)
{
	if(arg_cnt > 1) {
		printf("afu_as: error: too many argument for \"int\" instruction\n");
		return -1;
	} else if (arg_cnt < 1) {
		printf("afu_as: error: too few argument for \"int\" instruction\n");
		return -1;
	}

	machine_code[0] = INT_IMM8;
	machine_code[1] = (char)args[0].value;

	return 2;
}
