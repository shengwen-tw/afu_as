#################################################################
AS=afu_as
TEST=hello

#################################################################
build:
	gcc -g -o ${AS} main.c

clean:
	rm ${AS} ${TEST}.bin ${TEST}.img

gdbauto:
	cgdb --args ${AS} ${TEST}.s ${TEST}.bin

objdump:
	objdump -D -Mintel,i8086 -b binary -m i386 ${TEST}.bin

#################################################################
test_as:
	./afu_as ${TEST}.s ${TEST}.bin

floppy:
	dd if=/dev/zero of=${TEST}.img bs=512 count=2880
	dd if=${TEST}.bin of=${TEST}.img

qemu:
	qemu-system-i386 -fda ${TEST}.img -boot a

#################################################################
