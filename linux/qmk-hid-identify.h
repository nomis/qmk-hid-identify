/*
	qmk-hid-identify - Identify the current OS to QMK device
	Copyright 2021  Simon Arlott

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include <string>

class QMKDevice {
public:
	explicit QMKDevice(const std::string &device);
	virtual ~QMKDevice();

	int open();
	int identify();
	void close();

	std::string name();
	virtual void log(int level, const std::string &message);

private:
	int is_qmk_device();
	int send_report();

	int fd_ = -1;
	std::string device_;
	std::string name_;
};
