dataServer.o: dataServer.cpp
	g++ -c dataServer.cpp

dataServer: dataServer.o dataServer.h
	g++ dataServer.o -pthread -o dataServer

remoteClient.o: remoteClient.c remoteClient.h
	gcc -c remoteClient.c

remoteClient: remoteClient.o
	gcc remoteClient.o -pthread -o remoteClient

clean:
	rm *.o remoteClient dataServer
