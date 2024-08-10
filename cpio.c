#include "cpio.h"
#include "strutils.h"
#include "memutils.h"

#include "heap.h"
#include "utils.h"

struct cpio_newc_header {
	char c_magic[6];
	char c_ino[8];
	char c_mode[8];
	char c_uid[8];
	char c_gid[8];
	char c_nlink[8];
	char c_mtime[8];
	char c_filesize[8];
	char c_devmajor[8];
	char c_devminor[8];
	char c_rdevmajor[8];
	char c_rdevminor[8];
	char c_namesize[8];
	char c_check[8];
};

// The address is specified in config.txt
// #define INITRD_START 0x2000000
// Default address at which QEMU load ramdisk
#define INITRD_START 0x8000000

static uint8_t *initrd_start = NULL;

uint32_t hex_to_uint32(const char *s)
{
	uint32_t v = 0;
	int n = 8;
	while (n > 0) {
		v <<= 4;
		if (*s >= '0' && *s <= '9')
			v += *s++ - '0';
		else if (*s >= 'A' && *s <= 'F')
			v += *s++ - 'A' + 10;
		else if (*s >= 'a' && *s <= 'f')
			v += *s++ - 'a' + 10;
		else
			break;
		n--;
	}
	return v;
}

void cpio_read_catalog(putchar_cb_t putchar_cb)
{
	const uint8_t *p = initrd_start != NULL ? initrd_start : (uint8_t *)INITRD_START;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		uint32_t sz = hex_to_uint32(hdr.c_namesize);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);
		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0)
			break;
		for (uint32_t i = 0; i < (sz - 1); i++) {
			putchar_cb(buffer[i]);
		}
		putchar_cb('\r');
		putchar_cb('\n');
		
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
		sz = hex_to_uint32(hdr.c_filesize);
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
	}
}

int cpio_file_size(const char *fname)
{
	const uint8_t *p = initrd_start != NULL ? initrd_start : (uint8_t *)INITRD_START;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	char found = 0;
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		uint32_t sz = hex_to_uint32(hdr.c_namesize);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);

		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0) {
			break;
		}

		if (strlen(fname) == (sz -1 ) && strncmp(buffer, fname, sz - 1) == 0) {
			found = 1;
		}

		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
		sz = hex_to_uint32(hdr.c_filesize);
		if (found == 1) {
			return sz;
		}
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
	}
	return -1;
}

int cpio_read_file(const char *fname, char *rd_buffer, size_t rd_buffer_size)
{
	const uint8_t *p = initrd_start != NULL ? initrd_start : (uint8_t *)INITRD_START;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	uint32_t fsize = 0;
	const char *pfile = NULL;
	char found = 0;
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		uint32_t sz = hex_to_uint32(hdr.c_namesize);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);

		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0) {
			break;
		}

		if (strlen(fname) == (sz - 1) && strncmp(buffer, fname, sz - 1) == 0) {
			found = 1;
		}
		
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
		sz = hex_to_uint32(hdr.c_filesize);
		if (found == 1) {
			fsize = sz;
			pfile = (const char *)p;
			break;
		}
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
	}

	if (found == 1) {
		size_t n = fsize < rd_buffer_size ? fsize : rd_buffer_size;
		memcpy(rd_buffer, pfile, n);
		return n;
	}
	
	return -1;
}

int cpio_exec_file(const char *fname, void *stack)
{
	const uint8_t *p = initrd_start != NULL ? initrd_start : (uint8_t *)INITRD_START;

	struct cpio_newc_header hdr;

	char buffer[16];
	uint32_t fsize = 0;
	const uint8_t *pfile = NULL;
	char found = 0;
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		uint32_t sz = hex_to_uint32(hdr.c_namesize);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);

		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0) {
			break;
		}

		if (strlen(fname) == (sz - 1) && strncmp(buffer, fname, sz - 1) == 0) {
			found = 1;
		}

		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
		sz = hex_to_uint32(hdr.c_filesize);
		if (found == 1) {
			fsize = sz;
			pfile = p;
			break;
		}
		p += sz;
		p = (uint8_t *)(((int64_t)p + (4 - 1)) & -4);
	}

	if (found == 1) {
		//uint8_t *fbuffer = malloc(fsize);
		//memcpy(fbuffer, pfile, fsize);
		//execute_in_el0(fbuffer, stack);
		execute_in_el0(pfile, stack);
		return 0;
	}

	return -1;
}

void cpio_set_initrd_start(uint64_t addr)
{
	initrd_start = (uint8_t *)addr;
}

