.. :

###########

Overview
********
add:
zephyr/soc/linkedsemi/lsqsh/soc.c
peripheral_init()
    ls_clock_control_off(WWDT1_CLOCK);
    ls_reset_line_toggle(WWDT1_RESET);
    ls_clock_control_on(WWDT1_CLOCK)


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


