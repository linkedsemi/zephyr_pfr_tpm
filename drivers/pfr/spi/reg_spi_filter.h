
#ifndef REG_SPI_FILTER_H_
#define REG_SPI_FILTER_H_

#define SPIFILTER1                    (0x40070000)
#define SPIFILTER2                    (0x40072000)
#define SPIFILTER3                    (0x40074000)
#define SPIFILTER4                    (0x40076000)
#define SPIF_CFG                      (0x00)
#define SPIF_INTR_STT                 (0x04)
#define SPIF_INTR_MASK                (0x08)
#define SPIF_INTR_CLR                 (0x0c)
#define SPIF_READ_DUMMY_NUM           (0x10)
#define SPIF_ILLEGAL_CMD              (0x14)
#define SPIF_ILLEGAL_ADDR             (0x18)
#define SPIF_TARGET_ADDR              (0x1c)
#define SPIF_DMA_DATA                 (0x20)
#define SPIF_BCMD_RANGE               (0x28)
#define SPIF_PRG_CMD                  (0x2c)
#define SPIF_PRG_QADDR_QDATA_CMD      (0x30)
#define SPIF_ERASE_4KB_CMD            (0x34)
#define SPIF_ERASE_32KB_CMD           (0x38)
#define SPIF_ERASE_64KB_CMD           (0x3c)
#define SPIF_READ_CMD                 (0x40)
#define SPIF_READ_FAST_CMD            (0x44)
#define SPIF_READ_QDATA_CMD           (0x48)
#define SPIF_READ_QADDR_QDATA_CMD     (0x4c)
#define SPIF_QSPI_MODE_ENTER_CMD      (0x50)
#define SPIF_QSPI_MODE_EXIT_CMD       (0x54)
#define SPIF_4B_MODE_ENTER_CMD        (0x58)
#define SPIF_4B_MODE_EXIT_CMD         (0x5c)
#define SPIF_4B_RDEAR_CMD             (0x60)
#define SPIF_4B_WREAR_CMD             (0x64)
#define SPIF_4B_PRG_CMD               (0x68)
#define SPIF_4B_PRG_QADDR_QDATA_CMD   (0x6c)
#define SPIF_4B_ERASE_4KB_CMD         (0x70)
#define SPIF_4B_ERASE_32KB_CMD        (0x74)
#define SPIF_4B_ERASE_64KB_CMD        (0x78)
#define SPIF_4B_READ_CMD              (0x7c)
#define SPIF_4B_READ_FAST_CMD         (0x80)
#define SPIF_4B_READ_QDATA_CMD        (0x84)
#define SPIF_4B_READ_QADDR_QDATA_C    (0x88)
#define SPIF_READ_DDATA_CMD           (0x8c)
#define SPIF_READ_DADDR_DDATA_CMD     (0x90)
#define SPIF_4B_READ_DDATA_CMD        (0x94)
#define SPIF_4B_READ_DADDR_DDATA_C    (0x98)
#define SPIF_PRG_QDATA_CMD            (0x9c)
#define SPIF_4B_PRG_QDATA_CMD         (0xa0)
#define SPIF_INIT_CMD0                (0xa4)
#define SPIF_INIT_CMD1                (0xa8)
#define SPIF_INIT_CMD2                (0xac)
#define SPIF_INIT_CMD3                (0xb0)
#define SPIF_INIT_CMD4                (0xb4)
#define SPIF_INIT_CMD5                (0xb8)
#define SPIF_INIT_CMD6                (0xbc)
#define SPIF_INIT_CMD7                (0xc0)
#define SPIF_INIT_CMD8                (0xc4)
#define SPIF_INIT_CMD9                (0xc8)
#define SPIF_INIT_CMD10               (0xcc)
#define SPIF_INIT_CMD11               (0xd0)
#define SPIF_INIT_CMD12               (0xd4)
#define SPIF_INIT_CMD13               (0xd8)
#define SPIF_INIT_CMD14               (0xdc)
#define SPIF_INIT_CMD15               (0xe0)
#define SPIF_INIT_CMD16               (0xe4)
#define SPIF_INIT_CMD17               (0xe8)
#define SPIF_INIT_CMD18               (0xec)
#define SPIF_INIT_CMD19               (0xf0)
#define SPIF_INIT_CMD20               (0xf4)
#define SPIF_INIT_CMD21               (0xf8)
#define SPIF_INIT_CMD22               (0xfc)
#define SPIF_INIT_CMD23               (0x100)
#define SPIF_INIT_CMD24               (0x104)
#define SPIF_INIT_CMD25               (0x108)
#define SPIF_INIT_CMD26               (0x10c)
#define SPIF_INIT_CMD27               (0x110)
#define SPIF_INIT_CMD28               (0x114)
#define SPIF_INIT_CMD29               (0x118)
#define SPIF_INIT_CMD30               (0x11c)
#define SPIF_INIT_CMD31               (0x120)
#define SPIF_INIT_CMD32               (0x124)
#define SPIF_INIT_CMD33               (0x128)
#define SPIF_INIT_CMD34               (0x12c)
#define SPIF_INIT_CMD35               (0x130)
#define SPIF_INIT_CMD36               (0x134)
#define SPIF_INIT_CMD37               (0x138)
#define SPIF_INIT_CMD38               (0x13c)
#define SPIF_INIT_CMD39               (0x140)
#define SPIF_BIT_MAP7                 (0x144)
#define SPIF_BIT_MAP6                 (0x148)
#define SPIF_BIT_MAP5                 (0x14c)
#define SPIF_BIT_MAP4                 (0x150)
#define SPIF_BIT_MAP3                 (0x154)
#define SPIF_BIT_MAP2                 (0x158)
#define SPIF_BIT_MAP1                 (0x15c)
#define SPIF_BIT_MAP0                 (0x160)
#define SPIF_SCK_SET                  (0x164)
#define SPIF_SCK_FQC_HI               (0x168)
#define SPIF_SCK_FQC_LO               (0x16c)
#define SPIF_READ_ADDR_REQ            (0x170)
#define SPIF_READ_SRAM_ADDR           (0x174)
#define SPIF_READ_SRAM_DATA           (0x178)
#define SPIF_WRITE_ADDR_VALID_EN_ADDR (0x800)  /* 0x800-0xfff */
#define SPIF_READ_ADDR_VALID_EN_ADDR  (0x1000) /* 0x1000-0x17ff */
#define SPIF_ADDR_SIZE                (0x800)
#define SPIF_CMD_BASE                 SPIF_PRG_CMD
#define SPIF_GENERAL_CMD_BASE         SPIF_INIT_CMD0
#define SPIF_ADDR_PRIV_TABLE_BASE     SPIF_WRITE_ADDR_VALID_EN_ADDR

