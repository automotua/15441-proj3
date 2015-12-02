#
# Makefile for liso server
# 
# Created by: Ke Wu <kewu@andrew.cmu.edu>
#
CC = gcc
CFLAGS = -g -Wall -Werror -std=gnu99
OBJS = proxy.o px_parse.o http_parse.o f4m.o chunk.o helper.o y.tab.o lex.yy.o

default: proxy

proxy.o: proxy.c proxy.h
	$(CC) $(CFLAGS) -c proxy.c

px_parse.o: px_parse.c px_parse.h
	$(CC) $(CFLAGS) -c px_parse.c

http_parse.o: http_parse.c http_parse.h
	$(CC) $(CFLAGS) -c http_parse.c

f4m.o: f4m.c f4m.h
	$(CC) $(CFLAGS) -c f4m.c

chunk.o: chunk.c chunk.h
	$(CC) $(CFLAGS) -c chunk.c

helper.o: helper.c helper.h
	$(CC) $(CFLAGS) -c helper.c

y.tab.o: y.tab.c
	gcc -g -c $^ -o $@

lex.yy.o: lex.yy.c
	gcc -g -c $^ -o $@

lex.yy.c: lexer.l
	flex $^

y.tab.c: parser.y
	yacc -d $^

proxy: $(OBJS)
	$(CC) $(CFLAGS) -o proxy $(OBJS) -lexpat

clean:
	rm -f *.o proxy mylog log lex.yy.c y.tab.c y.tab.h

