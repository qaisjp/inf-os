/*
 * CMOS Real-time Clock
 * SKELETON IMPLEMENTATION -- TO BE FILLED IN FOR TASK (3)
 */

/*
 * STUDENT NUMBER: s1620208
 */
#include <infos/drivers/timer/rtc.h>
#include <infos/util/lock.h>
#include <arch/x86/pio.h>

using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;
using namespace infos::arch::x86;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */
	void read_timepoint(RTCTimePoint& current) override
	{
		current = get_timepoint();
		RTCTimePoint previous = current;

		// Keep reading timepoints (from registers) until we get two values in a row.
		// This avoids getting dodgy/inconsistent values due to RTC updates.
		do {
			previous = current;
			current = get_timepoint();
		} while (!tp_eq(current, previous));


		// FILL IN THIS METHOD - WRITE HELPER METHODS IF NECESSARY
	}

private:
	int const CMOS_ADDRESS = 0x70;
	int const CMOS_DATA = 0x71;

	uint8_t get_RTC_register(int reg)
	{
		__outb(CMOS_ADDRESS, reg); // activate the register
		return __inb(CMOS_DATA);
	}

	/**
	 * Reads the CMOS to determine if an RTC update is in progress
	 * @warning Does not ensure interrupts are disabled. Use with care.
	 * @returns boolean of update in progress (0 = Date and time can be read, 1 = Time update in progress)
	 */
	bool is_update_in_progress()
	{
		// Note: @returns text has been plucked from http://www.bioscentral.com/misc/cmosmap.htm

		// Read status register A
		auto reg = get_RTC_register(0xA);

		// Extract the 7th bit
		auto bit = (reg >> 7) & 1;

		// The bit is non-zero if an update is in progress
		return bit != 0;
	}

	/**
	 * Reads a timepoint straight from the RTC.
	 * If an RTC update is currently in progress, it will block until the update has completed.
	 * @warning THIS IS EXTREMELY EXPENSIVE AND UNSAFE.
	 * @return The timepoint composed from several register reads
	 */
	RTCTimePoint get_timepoint()
	{
		// You must make sure that interrupts are
		// disabled when accessing the RTC
		UniqueIRQLock l;

		// Wait for current update to complete
		while (is_update_in_progress()) {
			// nop
		}

		return RTCTimePoint{
			.seconds = get_RTC_register(0x00),
			.minutes = get_RTC_register(0x02),
			.hours = get_RTC_register(0x04),
			.day_of_month = get_RTC_register(0x07),
			.month = get_RTC_register(0x08),
			.year = get_RTC_register(0x09),
		};
	}

	/**
	 * Checks to see if two timepoints are different
	 * @param a Timepoint A
	 * @param b Timepoint B
	 * @return Timepoint A == Timepoint B
	 */
	bool tp_eq(RTCTimePoint a, RTCTimePoint b) {
		return (
			(a.seconds == b.seconds) &&
			(a.minutes == b.minutes) &&
			(a.hours == b.hours) &&
			(a.day_of_month == b.day_of_month) &&
			(a.month == b.month) &&
			(a.year == b.year) &&
			true
		);
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
