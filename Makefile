all: httpc-test 

httpc-test: httpc.o

clean:
	rm -f httpc-test rssfs *.o
