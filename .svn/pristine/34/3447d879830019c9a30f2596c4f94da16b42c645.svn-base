CFLAGS = -Wall -pedantic -std=gnu99 -ggdb

all: peer tracker

peer: compress/compress.o encrypt/encrypt.o util filemonitor/lukefilewalker.o peer/peer.o encrypt/aes/libaes.a
	gcc $(CFLAGS) -lpthread -lz compress/compress.o encrypt/encrypt.o filemonitor/lukefilewalker.o peer/peer.o utils/*.o encrypt/aes/libaes.a -o Dsync_peer

tracker: compress/compress.o encrypt/encrypt.o util filemonitor/lukefilewalker.o tracker/tracker.o encrypt/aes/libaes.a
	gcc $(CFLAGS) -lpthread -lz compress/compress.o encrypt/encrypt.o filemonitor/lukefilewalker.o tracker/tracker.o utils/*.o encrypt/aes/libaes.a -o Dsync_tracker

pitracker: compress/compress.o encrypt/encrypt.o util filemonitor/lukefilewalker.o tracker/tracker.o encrypt/aes/libaes.a
	gcc $(CFLAGS) -lrt -lpthread -lz compress/compress.o encrypt/encrypt.o filemonitor/lukefilewalker.o tracker/tracker.o utils/*.o encrypt/aes/libaes.a -o Dsync_pi_tracker

compress/compress.o: compress/compress.c compress/compress.h
	gcc $(CFLAGS) -c compress/compress.c -o compress/compress.o

encrypt/encrypt.o: encrypt/encrypt.c encrypt/encrypt.h
	gcc $(CFLAGS) -c encrypt/encrypt.c -o encrypt/encrypt.o

file_monitor/lukefilewalker.o: filemonitor/lukefilewalker.c filemonitor/lukefilewalker.h
	gcc $(CFLAGS) -c -D_BSD_SOURCE filemonitor/lukefilewalker.c -o filemonitor/lukefilewalker.o

peer/peer.o: peer/peer.c peer/peer.h
	gcc $(CFLAGS) -pthread -c peer/peer.c -o peer/peer.o

tracker/tracker.o: tracker/tracker.c tracker/tracker.h
	gcc $(CFLAGS) -pthread -c tracker/tracker.c -o tracker/tracker.o

util: utils/checksum.c utils/common.c utils/common.h utils/qs.c utils/qs.h utils/socket_common.c utils/socket_common.h
	cd utils && gcc $(CFLAGS) -c *.c
	
encrypt/aes/libaes.a: encrypt/aes
	cd encrypt/aes && gcc $(CFLAGS) -c aescrypt.c aeskey.c aestab.c aes_modes.c; ar rcs libaes.a *.o

clean:
	rm -rf *~ *.o core* *.a
	rm -rf testpeer
	rm -rf testtracker
	rm -rf Dsync_peer
	rm -rf Dsync_tracker
	rm -rf Dsync_pi_tracker
	rm -rf compress/*.o
	rm -rf encrypt/*.o
	rm -rf filemonitor/*.o
	rm -rf peer/*.o
	rm -rf tracker/*.o
	rm -rf utils/*.o
	rm -rf encrypt/aes/libaes.a encrypt/aes/*.o
