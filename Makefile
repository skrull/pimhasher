BLAKE3_GIT := https://github.com/BLAKE3-team/BLAKE3
CS_DIR := /lib/cryptsetup/scripts
DOCS_DIR := /usr/share/docs/pimhasher
DEST := /opt/pimhasher
INITRAMFS_HOOKS := /etc/initramfs-tools/hooks
all: blake3-git pimhasher-blake3

blake3-git:
	@if [ ! -d BLAKE3 ]; then\
		git clone $(BLAKE3_GIT);\
	fi
	@echo ""
	
pimhasher-blake3:
	@echo Compiling pimhasher-blake3...
	gcc -o pimhasher-blake3 \
	-O3 -Wall -Wextra \
	-I BLAKE3/c \
	pimhasher-blake3.c \
	BLAKE3/c/blake3.c \
	BLAKE3/c/blake3_dispatch.c \
	BLAKE3/c/blake3_portable.c \
	BLAKE3/c/blake3_sse2_x86-64_unix.S \
	BLAKE3/c/blake3_sse41_x86-64_unix.S \
	BLAKE3/c/blake3_avx2_x86-64_unix.S \
	BLAKE3/c/blake3_avx512_x86-64_unix.S

install: all
	install -d $(DEST)
	install -m 755 pimhasher.dash $(DEST)
	install -m 755 pimhasher-blake3 $(DEST)
	install -m 755 pimhasher.hooks $(DEST)

	install -d $(DOCS_DIR)
	install -m 644 AUTHORS $(DOCS_DIR)
	install -m 644 Benchmarks $(DOCS_DIR)
	install -m 644 ChangeLog $(DOCS_DIR)
	install -m 644 LICENSE $(DOCS_DIR)
	install -m 644 README $(DOCS_DIR)

	@echo ""
	@cp -fsv $(DEST)/pimhasher.dash /usr/bin/pimhasher
	@cp -fsv $(DEST)/pimhasher-blake3 /usr/bin/pmhasher-blake3
	@cp -fsv $(DEST)/pimhasher.hooks $(INITRAMFS_HOOKS)/pimhasher
	
	@echo ""
	@sudo update-initramfs -k all -u

	@echo ""
	@echo "You need to point your volume in crypttab with:"
	@echo "keyscript=pimhasher"

uninstall:
	@rm -fv $(DEST)/pimhasher.dash
	@rm -fv $(DEST)/pimhasher-blake3
	@rm -fv $(DEST)/pimhasher.hooks
	@rm -fv $(INITRAMFS_HOOKS)/pimhasher
	@rm -fv /usr/bin/pimhasher
	@rm -fv /usr/bin/pimhasher-blake3
	@rm -fv $(DOCS_DIR)/AUTHORS
	@rm -fv $(DOCS_DIR)/Benchmarks
	@rm -fv $(DOCS_DIR)/ChangeLog
	@rm -fv $(DOCS_DIR)/LICENSE
	@rm -fv $(DOCS_DIR)/README

	@if [ -d $(DEST) ]; then rmdir -v $(DEST); fi
	@if [ -d $(DOCS_DIR) ]; then rmdir -v $(DOCS_DIR); fi

clean:
	@rm -fv pimhasher-blake3

distclean: clean
	@if [ -d BLAKE3 ]; then rm -rf BLAKE3 && echo "removed directory 'BLAKE3'"; fi

benchmark: pimhasher-blake3
	@echo "CPU: $(shell grep -m1 'model name' /proc/cpuinfo | cut -d: -f2)"
	@echo ""
	@echo "Benchmarking (1 to 10.000.000):"
	
	@for p in 1 10 100 1000 10000 100000 1000000 5000000; do \
		printf "%10d rounds: " $${p} | numfmt --format "%'10f"; \
		echo 0 | command time -f "%E" ./pimhasher-blake3 -r $${p} >/dev/null; \
	done