typedef union {
    uint32_t value;
    struct {
        uint32_t
            EN : 1,                   /* [0] */
            OPERATION_MODE : 1,       /* [1] */
            ADDR_3B_4B_FLAG : 1,      /* [2] */
            ADDR_3B_4B_SEL : 1,       /* [3] */
            TARGET_ADDR_MODE_SEL : 1, /* [4] */
            BOW_CMD_SEL : 1,          /* [5] */
            IP_LOCK : 1,              /* [6] */
            DMA_EN : 1,               /* [7] */
            reserved : 24;            /* [31:8] */
    };
} spif_cfg_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            ERROR_OVERFLOW : 1, /* [0] */
            ERROR : 1,          /* [1] */
            TARGET_ADDR : 1,    /* [2] */
            SCK_CHECK : 1,      /* [3] */
            reserved : 28;      /* [31:4] */
    };
} spif_intr_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            ERROR_OVERFLOW : 1, /* [0] */
            ERROR : 1,          /* [1] */
            TARGET_ADDR : 1,    /* [2] */
            SCK_CHECK : 1,      /* [3] */
            BITMAP : 1,         /* [3] */
            reserved : 28;      /* [31:4] */
    };
} spif_intr_clr_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            READ_DUMMY_NUM : 5, /* [4:0] */
            reserved : 27;      /* [31:5] */
    };
} spif_read_dummy_num_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            ILLEGAL_CMD : 8, /* [7:0] */
            reserved : 24;   /* [31:8] */
    };
} spif_illegal_addr_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            BCMD_RANGE : 4; /* [3:0] */
    };
} spif_bcmd_range_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            CMD : 8,         /* [7:0] */
            DUMMY_CYCLE : 5, /* [12:8] */
            reserved : 18,   /* [30:13] */
            EN : 1;          /* [31] */
    };
} spif_cmd_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            SCK_FQC : 12,  /* [11:0] */
            SCK_DIV : 3,   /* [14:12] */
            reserved : 17; /* [31:15] */
    };
} spif_sck_set_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            SCK_FQC_HI : 12, /* [11:0] */
            reserved : 20;   /* [31:12] */
    };
} spif_sck_fqc_hi_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            SCK_FQC_LO : 12, /* [11:0] */
            reserved : 20;   /* [31:12] */
    };
} spif_sck_fqc_lo_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            READ_RADDR_REQ : 1, /* [0] */
            READ_WADDR_REQ : 1, /* [1] */
            reserved : 30;      /* [31:2] */
    };
} spif_read_addr_req_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            READ_SRAM_ADDR : 9, /* [8:0] */
            reserved : 23;      /* [31:9] */
    };
} spif_read_sram_addr_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t
            ADDR_ERR : 1, /* [0] */
            CMD_ERR : 1, /* [1] */
            POR_ADDR : 1, /* [2] */
            ERROR_ADDR : 21, /* [23:3] */
            ERROR_CMD : 8; /* [31:24] */
    };
} spif_dma_data_t;

