
#ifndef REG_I2C_FILTER_H_
#define REG_I2C_FILTER_H_

#define WHITELIST_COMMAND_0     (0x00)
#define WHITELIST_COMMAND_1     (0x04)
#define WHITELIST_COMMAND_2     (0x08)
#define WHITELIST_COMMAND_3     (0x0c)
#define WHITELIST_COMMAND_4     (0x10)
#define WHITELIST_COMMAND_5     (0x14)
#define WHITELIST_COMMAND_6     (0x18)
#define WHITELIST_COMMAND_7     (0x1c)
#define INTR_MSK                (0x20)
#define INTR_CLR                (0x24)
#define INTR_STT                (0x28)
#define INTR_RAW                (0x2c)
#define SMBUS_FILTER_SET        (0x30)
#define WHITELIST_ADDRESS_3_0   (0x34)
#define WHITELIST_ADDRESS_7_4   (0x38)
#define WHITELIST_ADDRESS_11_8  (0x3c)
#define WHITELIST_ADDRESS_15_12 (0x40)
#define SMBF_CONTROL0_REG       (0x44)
#define SMBF_ADDRESS_INDEX      (0x4c)
#define SMBF_NONWHITELIST       (0x50)
#define SMF_FSM                 (0x54)
#define SMBF_REG_ENABLE         (0x5c)

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t COMMAND_BEYOND_WHITELIST : 1, /*[0]*/
                          ADDRESS_BEYOND_WHITELIST : 1, /*[1]*/
                          reserve0                 : 30; /*[31:2]*/
    } field;
} intr_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t BLOCK_DISABLE     : 1, /*[0]*/
                          FILTER_DISABLE    : 1, /*[1]*/
                          reserve0          : 1, /*[2]*/
                          MASTER_WRITE_MODE : 1, /*[3]*/
                          reserve1          : 30; /*[31:3]*/
    } field;
} smbf_set_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint8_t WHITELIST_ADDRESS : 7, /*[6:0]*/
                         reserve0          : 1; /*[7]*/
    } field[4];
} whitelist_address_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t SCL_HOLD_TIME : 16, /*[15:0]*/
                          reserve0      : 16; /*[31:16]*/
    } field;
} smbf_control0_reg_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t ERROR_COMMAND : 8, /*[7:0]*/
                          ERROR_ADDRESS : 8, /*[15:8]*/
                          reserve0      : 15; /*[31:16]*/
    } field;
} smbf_nonwhitelist_t;

#endif /* REG_I2C_FILTER_H_ */
