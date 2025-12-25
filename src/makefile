LIBJALALIPATH = ../../jcal/sources/libjalali
LIBJALALI.a = ../../neojcal-build/libjalali/.libs/libjalali.a

irancal.exe: irancal.c ${LIBJALALI.a}
	gcc -I${LIBJALALIPATH} irancal.c ${LIBJALALI.a} -o irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32

debug: irancal.c ${LIBJALALI.a}
	gcc -I${LIBJALALIPATH} irancal.c ${LIBJALALI.a} -o debug_irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32 -DDEBUG
release: irancal.exe
	strip irancal.exe
clean:
	rm -f irancal.exe debug_irancal.exe

.PHONY: release debug clean
