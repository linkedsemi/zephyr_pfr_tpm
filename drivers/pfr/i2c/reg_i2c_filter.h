
#ifndef REG_I2C_FILTER_H_
#define REG_I2C_FILTER_H_

#define WHITELIST_COMMAND_0     (0x000)
#define WHITELIST_COMMAND_1     (0x004)
#define WHITELIST_COMMAND_2     (0x008)
#define WHITELIST_COMMAND_3     (0x00c)
#define WHITELIST_COMMAND_4     (0x010)
#define WHITELIST_COMMAND_5     (0x014)
#define WHITELIST_COMMAND_6     (0x018)
#define WHITELIST_COMMAND_7     (0x01c)
#define INTR_MSK                (0x020)
#define INTR_CLR                (0x024)
#define INTR_STT                (0x028)
#define INTR_RAW                (0x02c)
#define SMBUS_FILTER_SET        (0x030)
#define WHITELIST_ADDRESS_3_0   (0x034)
#define WHITELIST_ADDRESS_7_4   (0x038)
#define WHITELIST_ADDRESS_11_8  (0x03c)
#define WHITELIST_ADDRESS_15_12 (0x040)
#define SMBF_CONTROL0_REG       (0x044)
#define SMBF_CONTROL1_REG       (0x048)
#define SMBF_ADDRESS_INDEX      (0x04c)

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t COMMAND_BEYOND_WHITELIST : 1, /*[0]*/
                          NON_WHITELIST_WARN       : 1, /*[1]*/
                          ADDRESS_BEYOND_WHITELIST : 1, /*[2]*/
                          reserve0                 : 29; /*[31:3]*/
    } field;
} intr_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t BLOCK_DISABLE  : 1, /*[0]*/
                          FILTER_DISABLE : 1, /*[1]*/
                          reserve0       : 30; /*[31:2]*/
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
        volatile uint32_t SCL_HOLD_TIME    : 16, /*[15:0]*/
                          MASTER_SDA_DELAY : 8, /*[23:16]*/
                          MASTER_SCL_DELAY : 8; /*[31:24]*/
    } field;
} smbf_control0_reg_t;

typedef union __packed __aligned(4) {
    volatile uint32_t value;
    struct {
        volatile uint32_t SLAVE_SDA_DELAY  : 8, /*[7:0]*/
                          SLAVE_SCL_DELAY  : 8, /*[15:8]*/
                          reserve0         : 15; /*[31:16]*/
    } field;
} smbf_control1_reg_t;

#endif /* REG_I2C_FILTER_H_ */
