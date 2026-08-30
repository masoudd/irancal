LIBJALALIPATH = ../../jcal/sources/libjalali
LIBJALALI.a = ../../neojcal-build/libjalali/.libs/libjalali.a

irancal.exe: irancal.c irancal.res ${LIBJALALI.a}
	gcc -std=c11 -O2 -I${LIBJALALIPATH} irancal.c irancal.res ${LIBJALALI.a} -o irancal.exe -municode -Wall -Os -Wl,--subsystem,windows -lgdi32 -mwin32 -mwindows

irancal.res: irancal.rc
	windres -O coff -i irancal.rc -o irancal.res

debug: irancal.c irancal.res ${LIBJALALI.a}
	gcc -std=c11 -I${LIBJALALIPATH} irancal.c irancal.res ${LIBJALALI.a} -o debug_irancal.exe -municode -Wall -g -Og -Wl,--subsystem,windows -lgdi32 -DDEBUG
release: irancal.exe
	strip irancal.exe
clean:
	rm -f irancal.exe debug_irancal.exe irancal.res

.PHONY: release debug clean
