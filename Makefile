O = api_core.o api_utils.o attribute.o autobuffer.o edcl.o main.o udp.o # elfloader.o stub_main.o
CPPFLAGS=-std=c++11 -g

tester: serve.o libedcl.a
	gcc -o $@ serve.o libedcl.a

libedcl.o: $O
	ld -r -o libedcl.o $O -L/usr/lib/gcc/x86_64-linux-gnu/5 -L/usr/lib/x86_64-linux-gnu -lstdc++

libedcl.a: libedcl.o
	rm -f $@
	ar rc $@ libedcl.o

clean:
	rm -f tester $O