enum cmd_table {
    IDX_CMD_PAGE_PROGRAM,
    IDX_CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA,
    IDX_CMD_ERASE_4KB,
    IDX_CMD_ERASE_32KB,
    IDX_CMD_ERASE_64KB,
    IDX_CMD_READ,
    IDX_CMD_FAST_READ,
    IDX_CMD_READ_QUAD_DATA,
    IDX_CMD_READ_QUAD_ADDR_QUAD_DATA,
    IDX_CMD_QUAD_SPI_MODE_ENTER,
    IDX_CMD_QUAD_SPI_MODE_EXIT,
    IDX_CMD_4BYTE_MODE_ENTER,
    IDX_CMD_4BYTE_MODE_EXIT,
    IDX_CMD_4BYTE_READ_EXTENDED_ADDR,
    IDX_CMD_4BYTE_WRITE_EXTENDED_ADDR,
    IDX_CMD_4BYTE_PAGE_PROGRAM,
    IDX_CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA,
    IDX_CMD_4BYTE_ERASE_4KB,
    IDX_CMD_4BYTE_ERASE_32KB,
    IDX_CMD_4BYTE_ERASE_64KB,
    IDX_CMD_4BYTE_READ,
    IDX_CMD_4BYTE_FAST_READ,
    IDX_CMD_4BYTE_READ_QUAD_DATA,
    IDX_CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA,
    IDX_CMD_READ_DUAL_DATA,
    IDX_CMD_READ_DUAL_ADDR_DUAL_DATA,
    IDX_CMD_4BYTE_READ_DUAL_DATA,
    IDX_CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA,
    IDX_CMD_PROGRAM_QUAD_DATA,
    IDX_CMD_4BYTE_PROGRAM_QUAD_DATA,
};

