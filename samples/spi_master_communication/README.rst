.. _hello_world:

Hello World
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "Hello World" to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

[00:00:00.221,000] <inf> spi_master_communication: jedec-id = [85 23 19];

[00:00:00.229,000] <inf> spi_master_communication: 333
[00:00:00.237,000] <inf> spi_master_communication: spi_master_communication
[00:00:00.246,000] <inf> spi_master_communication: 444
[00:00:05.904,000] <inf> spi_nor: init flash4@0
[00:00:05.910,000] <inf> spi_nor: Exit QPI(4-4-4) mode
[00:00:05.978,000] <inf> spi_nor: flash4@0: SFDP v 1.10 AP ff with 3 PH
[00:00:05.985,000] <inf> spi_nor: PH0: ff00 rev 1.8: 23 DW @ 30
[00:00:05.994,000] <inf> spi_nor: flash4@0: 32 MiBy flash
[00:00:06.002,000] <inf> spi_nor: PH1: ff84 rev 1.1: 2 DW @ c0
[00:00:06.011,000] <inf> spi_dw: useless calibration, all delay is ok
[00:00:06.736,000] <inf> spi_master_communication: flash4@0: Performing final bulk verification of 32896 bytes...
[00:00:06.749,000] <inf> spi_master_communication: flash4@0: >>> FINAL BULK CHECK PASSED! <<<
[00:00:06.759,000] <inf> spi_master_communication: flash4@0: ========== All Flash Tests Passed! ==========
uart:~$
[00:00:03.807,000] <inf> spi_master_communication: jedec-id = [85 23 19];

[00:00:03.815,000] <inf> spi_master_communication: 333
[00:00:03.822,000] <inf> spi_master_communication: spi_master_communication
[00:00:03.832,000] <inf> spi_master_communication: 444
[00:00:03.842,000] <inf> spi_nor: init flash4@0
[00:00:03.848,000] <inf> spi_nor: Exit QPI(4-4-4) mode
[00:00:03.915,000] <inf> spi_nor: flash4@0: SFDP v 1.10 AP ff with 3 PH
[00:00:03.923,000] <inf> spi_nor: PH0: ff00 rev 1.8: 23 DW @ 30
[00:00:03.932,000] <inf> spi_nor: flash4@0: 32 MiBy flash
[00:00:03.940,000] <inf> spi_nor: PH1: ff84 rev 1.1: 2 DW @ c0
[00:00:03.949,000] <inf> spi_dw: useless calibration, all delay is ok
[00:00:04.624,000] <inf> spi_master_communication: flash4@0: Performing final bulk verification of 32896 bytes...
[00:00:04.637,000] <inf> spi_master_communication: flash4@0: >>> FINAL BULK CHECK PASSED! <<<

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
