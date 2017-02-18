O = edcl.o main.o udp.o # api_core.o api_utils.o attribute.o autobuffer.o  # elfloader.o stub_main.o
CPPFLAGS=-std=c++11 -g

tester: serve.o libedcl.a client.o
	gcc -o $@ serve.o libedcl.a

libedcl.a: $O
	rm -f $@
	ar rc $@ $O

libclient.o: client.o
	ld -r -o libclient.o client.o

libclient.a: libclient.o
	rm -f $@
	ar rc $@ libclient.o

clean:
	rm -f tester $O
