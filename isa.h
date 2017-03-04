#ifndef __ISA_H__
#define __ISA_H__

#define INSTRUCTION_LEN 10
#define MAX_CHAR_LINE 64
#define MAX_ARGS 5

#define DEF_INSTRUCTION(name) [_ ## name] = \
	{._name = #name, .func = name ## _ ## handler}

#define DEF_REGISTER(reg_name, reg_val) [reg_name] = \
	{.name = #reg_name, .value = reg_val} 

#define USE_DEBUG_PRINT 1
#if !USE_DEBUG_PRINT
	#define instruction_debug_print(...)
#endif

enum {
	_add,
	_sub,
	_mov,
	_int,
	INSTRUCTION_CNT	
} SUPPORT_INSTRUCTIONS;

enum {
	DIRECT_VALUE,
	REGISTER
} ARG_TYPE;

enum {
	ah,
	al,
	ax,
	bh,
	bl,
	bx,
	ch,
	cl,
	cx,
	dh,
	dl,
	dx,
	bp,
	si,
	di,
	sp,
	REG_CNT
} SUPPORT_REGISTER;

typedef struct {
	char *name;
	char value;
} reg_t;

enum {
	ADD16 = 0x01,
	ADD8 = 0x00,
	ADD_IMM16 = 0x05,
	ADD_IMM8 = 0x04,
	SUB16 = 0x29,
	SUB8 = 0x28,
	SUB_IMM16 = 0x2d,
	SUB_IMM8 = 0x2c,
	MOV16 = 0x89,
	MOV8 = 0x88,
	MOV_IMM16 = 0xb0,
	IMM8 = 0x80,
	INT_IMM8 = 0xcd
} OPCODE;

enum {
	 REG_INDIR_ADDR_MOD = 0x0, //Register indirect addressing mode
	 REG_ADDR_MOD = 0xc0 //Register addressing mode
} MOD; //2-bits addressing mode field

typedef struct {
	int type; //Direct value, register, etc...
	int value;
} instruction_arg_t;

typedef struct {
	char _name[INSTRUCTION_LEN];
	int (*func)(instruction_arg_t *args, int arg_cnt, char *machine_code);
} instruction_list_t;

#endif
