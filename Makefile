NAME= wordslide
RIVEMU= rivemu
RIVEMU_IFLAGS= -quiet -sdk -workspace
RIVEMU_FLAGS= $(RIVEMU_IFLAGS) -no-window
RIVEMU_RUNFLAGS= -print-outcard
RIVEMU_IEXEC= $(RIVEMU) $(RIVEMU_IFLAGS) $(RIVEMU_RUNFLAGS) -exec
RIVEMU_EXEC= $(RIVEMU) $(RIVEMU_FLAGS) -exec
RIVEMU_RUN= $(RIVEMU) $(RIVEMU_RUNFLAGS)
CFLAGS= $(shell $(RIVEMU_EXEC) riv-opt-flags -Ospeed)
SOURCE_FILES= wordslide.c dictionary.c
HEADER_FILES= dictionary.h
IMAGE_FILES= sprites.png
COMPRESSION= xz

.PHONY: build
build: $(NAME).sqfs

$(NAME).sqfs: $(NAME).elf $(IMAGE_FILES)
	$(RIVEMU_EXEC) riv-mksqfs $^ $@ -comp $(COMPRESSION)

$(NAME).elf: $(SOURCE_FILES) $(HEADER_FILES)
	$(RIVEMU_EXEC) gcc -o $@ $(SOURCE_FILES) $(CFLAGS)
	$(RIVEMU_EXEC) riv-strip $@

dictionary.c: dictionary.awk dictionary.txt
	$(RIVEMU_EXEC) awk -f $^ > $@

.PHONY: oxipng
oxipng: $(IMAGE_FILES)
	$(RIVEMU_EXEC) oxipng $^

.PHONY: run
run: $(NAME).sqfs
	$(RIVEMU_RUN) $<

.PHONY: bench
bench: $(NAME).sqfs
	$(RIVEMU_RUN) -bench $<

.PHONY: run-jit
run-jit: $(SOURCE_FILES)
	$(RIVEMU_IEXEC) c2m -L/usr/lib -lriv -O3 $^ -eb

.PHONY: clean
clean:
	$(RIVEMU_EXEC) rm -f *.sqfs *.elf dictionary.c
