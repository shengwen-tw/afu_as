######################################
# Print Hello                        #
######################################
mov $'H', %al     #char to print
mov $0x0e, %ah    #set tty mode
int $0x10         #call interrupt

mov $'e', %al
mov $0x0e, %ah
int $0x10

mov $'l', %al
mov $0x0e, %ah
int $0x10

mov $'l', %al
mov $0x0e, %ah
int $0x10

mov $'o', %al
mov $0x0e, %ah
int $0x10

mov $'\n', %al
mov $0x0e, %ah
int $0x10

mov $'\r' , %al
mov $0x0e, %ah
int $0x10

######################################
# Print 1 + 2 = 3                    #
######################################
mov $'1', %al
mov $0x0e, %ah
int $0x10

mov $'+', %al
mov $0x0e, %ah
int $0x10

mov $'2', %al
mov $0x0e, %ah
int $0x10

mov $'=', %al
mov $0x0e, %ah
int $0x10

#Calculate 1 + 2
mov $1, %al
add $2, %al
add $'0', %al #Convert to ascii
mov $0x0e, %ah
int $0x10

mov $'\n', %al
mov $0x0e, %ah
int $0x10

mov $'\r', %al
mov $0x0e, %ah
int $0x10

######################################
# Print 8 - 7 = 1                    #
######################################
mov $'8', %al
mov $0x0e, %ah
int $0x10

mov $'-', %al
mov $0x0e, %ah
int $0x10

mov $'7', %al
mov $0x0e, %ah
int $0x10

mov $'=', %al
mov $0x0e, %ah
int $0x10

#Calculate 8 - 7
mov $8, %al
sub $7, %al
add $'0', %al #Convert to ascii
mov $0x0e, %ah
int $0x10

mov $'\n', %al
mov $0x0e, %ah
int $0x10

mov $'\r', %al
mov $0x0e, %ah
int $0x10
