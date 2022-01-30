Change log
==========

Unreleased_
-----------

Added
~~~~~

* Handle power resume event on Windows by sending a report to all devices.

1.0.1_ |--| 2021-04-13
----------------------

Improvements and bug fixes for Windows.

Changed
~~~~~~~

* Use lazy initialisation for HID enumeration on Windows so that the service
  will not stop responding if there are a lot of devices.

Fixed
~~~~~

* Windows service start/stop does not timeout correctly.

1.0.0_ |--| 2021-04-12
----------------------

Initial release for Linux and Windows.

.. |--| unicode:: U+2013 .. EN DASH

.. _Unreleased: https://github.com/nomis/qmk-hid-identify/compare/1.0.1...HEAD
.. _1.0.1: https://github.com/nomis/qmk-hid-identify/compare/1.0.0...1.0.1
.. _1.0.0: https://github.com/nomis/qmk-hid-identify/commits/1.0.0
