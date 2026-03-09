.link_to_pfr_tpm_bss :
{
    . = ALIGN(4);
    __pfr_tpm_bss_start = .;

    . = ALIGN(4);
    __pfr_tpm_bss_end = .;
} > PFR_TPM
__pfr_tpm_bss_size = __pfr_tpm_bss_end - __pfr_tpm_bss_start;

.link_to_pfr_tpm_noinit :
{
    . = ALIGN(4);
    __pfr_tpm_noinit_start = .;

    *(.noinit.*drivers/pfr/spi/spi_filter.c*.spi_filter_log_buf_*)
    *(.noinit.*drivers/pfr/spi/spi_filter.c*.spi_filter_log_info_*)
    *(.noinit.*drivers/pfr/spi/spi_filter.c*.spi_filter_read_addr_whitelist_*)
    *(.noinit.*drivers/pfr/spi/spi_filter.c*.spi_filter_write_addr_whitelist_*)

    . = ALIGN(4);
    __pfr_tpm_noinit_end = .;
} > PFR_TPM
__pfr_tpm_noinit_size = __pfr_tpm_noinit_end - __pfr_tpm_noinit_start;
