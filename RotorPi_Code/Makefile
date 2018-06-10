
CC = gcc
ODIR = obj
SDIR = src
DOCDIR = doc
INC = -Iinclude
LIBS = -lpthread -lwiringPi -lm
CFLAGS = -Wall
BINS = led

_OBJ_LED = led.o utils.o web_client.o


OBJS_LED = $(patsubst %,$(ODIR)/%,$(_OBJ_LED))

$(ODIR)/%.o: $(SDIR)/%.c
	@mkdir -p $(@D)
	$(CC) -c $(INC) -o $@ $< $(CFLAGS) $(LIBS)

led: $(OBJS_LED)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

all: $(BINS)

.PHONY: doc
doc:
	doxygen

clean:
	rm -Rf $(ODIR) $(BINS) $(DOCDIR)
