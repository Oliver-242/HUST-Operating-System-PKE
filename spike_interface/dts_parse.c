/*
 * Utility functions scanning the Flattened Device Tree (FDT), stored in DTS (Device Tree String).
 *
 * codes are borrowed from riscv-pk (https://github.com/riscv/riscv-pk)
 */

#include "dts_parse.h"
#include "spike_interface/spike_utils.h"
#include "string.h"

static inline uint32 bswap(uint32 x) {
  uint32 y = (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
  uint32 z = (y & 0x0000FFFF) << 16 | (y & 0xFFFF0000) >> 16;
  return z;
}

static uint32 *fdt_scan_helper(uint32 *lex, const char *strings, struct fdt_scan_node *node,
                               const struct fdt_cb *cb) {
  struct fdt_scan_node child;
  struct fdt_scan_prop prop;
  int last = 0;

  child.parent = node;
  // these are the default cell counts, as per the FDT spec
  child.address_cells = 2;
  child.size_cells = 1;
  prop.node = node;

  while (1) {
    switch (bswap(lex[0])) {
      case FDT_NOP: {
        lex += 1;
        break;
      }
      case FDT_PROP: {
        assert(!last);
        prop.name = strings + bswap(lex[2]);
        prop.len = bswap(lex[1]);
        prop.value = lex + 3;
        if (node && !strcmp(prop.name, "#address-cells")) {
          node->address_cells = bswap(lex[3]);
        }
        if (node && !strcmp(prop.name, "#size-cells")) {
          node->size_cells = bswap(lex[3]);
        }
        lex += 3 + (prop.len + 3) / 4;
        cb->prop(&prop, cb->extra);
        break;
      }
      case FDT_BEGIN_NODE: {
        uint32 *lex_next;
        if (!last && node && cb->done) cb->done(node, cb->extra);
        last = 1;
        child.name = (const char *)(lex + 1);
        if (cb->open) cb->open(&child, cb->extra);
        lex_next = fdt_scan_helper(lex + 2 + strlen(child.name) / 4, strings, &child, cb);
        if (cb->close && cb->close(&child, cb->extra) == -1)
          while (lex != lex_next) *lex++ = bswap(FDT_NOP);
        lex = lex_next;
        break;
      }
      case FDT_END_NODE: {
        if (!last && node && cb->done) cb->done(node, cb->extra);
        return lex + 1;
      }
      default: {  // FDT_END
        if (!last && node && cb->done) cb->done(node, cb->extra);
        return lex;
      }
    }
  }
}

const uint32 *fdt_get_address(const struct fdt_scan_node *node, const uint32 *value,
                              uint64 *result) {
  *result = 0;
  for (int cells = node->address_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

const uint32 *fdt_get_size(const struct fdt_scan_node *node, const uint32 *value, uint64 *result) {
  *result = 0;
  for (int cells = node->size_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

void fdt_scan(uint64 fdt, const struct fdt_cb *cb) {
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC || bswap(header->last_comp_version) > FDT_VERSION) return;

  const char *strings = (const char *)(fdt + bswap(header->off_dt_strings));
  uint32 *lex = (uint32 *)(fdt + bswap(header->off_dt_struct));

  fdt_scan_helper(lex, strings, 0, cb);
}
