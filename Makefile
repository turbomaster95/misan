# Misan
# Freestanding, Minimalist sanitizer runtime libraries
# Build: make all
# Install: make install PREFIX=/usr/local

CC      ?= clang
AR      ?= ar
RANLIB  ?= ranlib

PREFIX  ?= /usr/local
LIBDIR  ?= $(PREFIX)/lib
INCDIR  ?= $(PREFIX)/include/misan

CFLAGS_BASE = -std=c11 -O2 -g -ffreestanding -nostdlib -nostdinc \
              -fno-builtin -fno-stack-protector -fno-asynchronous-unwind-tables \
              -Wall -Wextra -fPIC \
              -I$(PWD)/include -isystem $(shell $(CC) -print-file-name=include)

CFLAGS_ASAN  = $(CFLAGS_BASE) -DASAN_ENABLED=1
CFLAGS_UBSAN = $(CFLAGS_BASE) -DUBSAN_ENABLED=1
CFLAGS_LSAN  = $(CFLAGS_BASE) -DLSAN_ENABLED=1
CFLAGS_CFI   = $(CFLAGS_BASE) -DCFI_ENABLED=1
CFLAGS_TSAN  = $(CFLAGS_BASE) -DTSAN_ENABLED=1
CFLAGS_MSAN  = $(CFLAGS_BASE) -DMSAN_ENABLED=1

LDFLAGS_SO = -shared -nostdlib

COMMON_SRCS = src/common/san_printf.c src/common/san_pal.c
COMMON_OBJS = $(COMMON_SRCS:%.c=%.o)

DRT_ASAN_A  = libmisan_asan.a
DRT_UBSAN_A = libmisan_ubsan.a
DRT_LSAN_A  = libmisan_lsan.a
DRT_CFI_A   = libmisan_cfi.a
DRT_TSAN_A  = libmisan_tsan.a
DRT_MSAN_A  = libmisan_msan.a

DRT_ASAN_SO  = libmisan_asan.so
DRT_UBSAN_SO = libmisan_ubsan.so
DRT_LSAN_SO  = libmisan_lsan.so
DRT_CFI_SO   = libmisan_cfi.so
DRT_TSAN_SO  = libmisan_tsan.so
DRT_MSAN_SO  = libmisan_msan.so

ALL_A  = $(DRT_ASAN_A) $(DRT_UBSAN_A) $(DRT_LSAN_A) $(DRT_CFI_A) $(DRT_TSAN_A) $(DRT_MSAN_A)
ALL_SO = $(DRT_ASAN_SO) $(DRT_UBSAN_SO) $(DRT_LSAN_SO) $(DRT_CFI_SO) $(DRT_TSAN_SO) $(DRT_MSAN_SO)

.PHONY: all static shared clean install test

all: static shared

static: $(ALL_A)

shared: $(ALL_SO)

src/common/san_printf_asan.o: src/common/san_printf.c
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

src/common/san_pal_asan.o: src/common/san_pal.c
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

src/common/san_printf_ubsan.o: src/common/san_printf.c
	$(CC) $(CFLAGS_UBSAN) -c $< -o $@

src/common/san_pal_ubsan.o: src/common/san_pal.c
	$(CC) $(CFLAGS_UBSAN) -c $< -o $@

src/common/san_printf_lsan.o: src/common/san_printf.c
	$(CC) $(CFLAGS_LSAN) -c $< -o $@

src/common/san_pal_lsan.o: src/common/san_pal.c
	$(CC) $(CFLAGS_LSAN) -c $< -o $@

src/common/san_printf_cfi.o: src/common/san_printf.c
	$(CC) $(CFLAGS_CFI) -c $< -o $@

src/common/san_pal_cfi.o: src/common/san_pal.c
	$(CC) $(CFLAGS_CFI) -c $< -o $@

src/common/san_printf_tsan.o: src/common/san_printf.c
	$(CC) $(CFLAGS_TSAN) -c $< -o $@

src/common/san_pal_tsan.o: src/common/san_pal.c
	$(CC) $(CFLAGS_TSAN) -c $< -o $@

src/common/san_printf_msan.o: src/common/san_printf.c
	$(CC) $(CFLAGS_MSAN) -c $< -o $@

src/common/san_pal_msan.o: src/common/san_pal.c
	$(CC) $(CFLAGS_MSAN) -c $< -o $@

src/asan/asan_rtl.o: src/asan/asan_rtl.c
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

