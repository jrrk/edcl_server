O = edcl.o main.o udp.o # api_core.o api_utils.o attribute.o autobuffer.o  # elfloader.o stub_main.o
CPPFLAGS=-std=c++11 -g

tester: serve.o libedcl.a client.o
	gcc -o $@ serve.o libedcl.a

libedcl.o: $O
	ld -r -o libedcl.o $O -L/usr/lib/gcc/x86_64-linux-gnu/4.8 -L/usr/lib/x86_64-linux-gnu -lstdc++

libedcl.a: libedcl.o
	rm -f $@
	ar rc $@ libedcl.o

libclient.o: client.o
	ld -r -o libclient.o client.o

libclient.a: libclient.o
	rm -f $@
	ar rc $@ libclient.o

clean:
	rm -f tester $O
