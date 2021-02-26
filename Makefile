
all: client/Makefile server/Makefile
	make -C client
	make -C server

.PHONY: clean
clean:
	make -i -C client clean
	make -i -C server clean
