.. spi_monitor_warm_reset:

spi_monitor_warm_reset
###########

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints "Hello World" to the console.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/spi_monitor_warm_reset
   :host-os: unix
   :board: qemu_x86
   :goals: run
   :compact:

To build for another board, change "qemu_x86" above to that board's name.

Sample Output
=============

.. code-block:: console

    0.gitlab  clone to modules/lib/linkedsemi_zephyr_util
    1.shell input :spi_test
    2.shell input :reboot
    3.shell input :spi_dump
    3.shell input :spi_test2

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
