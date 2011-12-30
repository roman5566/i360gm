
.SILENT:

all: Release/xextool

Release/xextool: xextool.c aes/aes.c mspack/lzxc.c mspack/lzxd.c mspack/system.c sha1/sha1.c
	gcc -O2 -W -Wall -o Release/xextool xextool.c aes/aes.c mspack/lzxc.c mspack/lzxd.c mspack/system.c sha1/sha1.c

test: Release/xextool
	./Release/xextool ./Release/xbdm.xex out.pe

.PHONY: clean zap

clean:
	rm -f ./Release/out.pe
	rm -f *~
	rm -f ./aes/*~
	rm -f ./mspack/*~
	rm -f ./msvc/*~
	rm -f ./Release/*~
	rm -f ./sha1/*~

zap: clean
	rm -f ./Release/xextool