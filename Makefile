NAME= wordslide
RIVEMU= rivemu
RIVEMU_EXEC= $(RIVEMU) -workspace -quiet -no-window -sdk -exec
RIVEMU_RUN= $(RIVEMU) -bench -print-outcard
RIVEMU_JIT= $(RIVEMU) -workspace -print-outcard -exec riv-jit-c
CFLAGS= $(shell $(RIVEMU_EXEC) riv-opt-flags -Ospeed)
SOURCE_FILES= main.c
COMPRESSION= xz

.PHONY: build
build: $(NAME).sqfs

$(NAME).sqfs: $(NAME).elf
	$(RIVEMU_EXEC) riv-mksqfs $^ $@ -comp $(COMPRESSION)

$(NAME).elf: $(SOURCE_FILES)
	$(RIVEMU_EXEC) gcc -o $@ $^ $(CFLAGS)
	$(RIVEMU_EXEC) riv-strip $@

.PHONY: run
run: $(NAME).sqfs
	$(RIVEMU_RUN) $<

.PHONY: run-jit
run-jit: $(SOURCE_FILES)
	$(RIVEMU_JIT) $^

.PHONY: clean
clean:
	rm -f *.sqfs *.elf
