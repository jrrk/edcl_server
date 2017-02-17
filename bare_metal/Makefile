CC = g++ -g
O  = diskio.o elf.o ff.o memory.o minion_helper.o sdhci-minion-hash-md5.o selftest.o spi.o syscalls.o uart.o

selftest: $O ../client.o
	$(CC) -o $@ $O ../client.o
