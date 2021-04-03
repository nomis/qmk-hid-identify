QMK HID Identify
================

Identify the current OS to connected QMK HID devices.

Use in conjunction with `raw-identify.h <https://github.com/nomis/qmk_firmware/blob/sa/users/nomis/raw-identify.h>`_
to automatically reconfigure the keyboard for each operating system.

There is a list of `allowed USB devices <common/usb-vid-pid.cc>`_ that
will need to be amended to allow it to recognise your device if you're
not already using one of the recognised identifiers.

Operating Systems
-----------------

+------------+---------------------------------+
| ID         | Name                            |
+============+=================================+
| 0x4C4E5800 | `Linux <linux/README.rst>`_     |
+------------+---------------------------------+
| 0x57494E00 | `Windows <windows/README.rst>`_ |
+------------+---------------------------------+
| 0x4D414300 | Mac OS                          |
+------------+---------------------------------+
| 0x42534400 | BSD                             |
+------------+---------------------------------+
