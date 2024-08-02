#ifndef __FDT_H__
#define __FDT_H__

#include <stddef.h>
#include <stdint.h>

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009

typedef void (*node_visit_callback)(const uint8_t *node);

void fdt_parse(const uint8_t *buffer, node_visit_callback node_visit_cb);
const char* fdt_get_string(uint32_t off);

uint32_t read_uint32_be(const uint8_t *p);

#endif
