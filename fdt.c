#include "fdt.h"


struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

const uint8_t *fdt_begin = NULL;

struct fdt_header hdr;


uint32_t read_uint32_be(const uint8_t *p) {
	uint32_t val = 0;
	val += *p;
	val <<= 8;
	val += *(p + 1);
	val <<= 8;
	val += *(p + 2);
	val <<= 8;
	val += *(p + 3);
	return val;
}

void fdt_parse_prop(const uint8_t **p)
{
	uint32_t len = read_uint32_be(*p);
	(*p) += sizeof(uint32_t);
	// uint32_t nameoff = read_uint32_be(*p);
	(*p) += sizeof(uint32_t);
	// the name of property
	// const char *s = fdt_begin + hdr.off_dt_strings + nameoff;
	// the value of property
	// const unsigned char *b = (unsigned char *)(*p);
	(*p) += len;

	// round up to 4-byte boundary
	(*p) = (uint8_t *)(((int64_t)(*p) + (4 - 1)) & -4);
}

void fdt_parse_node(const uint8_t **p, node_visit_callback node_visit_cb)
{
	node_visit_cb(*p);
	// read the name
	while (*(*p) != '\0') {
		(*p) += 1;
	}
	// round up to 4-byte boundary
	(*p) = (uint8_t *)(((int64_t)(*p) + (4 - 1)) & -4);

	// read the next token (tag)
	uint32_t tag = read_uint32_be(*p);
	(*p) += sizeof(uint32_t);
	while (tag != FDT_END_NODE) {
		if (tag == FDT_NOP) {
			;
		} else if (tag == FDT_PROP) {
			fdt_parse_prop(p);
		} else if (tag == FDT_BEGIN_NODE) {
			fdt_parse_node(p, node_visit_cb);
		}
		
		tag = read_uint32_be(*p);
		(*p) += sizeof(uint32_t);
	}
}


void fdt_parse(const uint8_t *buffer, node_visit_callback node_visit_cb) {
	const uint8_t *p = (const uint8_t *)buffer;
	fdt_begin = p;

	hdr.magic = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.totalsize = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.off_dt_struct = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.off_dt_strings = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.off_mem_rsvmap = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.version = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.last_comp_version = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.boot_cpuid_phys = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.size_dt_strings = read_uint32_be(p);
	p += sizeof(uint32_t);
	hdr.size_dt_struct = read_uint32_be(p);
	
	p = buffer + hdr.off_dt_struct;

	uint32_t tag = read_uint32_be(p);
	if (tag != FDT_BEGIN_NODE) {
		return;
	}

	p += sizeof(uint32_t);
	fdt_parse_node(&p, node_visit_cb);
}

const char* fdt_get_string(uint32_t off)
{
	return (const char *)(fdt_begin + hdr.off_dt_strings + off);
}

