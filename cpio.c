#include "cpio.h"
#include "strutils.h"
#include "memutils.h"

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
#define CPIO_ADDR 0x20000

extern volatile unsigned char _binary_ramdisk_start;

int hex_to_int(const char *s, int n)
{
	int v = 0;
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
	// const char *p = (char *)CPIO_ADDR;
	const char *p = (char *)&_binary_ramdisk_start;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		int sz = hex_to_int(hdr.c_namesize, 8);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);
		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0)
			break;
		for (int i = 0; i < (sz - 1); i++) {
			putchar_cb(buffer[i]);
		}
		putchar_cb('\r');
		putchar_cb('\n');
		while (((sz + sizeof(hdr)) & 0x3) != 0) {
			sz++;
		}
		p += sz;
		sz = hex_to_int(hdr.c_filesize, 8);
		while ((sz & 0x3) != 0) {
			sz++;
		}
		p += sz;
	}
}

int cpio_file_size(const char *fname)
{
	// const char *p = (char *)CPIO_ADDR;
	const char *p = (char *)&_binary_ramdisk_start;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	char found = 0;
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		int sz = hex_to_int(hdr.c_namesize, 8);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);

		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0) {
			break;
		}

		if (strlen(fname) == (sz -1 ) && strncmp(buffer, fname, sz - 1) == 0) {
			found = 1;
		}

		while (((sz + sizeof(hdr)) & 0x3) != 0) {
			sz++;
		}
		p += sz;
		sz = hex_to_int(hdr.c_filesize, 8);
		if (found == 1) {
			return sz;
		}
		while ((sz & 0x3) != 0) {
			sz++;
		}
		p += sz;
	}
	return -1;
}

int cpio_read_file(const char *fname, char *rd_buffer, int rd_buffer_size)
{
	// const char *p = (char *)CPIO_ADDR;
	const char *p = (char *)&_binary_ramdisk_start;

	struct cpio_newc_header hdr;
	
	char buffer[16];
	int fsize = 0;
	const char *pfile = 0L;
	char found = 0;
	while (1) {
		memcpy(&hdr, p, sizeof(hdr));
		int sz = hex_to_int(hdr.c_namesize, 8);
		p += sizeof(hdr);
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, p, sz);

		if (strncmp(buffer, "TRAILER!!!", sz - 1) == 0) {
			break;
		}

		if (strlen(fname) == (sz - 1) && strncmp(buffer, fname, sz - 1) == 0) {
			found = 1;
		}
		
		while (((sz + sizeof(hdr)) & 0x3) != 0) {
			sz++;
		}
		p += sz;
		sz = hex_to_int(hdr.c_filesize, 8);
		if (found == 1) {
			fsize = sz;
			pfile = p;
			break;
		}
		while ((sz & 0x3) != 0) {
			sz++;
		}
		p += sz;
	}

	if (found == 1) {
		int n = fsize < rd_buffer_size ? fsize : rd_buffer_size;
		for (int i = 0; i < n; i++)
			rd_buffer[i] = *pfile++;
		return n;
	}
	
	return -1;
}

