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

		if (is_bcd_mode()) {
			convert_tp_from_bcd(current);
		}
	}

	/**
	 * Converts a timepoint from BCD to binary values
	 * @param tp The timepoint to convert
	 */
	void convert_tp_from_bcd(RTCTimePoint& tp)
	{
		tp.seconds = from_bcd(tp.seconds);
		tp.minutes = from_bcd(tp.minutes);
		// Below line from https://wiki.osdev.org/CMOS#Examples
		// tp.hours = ( (tp.hours & 0xF) + (((tp.hours & 0x70) / 16) * 10) ) | (tp.hours & 0x80);
		tp.hours = from_bcd(tp.hours);
		tp.day_of_month = from_bcd(tp.day_of_month);
		tp.month = from_bcd(tp.month);
		tp.year = from_bcd(tp.year);
	}

	/**
	 * Reads the CMOS to determine if we are in BCD mode
	 * @returns true if we are in BCD mode
	 */
	bool is_bcd_mode()
	{
		// You must make sure that interrupts are
		// disabled when accessing the RTC
		UniqueIRQLock l;

		// Read bit 2 from status register B
		auto bit = get_cmos_register(0xB, 2);

		// The bit is zero if register values are in binary coded decimal
		return bit == 0;
	}

	/**
	 * Reads the CMOS to determine if we are in 24hr mode
	 * @returns true if we are in 24hr mode
	 */
	bool is_24hr_mode()
	{
		// You must make sure that interrupts are
		// disabled when accessing the RTC
		UniqueIRQLock l;

		// Read bit 1 from status register B
		auto bit = get_cmos_register(0xB, 2);

		// The bit is non-zero if we are in 24hr mode
		return bit != 0;
	}

private:
	int const CMOS_ADDRESS = 0x70;
	int const CMOS_DATA = 0x71;

	/**
	 * Reads a specific register from the CMOS
	 * @warning Does not ensure interrupts are disabled. Use with care.
	 * @param reg The register to read
	 */
	uint8_t get_cmos_register(int reg)
	{
		__outb(CMOS_ADDRESS, reg); // activate the register
		return __inb(CMOS_DATA);
	}

	/**
	 * Reads a specific bit from a specific register from the CMOS
	 * @warning Does not ensure interrupts are disabled. Use with care.
	 * @warning This reads the entire register as well, so is not suitable for batch operations.
	 * @param reg The register to read
	 * @param bit The nth bit to read
	 */
	uint8_t get_cmos_register(int reg, int bit)
	{
		return (get_cmos_register(reg) >> bit) & 1;
	}

	/**
	 * Reads the CMOS to determine if an RTC update is in progress
	 * @warning Does not ensure interrupts are disabled. Use with care.
	 * @returns boolean of update in progress (0 = Date and time can be read, 1 = Time update in progress)
	 */
	bool is_update_in_progress()
	{
		// Note: @returns text has been plucked from http://www.bioscentral.com/misc/cmosmap.htm

		// Read bit 7 from status register A
		auto bit = get_cmos_register(0xA, 7);

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
			.seconds = get_cmos_register(0x00),
			.minutes = get_cmos_register(0x02),
			.hours = get_cmos_register(0x04),
			.day_of_month = get_cmos_register(0x07),
			.month = get_cmos_register(0x08),
			.year = get_cmos_register(0x09),
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

	/**
	 * Converts a two-digit number in BCD to the native number format
	 * @param n The binary-coded decimal number
	 * @return A natively understood number
	 */
	constexpr inline unsigned short from_bcd(unsigned short n) {
		return ((n >> 4) * 10) + (n & 0xF);
	}
};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
