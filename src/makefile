LIBJALALIPATH = ../../jcal/sources/libjalali
LIBJALALI.a = ../../neojcal-build/libjalali/.libs/libjalali.a

irancal.exe: irancal.c ${LIBJALALI.a}
	gcc -O2 -I${LIBJALALIPATH} irancal.c ${LIBJALALI.a} -o irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32 -mwin32 -mwindows

release: irancal.exe
	strip irancal.exe

debug: irancal.c ${LIBJALALI.a}
	gcc -I${LIBJALALIPATH} irancal.c ${LIBJALALI.a} -o irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32 -mwin32 -mwindows -DDEBUG

.PHONY: release
