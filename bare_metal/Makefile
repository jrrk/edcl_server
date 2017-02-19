CC = g++ -g
O  = diskio.o elf.o ff.o memory.o minion_helper.o sdhci-minion-hash-md5.o selftest.o spi.o syscalls.o uart.o

selftest: $O minion_lib.o ../client.o
	$(CC) -o $@ $O minion_lib.o ../client.o

selftest2: $O queue.o ../libedcl.a
	$(CC) -o $@ $O queue.o ../libedcl.a
