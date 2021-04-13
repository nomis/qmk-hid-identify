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
#pragma once

#include <windows.h>
#include <setupapi.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include <iterator>
#include <string>

#include "windows++.h"

namespace hid_identify {

class WindowsHIDEnumeration {
public:
	class const_iterator {
		using iterator_category = std::input_iterator_tag;
		using value_type = const std::wstring;
		using difference_type = DWORD;
		using pointer = const std::wstring*;
		using reference = const std::wstring&;

	public:
		const_iterator(const WindowsHIDEnumeration &di_shared, DWORD di_idx);
		const_iterator& operator++();
		const_iterator operator++(int);
		bool operator==(const const_iterator &other) const;
		bool operator!=(const const_iterator &other) const;
		reference operator*() const;

	private:
		void populate_device_path();

		const WindowsHIDEnumeration &di_shared_;
		DWORD di_idx_;
		std::wstring device_path_;
	};

	WindowsHIDEnumeration();

	const_iterator begin() const;
	const_iterator end() const;
	const_iterator cbegin() const;
	const_iterator cend() const;

private:
	GUID guid_{};
	win32::wrapped_ptr<HDEVINFO, ::SetupDiDestroyDeviceInfoList> devinfo_;
};

} // namespace hid_identify
