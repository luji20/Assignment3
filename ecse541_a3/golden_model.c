
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// ── Configuration (mirrors common.h) ────────────────────────
#define WORD_BYTES      4
#define MMIO_END        0x400
#define DEFAULT_SIZE    6
#define DEFAULT_LOOPS   1000
#define DEFAULT_ADDR_C  0x000400
#define DEFAULT_ADDR_A  0x024400
#define DEFAULT_ADDR_B  0x048400

// ── Helpers ──────────────────────────────────────────────────
static inline unsigned int matrix_addr(unsigned int base,
                                       unsigned int n,
                                       unsigned int i,
                                       unsigned int j,
                                       unsigned int size)
{
    return base + (n * size * size + i * size + j) * WORD_BYTES;
}

static uint32_t mem_read(uint8_t *mem, unsigned int byte_addr)
{
    return (uint32_t)mem[byte_addr]
         | ((uint32_t)mem[byte_addr + 1] << 8)
         | ((uint32_t)mem[byte_addr + 2] << 16)
         | ((uint32_t)mem[byte_addr + 3] << 24);
}

static void mem_write(uint8_t *mem, unsigned int byte_addr, uint32_t val)
{
    mem[byte_addr]     = (uint8_t)(val & 0xFF);
    mem[byte_addr + 1] = (uint8_t)((val >> 8)  & 0xFF);
    mem[byte_addr + 2] = (uint8_t)((val >> 16) & 0xFF);
    mem[byte_addr + 3] = (uint8_t)((val >> 24) & 0xFF);
}

// ── Main ─────────────────────────────────────────────────────
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <memInitFile> [addrC addrA addrB size loops]\n",
                argv[0]);
        return 1;
    }

    const char   *mem_file = argv[1];
    unsigned int  addr_c   = DEFAULT_ADDR_C;
    unsigned int  addr_a   = DEFAULT_ADDR_A;
    unsigned int  addr_b   = DEFAULT_ADDR_B;
    unsigned int  size     = DEFAULT_SIZE;
    unsigned int  loops    = DEFAULT_LOOPS;

    if (argc >= 5) {
        addr_c = (unsigned int)strtoul(argv[2], NULL, 0);
        addr_a = (unsigned int)strtoul(argv[3], NULL, 0);
        addr_b = (unsigned int)strtoul(argv[4], NULL, 0);
    }
    if (argc >= 6) size  = (unsigned int)strtoul(argv[5], NULL, 0);
    if (argc >= 7) loops = (unsigned int)strtoul(argv[6], NULL, 0);

    // ── Validate addresses ────────────────────────────────────
    if (addr_c < MMIO_END || addr_a < MMIO_END || addr_b < MMIO_END) {
        fprintf(stderr, "Error: array addresses overlap MMIO region [0x000, 0x%03X)\n",
                MMIO_END);
        return 1;
    }

    // ── Allocate memory ───────────────────────────────────────
    unsigned int matrix_bytes = loops * size * size * WORD_BYTES;
    unsigned int max_addr = addr_c + matrix_bytes;
    if (addr_a + matrix_bytes > max_addr) max_addr = addr_a + matrix_bytes;
    if (addr_b + matrix_bytes > max_addr) max_addr = addr_b + matrix_bytes;

    uint8_t *memory = (uint8_t *)calloc(max_addr, 1);
    if (!memory) {
        fprintf(stderr, "Error: could not allocate %u bytes\n", max_addr);
        return 1;
    }

    // ── Load memory init file ─────────────────────────────────
    // Supports comment lines starting with # and blank lines.
    FILE *fp = fopen(mem_file, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", mem_file);
        free(memory);
        return 1;
    }

    char line[256];
    int  lines_loaded = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Skip blank lines and comment lines
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;       // skip leading whitespace
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
            continue;

        unsigned int load_addr, load_val;
        if (sscanf(p, "%x %x", &load_addr, &load_val) == 2) {
            if (load_addr + WORD_BYTES <= max_addr) {
                mem_write(memory, load_addr, load_val);
                lines_loaded++;
            }
        }
    }
    fclose(fp);

    printf("Loaded %d entries from '%s'\n\n", lines_loaded, mem_file);

    // ── Golden Multiplication ─────────────────────────────────
    unsigned int n, i, j, k;
    for (n = 0; n < loops; n++) {
        // Init: zero out c
        for (i = 0; i < size; i++)
            for (j = 0; j < size; j++)
                mem_write(memory, matrix_addr(addr_c, n, i, j, size), 0);

        // Multiply-accumulate
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                int32_t acc = 0;
                for (k = 0; k < size; k++) {
                    int32_t a_val = (int32_t)mem_read(memory,
                                        matrix_addr(addr_a, n, i, k, size));
                    int32_t b_val = (int32_t)mem_read(memory,
                                        matrix_addr(addr_b, n, k, j, size));
                    acc += a_val * b_val;
                }
                mem_write(memory, matrix_addr(addr_c, n, i, j, size),
                          (uint32_t)acc);
            }
        }
    }

    // ── Print result ──────────────────────────────────────────
    printf("Golden model result (array C):\n");
    printf("%-14s %-12s %s\n", "Address", "Hex Value", "Decimal");
    printf("%-14s %-12s %s\n", "-------", "---------", "-------");
    for (n = 0; n < loops; n++) {
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                unsigned int addr = matrix_addr(addr_c, n, i, j, size);
                int32_t      val  = (int32_t)mem_read(memory, addr);
                printf("0x%08X     0x%08X   %d   (c[%u][%u][%u])\n",
                       addr, (uint32_t)val, val, n, i, j);
            }
        }
    }

    free(memory);
    return 0;
}
