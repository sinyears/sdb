CC=g++
TARGET=wdb
FLAGS= -g
INCLUDES= ./include
LIBS=  -lwdb -lpthread
LIBLOCALDIR= ./libs
LIBDIRS=${LIBLOCALDIR}

CMD=${CC} -c $< ${FLAGS} -I${INCLUDES} -L${LIBDIRS} -o $@
DYLIB=libwdb.so
SHOBJ=mutex.o common.o region.o mpool.o

ALL:${DYLIB} ${TARGET}
${DYLIB}:${SHOBJ}
	${CC} -shared -fPIC ${SHOBJ} -o ${DYLIB}
	cp ${DYLIB} /usr/lib
	mv ${DYLIB} ${LIBLOCALDIR}
	rm *.o

mutex.o:mutex/*.cpp
	${CMD}

common.o:common/*.cpp
	${CMD}

region.o:mem/region.cpp common.o mutex.o
	${CMD}

mpool.o:mem/mpool.cpp common.o mutex.o region.o
	${CMD}

OBJS=db.cpp

${TARGET}:${OBJS}
	${CC} ${OBJS} ${FLAGS} -I${INCLUDES}  ${LIBS} -o ${TARGET}

clean:
	&rm ${TARGET}
	&rm ${LIBLOCALDIR}/*


