CFLAGS=$(shell pkg-config fuse3 --cflags) -Wall -std=gnu11 -Wno-unused-variable  # 编译标志
LDFLAGS=$(shell pkg-config fuse3 --libs)  # 链接标志
LDLIBS=                     # 链接库

CC=gcc                      # 编译器

.PHONY: clean debug static  # 伪目标

all: simple_fat16

debug: CFLAGS += -g		# 目标
debug: simple_fat16     # 依赖

static: CFLAGS += -static
static: simple_fat16

simple_fat16: simple_fat16.o fat16_fixed.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

fat16_fixed.o: fat16_fixed.c fat16.h
	$(CC) $(CFLAGS) -c -o $@ $<

simple_fat16.o: simple_fat16.c fat16.h fat16_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

hello: hello.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

hello.o: hello.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f simple_fat16 hello *.o



# `$@`：表示目标文件名。
# 例如，在规则`simple_fat16: simple_fat16.o fat16_fixed.o`中，
# 如果我们在命令中使用`$@`，它就会被替换为`simple_fat16`。

# `$^`：表示所有的依赖文件，以空格分隔。
# 例如，在规则`simple_fat16: simple_fat16.o fat16_fixed.o`中，
# 如果我们在命令中使用`$^`，它就会被替换为`simple_fat16.o fat16_fixed.o`。

# 所以，命令`$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)`
# 会被展开为
# gcc (CFLAGS的值) -o simple_fat16 simple_fat16.o fat16_fixed.o (LDFLAGS的值) (LDLIBS的值)