src/asan/asan_hooks.o: src/asan/asan_hooks.c
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

$(DRT_ASAN_A): src/asan/asan_rtl.o src/asan/asan_hooks.o src/common/san_printf_asan.o src/common/san_pal_asan.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_ASAN_SO): src/asan/asan_rtl.o src/asan/asan_hooks.o src/common/san_printf_asan.o src/common/san_pal_asan.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_ASAN_SO) $^ -o $@

src/ubsan/ubsan_handlers.o: src/ubsan/ubsan_handlers.c
	$(CC) $(CFLAGS_UBSAN) -c $< -o $@

$(DRT_UBSAN_A): src/ubsan/ubsan_handlers.o src/common/san_printf_ubsan.o src/common/san_pal_ubsan.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_UBSAN_SO): src/ubsan/ubsan_handlers.o src/common/san_printf_ubsan.o src/common/san_pal_ubsan.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_UBSAN_SO) $^ -o $@

src/lsan/lsan_rtl.o: src/lsan/lsan_rtl.c
	$(CC) $(CFLAGS_LSAN) -c $< -o $@

$(DRT_LSAN_A): src/lsan/lsan_rtl.o src/common/san_printf_lsan.o src/common/san_pal_lsan.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_LSAN_SO): src/lsan/lsan_rtl.o src/common/san_printf_lsan.o src/common/san_pal_lsan.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_LSAN_SO) $^ -o $@

src/cfi/cfi_rtl.o: src/cfi/cfi_rtl.c
	$(CC) $(CFLAGS_CFI) -c $< -o $@

$(DRT_CFI_A): src/cfi/cfi_rtl.o src/common/san_printf_cfi.o src/common/san_pal_cfi.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_CFI_SO): src/cfi/cfi_rtl.o src/common/san_printf_cfi.o src/common/san_pal_cfi.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_CFI_SO) $^ -o $@

src/tsan/tsan_rtl.o: src/tsan/tsan_rtl.c
	$(CC) $(CFLAGS_TSAN) -c $< -o $@

$(DRT_TSAN_A): src/tsan/tsan_rtl.o src/common/san_printf_tsan.o src/common/san_pal_tsan.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_TSAN_SO): src/tsan/tsan_rtl.o src/common/san_printf_tsan.o src/common/san_pal_tsan.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_TSAN_SO) $^ -o $@

src/msan/msan_rtl.o: src/msan/msan_rtl.c
	$(CC) $(CFLAGS_MSAN) -c $< -o $@

$(DRT_MSAN_A): src/msan/msan_rtl.o src/common/san_printf_msan.o src/common/san_pal_msan.o
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(DRT_MSAN_SO): src/msan/msan_rtl.o src/common/san_printf_msan.o src/common/san_pal_msan.o
	$(CC) $(LDFLAGS_SO) -Wl,-soname,$(DRT_MSAN_SO) $^ -o $@

tests/test_asan.o: tests/test_asan.c
	$(CC) $(CFLAGS_ASAN) -c $< -o $@

tests/test_ubsan.o: tests/test_ubsan.c
	$(CC) $(CFLAGS_UBSAN) -c $< -o $@

tests/test_lsan.o: tests/test_lsan.c
	$(CC) $(CFLAGS_LSAN) -c $< -o $@

test_asan: tests/test_asan.o $(DRT_ASAN_A)
	$(CC) -nostdlib -ffreestanding tests/test_asan.o -L. -lmisan_asan -o $@

test_ubsan: tests/test_ubsan.o $(DRT_UBSAN_A)
	$(CC) -nostdlib -ffreestanding tests/test_ubsan.o -L. -lmisan_ubsan -o $@

test_lsan: tests/test_lsan.o $(DRT_LSAN_A)
	$(CC) -nostdlib -ffreestanding tests/test_lsan.o -L. -lmisan_lsan -o $@

test: test_asan test_ubsan test_lsan

clean:
	rm -f src/common/*.o src/asan/*.o src/ubsan/*.o src/lsan/*.o \
	      src/cfi/*.o src/tsan/*.o src/msan/*.o \
	      tests/*.o $(ALL_A) $(ALL_SO) test_asan test_ubsan test_lsan

install: all
	install -d $(LIBDIR) $(INCDIR)/sanitizer
	install -m 644 $(ALL_A) $(LIBDIR)/
	install -m 755 $(ALL_SO) $(LIBDIR)/
	install -m 644 include/sanitizer/*.h $(INCDIR)/sanitizer/