enum cmd_dummy_table {
    IDX_CMD_PAGE_PROGRAM_DUMMY,
    IDX_CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA_DUMMY,
    IDX_CMD_ERASE_4KB_DUMMY,
    IDX_CMD_ERASE_32KB_DUMMY,
    IDX_CMD_ERASE_64KB_DUMMY,
    IDX_CMD_READ_DUMMY,
    IDX_CMD_FAST_READ_DUMMY,
    IDX_CMD_READ_QUAD_DATA_DUMMY,
    IDX_CMD_READ_QUAD_ADDR_QUAD_DATA_DUMMY,
    IDX_CMD_QUAD_SPI_MODE_ENTER_DUMMY,
    IDX_CMD_QUAD_SPI_MODE_EXIT_DUMMY,
    IDX_CMD_4BYTE_MODE_ENTER_DUMMY,
    IDX_CMD_4BYTE_MODE_EXIT_DUMMY,
    IDX_CMD_4BYTE_READ_EXTENDED_ADDR_DUMMY,
    IDX_CMD_4BYTE_WRITE_EXTENDED_ADDR_DUMMY,
    IDX_CMD_4BYTE_PAGE_PROGRAM_DUMMY,
    IDX_CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA_DUMMY,
    IDX_CMD_4BYTE_ERASE_4KB_DUMMY,
    IDX_CMD_4BYTE_ERASE_32KB_DUMMY,
    IDX_CMD_4BYTE_ERASE_64KB_DUMMY,
    IDX_CMD_4BYTE_READ_DUMMY,
    IDX_CMD_4BYTE_FAST_READ_DUMMY,
    IDX_CMD_4BYTE_READ_QUAD_DATA_DUMMY,
    IDX_CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA_DUMMY,
    IDX_CMD_READ_DUAL_DATA_DUMMY,
    IDX_CMD_READ_DUAL_ADDR_DUAL_DATA_DUMMY,
    IDX_CMD_4BYTE_READ_DUAL_DATA_DUMMY,
    IDX_CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA_DUMMY,
    IDX_CMD_PROGRAM_QUAD_DATA_DUMMY,
    IDX_CMD_4BYTE_PROGRAM_QUAD_DATA_DUMMY,
};

#define CMD_PAGE_PROGRAM                              0x02
#define CMD_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA          0x38
#define CMD_ERASE_4KB                                 0x20
#define CMD_ERASE_32KB                                0x52
#define CMD_ERASE_64KB                                0xd8
#define CMD_READ                                      0x03
#define CMD_FAST_READ                                 0x0b
#define CMD_READ_QUAD_DATA                            0x6b
#define CMD_READ_QUAD_ADDR_QUAD_DATA                  0xeb
#define CMD_QUAD_SPI_MODE_ENTER                       0x35
#define CMD_QUAD_SPI_MODE_EXIT                        0xf5
#define CMD_4BYTE_MODE_ENTER                          0xb7
#define CMD_4BYTE_MODE_EXIT                           0xe9
#define CMD_4BYTE_READ_EXTENDED_ADDRESS               0xc8
#define CMD_4BYTE_WRITE_EXTENDED_ADDRESS              0xc5
#define CMD_4BYTE_PAGE_PROGRAM                        0x12
#define CMD_4BYTE_PAGE_PROGRAM_QUAD_ADDR_QUAD_DATA    0x3e
#define CMD_4BYTE_ERASE_4KB                           0x21
#define CMD_4BYTE_ERASE_32KB                          0x5c
#define CMD_4BYTE_ERASE_64KB                          0xdc
#define CMD_4BYTE_READ                                0x13
#define CMD_4BYTE_FAST_READ                           0x0c
#define CMD_4BYTE_READ_QUAD_DATA                      0x6c
#define CMD_4BYTE_READ_QUAD_ADDR_QUAD_DATA            0xec
#define CMD_READ_DUAL_DATA                            0x3b
#define CMD_READ_DUAL_ADDR_DUAL_DATA                  0xbb
#define CMD_4BYTE_READ_DUAL_DATA                      0x3c
#define CMD_4BYTE_READ_DUAL_ADDR_DUAL_DATA            0xbc
#define CMD_PROGRAM_QUAD_DATA                         0x32
#define CMD_4BYTE_PROGRAM_QUAD_DATA                   0x34

#endif /* REG_SPI_FILTER_H_ */
