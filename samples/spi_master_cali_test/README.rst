.. _spi_master_cali_test:

SPI Master Timing Calibration Test
##################################

Overview
********

This sample checks the DesignWare SPI RX sample delay calibration on two SPI
NOR flashes that share one SPI controller (``spi4``) and use different
chip-selects. Each flash sits behind its own SPI filter, and the sample
switches the filters so that only one flash is driven at a time:

=============  ==========  ===========  ============
Flash alias    DT node     Chip-select  SPI filter
=============  ==========  ===========  ============
``spiflash``   flash4_0    0            ``spif3``
``spiflash2``  flash4_1    1            ``spif4``
=============  ==========  ===========  ============

``spif3`` and ``spif4`` cannot both be master; the sample puts ``spif3`` back
into filter mode before switching ``spif4`` to master.

For each flash the sample:

#. Switches its SPI filter to master mode.
#. Reads the JEDEC ID.
#. Reads the calibration result for that chip-select with
   ``spi_dw_timing_calibration_value()``.
#. Reads the first bytes of the flash. The first access initialises the flash
   driver, which runs the timing calibration.
#. Reads the calibration result again.

The calibration result is stored per chip-select and indexed by the flash
node's ``reg``. ``rx_sample_dly`` reads 255 (0xFF) before calibration and the
selected delay (0 to 4) afterwards.

Requirements
************

* ``lsqsh_evb`` board (revision ``2os``), CPU1.
* Two SPI NOR flashes on ``spi4``, CS0 behind ``spif3`` and CS1 behind
  ``spif4``.
* ``timing-calibration-disabled`` must not be set on ``spi4``, otherwise the
  calibration is skipped and the value stays 255.

The board setup lives in :file:`boards/lsqsh_evb_cpu1.overlay`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: modules/lib/zephyr_pfr_tpm/samples/spi_master_cali_test
   :board: lsqsh_evb@2os/lsqsh/cpu1
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   [00:00:01.858,000] <inf> spi_monitor_sample: spi_master_tranceive_start
   [00:00:01.873,000] <inf> spi_monitor_sample: jedec-id = [c2 20 1b];
   [00:00:01.887,000] <inf> spi_monitor_sample: [after spif3 jedec] calib value: slave=0 rx_sample_dly=255
   [00:00:01.897,000] <inf> spi_nor: init flash4@0
   [00:00:01.933,000] <inf> spi_nor: flash4@0: 128 MiBy flash
   [00:00:01.945,000] <inf> spi_dw: reference JEDEC ID c2 20 1b @ 10000000 Hz (SPI 1-1-1 calibration)
   [00:00:01.954,000] <inf> spi_dw: spi_timing_calibration: calibration success delay=2 window=[0,5) bitmap=0x1f @ 50000000 Hz
   01 02 03 03 04 05 04 05 06 07 05 06 07 08 09 06
   07 08 09
   [00:00:01.994,000] <inf> spi_monitor_sample: [after spif3 read] calib value: slave=0 rx_sample_dly=2
   [00:00:02.014,000] <inf> spi_monitor_sample: jedec-id = [20 41 20];
   [00:00:02.027,000] <inf> spi_monitor_sample: [after spif4 jedec] calib value: slave=1 rx_sample_dly=255
   [00:00:02.037,000] <inf> spi_nor: init flash4@1
   [00:00:02.073,000] <inf> spi_nor: flash4@1: 64 MiBy flash
   [00:00:02.085,000] <inf> spi_dw: reference JEDEC ID 20 41 20 @ 10000000 Hz (SPI 1-1-1 calibration)
   [00:00:02.095,000] <inf> spi_dw: spi_timing_calibration: calibration success delay=2 window=[0,5) bitmap=0x1f @ 50000000 Hz
   FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF
   FF FF FF
   [00:00:02.126,000] <inf> spi_monitor_sample: all test pass

The JEDEC IDs, flash sizes, data and chosen delays depend on the flashes
fitted on the board.
