
BUILD:= build
SRC  := .

compile:$(BUILD)/test.out

run_t:compile
	./$(BUILD)/test.out

OBJECTS:= \
	$(BUILD)/disk_emu.o\
	$(BUILD)/sfs_api.o\
	$(BUILD)/sfs_util.o\
	$(BUILD)/test.o\
	$(BUILD)/assert.o

.PHONY:$(BUILD)/main.out

$(BUILD)/test.out:$(OBJECTS)
	gcc $^ -g -o  $@

$(BUILD)/%.o:$(SRC)/%.c
	$(shell mkdir -p $(dir $@))
	gcc -c -g $< -o $@


.PHONY:clean
clean:
	rm -rf $(BUILD)