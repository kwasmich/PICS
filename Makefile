#CCC_CC=clang
#CC=clang
LIBS=libjpeg
CFLAGS=
#CFLAGS+=-fsanitize=address
CFLAGS+=-g3 -std=gnu11 -Wall -Wextra -pedantic -Wno-gnu -Wno-variadic-macros -O3
#--analyze
#-fsanitize=address -fno-omit-frame-pointer -funwind-tables -rdynamic
CFLAGS+=-I.
CFLAGS+=`pkg-config --cflags $(LIBS)`
#LDLIBS+=-fsanitize=address
LDLIBS+=-L/opt/vc/lib -lpthread
LDLIBS+=`pkg-config --libs $(LIBS)`
SOURCES=main.c\
        httpClient.c\
        mmapHelper.c\
        uvcCamera.c\
        uvcCapture.c



OBJECTS=$(SOURCES:%.c=%.o)
DEPS=$(sort $(patsubst %, %.deps, $(OBJECTS)))
EXECUTABLE=PICS

all: $(EXECUTABLE)

-include $(DEPS)

$(DEPS):
	@true

$(EXECUTABLE): $(OBJECTS)
#	$^ == $(OBJECTS) (the list of prerequisites)
	$(CC) $^ $(LDLIBS) -o $@

%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $< -MMD -MF $@.deps

.PHONY: clean
clean:
	rm -f $(DEPS) $(OBJECTS) $(EXECUTABLE)

.PHONY: analyse
analyse:
	make clean
	scan-build make
	cppcheck --template="{file}:{line}: warning: {severity} ({id}): {message}" --enable=all --inconclusive --inconclusive .
