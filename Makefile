
CFLAGS = -std=c11 -Wall -D_GNU_SOURCE -O2
LDFLAGS = -lpthread -lssl -lcrypto -lmagic
OBJDIR = obj
OBJS = $(OBJDIR)/admin.o $(OBJDIR)/cmd-ascii-art.o $(OBJDIR)/cmd-astro.o \
$(OBJDIR)/cmd-cal.o $(OBJDIR)/cmd-calc.o $(OBJDIR)/cmd-cc.o \
$(OBJDIR)/cmd-chars.o $(OBJDIR)/cmd-colorize.o $(OBJDIR)/cmd-date.o \
$(OBJDIR)/cmd-dict.o $(OBJDIR)/cmd-foldoc.o $(OBJDIR)/cmd-fortune.o \
$(OBJDIR)/cmd-joke.o $(OBJDIR)/cmd-rainbow.o $(OBJDIR)/cmd-sh.o \
$(OBJDIR)/cmd-slap.o $(OBJDIR)/cmd-stats.o $(OBJDIR)/cmd-weather.o \
$(OBJDIR)/codybot.o $(OBJDIR)/console.o $(OBJDIR)/log.o $(OBJDIR)/msg.o \
$(OBJDIR)/server.o $(OBJDIR)/raw.o $(OBJDIR)/thread.o
PROGNAME = codybot
PROGNAME_API = api-fetcher

.PHONY: tcc default all prepare clean

default: all

all: prepare $(OBJS) $(PROGNAME_API) $(PROGNAME)

prepare:
	@[ -d $(OBJDIR) ] || mkdir $(OBJDIR) 2>/dev/null || true

$(OBJDIR)/admin.o: src/admin.c
	gcc -c $(CFLAGS) src/admin.c -o $(OBJDIR)/admin.o

$(OBJDIR)/cmd-ascii-art.o: src/cmd-ascii-art.c
	gcc -c $(CFLAGS) src/cmd-ascii-art.c -o $(OBJDIR)/cmd-ascii-art.o

$(OBJDIR)/cmd-astro.o: src/cmd-astro.c
	gcc -c $(CFLAGS) src/cmd-astro.c -o $(OBJDIR)/cmd-astro.o

$(OBJDIR)/cmd-cal.o: src/cmd-cal.c
	gcc -c $(CFLAGS) src/cmd-cal.c -o $(OBJDIR)/cmd-cal.o

$(OBJDIR)/cmd-calc.o: src/cmd-calc.c
	gcc -c $(CFLAGS) src/cmd-calc.c -o $(OBJDIR)/cmd-calc.o

$(OBJDIR)/cmd-cc.o: src/cmd-cc.c
	gcc -c $(CFLAGS) src/cmd-cc.c -o $(OBJDIR)/cmd-cc.o

$(OBJDIR)/cmd-chars.o: src/cmd-chars.c
	gcc -c $(CFLAGS) src/cmd-chars.c -o $(OBJDIR)/cmd-chars.o

$(OBJDIR)/cmd-colorize.o: src/cmd-colorize.c
	gcc -c $(CFLAGS) src/cmd-colorize.c -o $(OBJDIR)/cmd-colorize.o

$(OBJDIR)/cmd-date.o: src/cmd-date.c
	gcc -c $(CFLAGS) src/cmd-date.c -o $(OBJDIR)/cmd-date.o

$(OBJDIR)/cmd-dict.o: src/cmd-dict.c
	gcc -c $(CFLAGS) src/cmd-dict.c -o $(OBJDIR)/cmd-dict.o

$(OBJDIR)/cmd-foldoc.o: src/cmd-foldoc.c
	gcc -c $(CFLAGS) src/cmd-foldoc.c -o $(OBJDIR)/cmd-foldoc.o

$(OBJDIR)/cmd-fortune.o: src/cmd-fortune.c
	gcc -c $(CFLAGS) src/cmd-fortune.c -o $(OBJDIR)/cmd-fortune.o

$(OBJDIR)/cmd-joke.o: src/cmd-joke.c
	gcc -c $(CFLAGS) src/cmd-joke.c -o $(OBJDIR)/cmd-joke.o

$(OBJDIR)/cmd-rainbow.o: src/cmd-rainbow.c
	gcc -c $(CFLAGS) src/cmd-rainbow.c -o $(OBJDIR)/cmd-rainbow.o

$(OBJDIR)/cmd-sh.o: src/cmd-sh.c
	gcc -c $(CFLAGS) src/cmd-sh.c -o $(OBJDIR)/cmd-sh.o

$(OBJDIR)/cmd-slap.o: src/cmd-slap.c
	gcc -c $(CFLAGS) src/cmd-slap.c -o $(OBJDIR)/cmd-slap.o

$(OBJDIR)/cmd-stats.o: src/cmd-stats.c
	gcc -c $(CFLAGS) src/cmd-stats.c -o $(OBJDIR)/cmd-stats.o

$(OBJDIR)/cmd-weather.o: src/cmd-weather.c
	gcc -c $(CFLAGS) src/cmd-weather.c -o $(OBJDIR)/cmd-weather.o

$(OBJDIR)/codybot.o: src/codybot.c
	gcc -c $(CFLAGS) src/codybot.c -o $(OBJDIR)/codybot.o

$(OBJDIR)/console.o: src/console.c
	gcc -c $(CFLAGS) src/console.c -o $(OBJDIR)/console.o

$(OBJDIR)/log.o: src/log.c
	gcc -c $(CFLAGS) src/log.c -o $(OBJDIR)/log.o

$(OBJDIR)/msg.o: src/msg.c
	gcc -c $(CFLAGS) src/msg.c -o $(OBJDIR)/msg.o

$(OBJDIR)/raw.o: src/raw.c
	gcc -c $(CFLAGS) src/raw.c -o $(OBJDIR)/raw.o

$(OBJDIR)/server.o: src/server.c
	gcc -c $(CFLAGS) src/server.c -o $(OBJDIR)/server.o

$(OBJDIR)/thread.o: src/thread.c
	gcc -c $(CFLAGS) src/thread.c -o $(OBJDIR)/thread.o

$(PROGNAME_API): src/api-fetcher.c
	gcc $(CFLAGS) src/api-fetcher.c -o $(PROGNAME_API) -ljson-c -lcurl

$(PROGNAME): $(OBJS)
	gcc $(CFLAGS) $(OBJS) -o $(PROGNAME) $(LDFLAGS)

clean:
	@rm -rv $(OBJDIR) $(PROGNAME_API) $(PROGNAME) $(PROGRUN) 2>/dev/null || true

tcc:
	tcc -lmagic -lssl -o $(PROGNAME) src/*.c

