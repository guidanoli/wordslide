NAME= wordslide
RIVEMU= rivemu
RIVEMU_EXEC= $(RIVEMU) -workspace -quiet -no-window -sdk -exec
RIVEMU_RUN= $(RIVEMU) -bench -print-outcard
CFLAGS= $(shell $(RIVEMU_EXEC) riv-opt-flags -Ospeed)
SOURCE_FILES= wordslide.c dictionary.c
HEADER_FILES= dictionary.h
COMPRESSION= xz

.PHONY: build
build: $(NAME).sqfs

$(NAME).sqfs: $(NAME).elf dictionary.txt
	$(RIVEMU_EXEC) riv-mksqfs $^ $@ -comp $(COMPRESSION)

$(NAME).elf: $(SOURCE_FILES) $(HEADER_FILES)
	$(RIVEMU_EXEC) gcc -o $@ $(SOURCE_FILES) $(CFLAGS)
	$(RIVEMU_EXEC) riv-strip $@

dictionary.c: dictionary.awk dictionary.txt
	awk -f $^ > $@

.PHONY: run
run: $(NAME).sqfs
	$(RIVEMU_RUN) $<

.PHONY: run-jit
run-jit: $(SOURCE_FILES)
	$(RIVEMU) -workspace -print-outcard -exec c2m -L/usr/lib -lriv -O3 $^ -eb

.PHONY: clean
clean:
	rm -f *.sqfs *.elf dictionary.c
