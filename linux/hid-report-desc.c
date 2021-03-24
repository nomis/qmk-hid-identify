// https://github.com/libusb/hidapi 6a01f3b4a8862b19a7ec768752ebcfc1a412f4b1 hidapi/linux/hid.c
/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2009

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU General Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        https://github.com/libusb/hidapi .
********************************************************/

#include <stdint.h>
#include <unistd.h>

#include "hid-report-desc.h"

/*
 * Gets the size of the HID item at the given position
 * Returns 1 if successful, 0 if an invalid key
 * Sets data_len and key_size when successful
 */
static int get_hid_item_size(uint8_t *report_descriptor, unsigned int pos, size_t size, int *data_len, int *key_size)
{
	int key = report_descriptor[pos];
	int size_code;

	/*
	 * This is a Long Item. The next byte contains the
	 * length of the data section (value) for this key.
	 * See the HID specification, version 1.11, section
	 * 6.2.2.3, titled "Long Items."
	 */
	if ((key & 0xf0) == 0xf0) {
		if (pos + 1 < size)
		{
			*data_len = report_descriptor[pos + 1];
			*key_size = 3;
			return 1;
		}
		*data_len = 0; /* malformed report */
		*key_size = 0;
	}

	/*
	 * This is a Short Item. The bottom two bits of the
	 * key contain the size code for the data section
	 * (value) for this key. Refer to the HID
	 * specification, version 1.11, section 6.2.2.2,
	 * titled "Short Items."
	 */
	size_code = key & 0x3;
	switch (size_code) {
	case 0:
	case 1:
	case 2:
		*data_len = size_code;
		*key_size = 1;
		return 1;
	case 3:
		*data_len = 4;
		*key_size = 1;
		return 1;
	default:
		/* Can't ever happen since size_code is & 0x3 */
		*data_len = 0;
		*key_size = 0;
		break;
	};

	/* malformed report */
	return 0;
}

/*
 * Get bytes from a HID Report Descriptor.
 * Only call with a num_bytes of 0, 1, 2, or 4.
 */
static uint32_t get_hid_report_bytes(uint8_t *rpt, size_t len, size_t num_bytes, size_t cur)
{
	/* Return if there aren't enough bytes. */
	if (cur + num_bytes >= len)
		return 0;

	if (num_bytes == 0)
		return 0;
	else if (num_bytes == 1)
		return rpt[cur + 1];
	else if (num_bytes == 2)
		return (rpt[cur + 2] * 256 + rpt[cur + 1]);
	else if (num_bytes == 4)
		return (
			rpt[cur + 4] * 0x01000000 +
			rpt[cur + 3] * 0x00010000 +
			rpt[cur + 2] * 0x00000100 +
			rpt[cur + 1] * 0x00000001
		);
	else
		return 0;
}

/*
 * Retrieves the device's Usage Page and Usage from the report descriptor.
 * The algorithm returns the current Usage Page/Usage pair whenever a new
 * Collection is found and a Usage Local Item is currently in scope.
 * Usage Local Items are consumed by each Main Item (See. 6.2.2.8).
 * The algorithm should give similar results as Apple's:
 *   https://developer.apple.com/documentation/iokit/kiohiddeviceusagepairskey?language=objc
 * Physical Collections are also matched (macOS does the same).
 *
 * This function can be called repeatedly until it returns non-0
 * Usage is found. pos is the starting point (initially 0) and will be updated
 * to the next search position.
 *
 * The return value is 0 when a pair is found.
 * 1 when finished processing descriptor.
 * -1 on a malformed report.
 */
int get_next_hid_usage(uint8_t *report_descriptor, size_t size, unsigned int *pos, uint32_t *usage_page, uint32_t *usage)
{
	int data_len, key_size;
	int initial = *pos == 0; /* Used to handle case where no top-level application collection is defined */
	int usage_pair_ready = 0;

	/* Usage is a Local Item, it must be set before each Main Item (Collection) before a pair is returned */
	int usage_found = 0;

	while (*pos < size) {
		int key = report_descriptor[*pos];
		int key_cmd = key & 0xfc;

		/* Determine data_len and key_size */
		if (!get_hid_item_size(report_descriptor, *pos, size, &data_len, &key_size))
			return -1; /* malformed report */

		switch (key_cmd) {
		case 0x4: /* Usage Page 6.2.2.7 (Global) */
			*usage_page = get_hid_report_bytes(report_descriptor, size, data_len, *pos);
			break;

		case 0x8: /* Usage 6.2.2.8 (Local) */
			*usage = get_hid_report_bytes(report_descriptor, size, data_len, *pos);
			usage_found = 1;
			break;

		case 0xa0: /* Collection 6.2.2.4 (Main) */
			/* A Usage Item (Local) must be found for the pair to be valid */
			if (usage_found)
				usage_pair_ready = 1;

			/* Usage is a Local Item, unset it */
			usage_found = 0;
			break;

		case 0x80: /* Input 6.2.2.4 (Main) */
		case 0x90: /* Output 6.2.2.4 (Main) */
		case 0xb0: /* Feature 6.2.2.4 (Main) */
		case 0xc0: /* End Collection 6.2.2.4 (Main) */
			/* Usage is a Local Item, unset it */
			usage_found = 0;
			break;
		}

		/* Skip over this key and it's associated data */
		*pos += data_len + key_size;

		/* Return usage pair */
		if (usage_pair_ready)
			return 0;
	}

	/* If no top-level application collection is found and usage page/usage pair is found, pair is valid
	   https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections */
	if (initial && usage_found)
		return 0; /* success */

	return 1; /* finished processing */
}
