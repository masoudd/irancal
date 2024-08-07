LIBJALALIPATH = ../../jcal/sources/libjalali
LIBJALALI.a = ${LIBJALALIPATH}/.libs/libjalali.a

irancal.exe: irancal.c ${LIBJALALI.a}
	gcc -I${LIBJALALIPATH} irancal.c ${LIBJALALI.a} -o irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32

release: irancal.exe
	strip irancal.exe

.PHONY: release
