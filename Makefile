CFLAGS		+= -Wall -Wextra
LDFLAGS		+= -limobiledevice-1.0

ifneq ($(DEBUG),)
  CFLAGS	+= -g
else
  CFLAGS	+= -O2
endif

IQUICK_T	:= iquick
IQUICK_S	:= iquick.c

.PHONY: all
all: $(IQUICK_T)

$(IQUICK_T): $(IQUICK_S)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

.PHONY: clean
clean:
	rm -f $(IQUICK_T)