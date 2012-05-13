/*
Copyright (C) 2012 Mathias Gottschlag <mgottschlag@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "usbdrv.h"
#include "adc.h"

#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// USB report descriptor
PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
	0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
	0x09, 0x04,                    // USAGE (Joystick)
	0xa1, 0x01,                    // COLLECTION (Application)
	0x05, 0x02,                    //   Usage_Page (Simulation Controls)
	0x09, 0xBA,                    //   Usage (Rudder) - OK
	0x09, 0xBB,                    //   Usage (Throttle) - OK
	0x15, 0x00,                    //   Logical_Minimum (0)
	0x26, 0xFF, 0x00,              //   Logical Maximum (255)
	0x75, 0x08,                    //   Report_Size (8)
	0x95, 0x02,                    //   Report_Count (2)
	0x81, 0x02,                    //   Input (Data, Var, Abs)
	0x05, 0x01,                    //   Usage_Page (Generic Desktop)
	0x09, 0x01,                    //   Usage (Pointer)
	0xA1, 0x00,                    //   Collection (Physical)
	0x09, 0x36,                    //   Usage (Slider) - OK
	0x15, 0x00,                    //   Logical_Minimum (0)
	0x26, 0xFF, 0x00,              //   Logical Maximum (255)
	0x75, 0x08,                    //   Report_Size (8)
	0x95, 0x01,                    //   Report_Count (1)
	0x81, 0x02,                    //   Input (Data, Var, Abs)
	0xC0, 0x16,                    //   End_Collection
	0x05, 0x09,                    //   USAGE_PAGE (Button)
	0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
	0x29, 0x08,                    //     USAGE_MAXIMUM (Button 8)
	0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
	0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
	0x75, 0x01,                    //   REPORT_SIZE (1)
	0x95, 0x08,                    //   REPORT_COUNT (8)
	0x81, 0x02,                    //   INPUT (Data,Var,Abs)
	0xC0                           // End_Collection
};

typedef struct {
	uchar axisData[3];
	uchar buttons;
} report_t;

static report_t reportBuffer = {{0}, 0};
static uchar idleRate = 0;
// The toggle switches actually act as buttons which are pressed for a short
// time whenever they are toggled
static uchar toggleStatus[2] = {0};
static uchar toggleTimeout[2] = {0};

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
	usbRequest_t *rq = (void *)data;
	if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
		// class request type
		if (rq->bRequest == USBRQ_HID_GET_REPORT){
			// wValue: ReportType (highbyte), ReportID (lowbyte)
			// we only have one report type, so don't look at wValue
			usbMsgPtr = (void*)&reportBuffer;
			return sizeof(reportBuffer);
		} else if(rq->bRequest == USBRQ_HID_GET_IDLE) {
			usbMsgPtr = &idleRate;
			return 1;
		} else if(rq->bRequest == USBRQ_HID_SET_IDLE) {
			idleRate = rq->wValue.bytes[1];
		}
	} else {
		// no vendor specific requests implemented
	}
	return 0;
}

int __attribute__((noreturn)) main(void) {
	// Set the watchdog to 1s reset frequency
	wdt_enable(WDTO_1S);
	// Initialize USB and enforce re-enumeration
	usbInit();
    usbDeviceDisconnect();
	// Fake USB disconnect for >250 ms
	wdt_reset();
	_delay_ms(255);
	wdt_reset();
	// Enable the timer for toggle switches
	TCCR0 = 0x4; // Prescaler: 512
	TIMSK = 1 << TOIE0; // Timer overflow interrupt enabled
	// Configure button input
	DDRC = 0;
	PORTC = 0x7F;
	// Initialize the ADC
	adcInit();
	wdt_reset();
	// Enable USB and reenable interrupts
	usbDeviceConnect();
	sei();
	// In a loop repeatedly send packets with the joystick state
	uchar oldToggleStatus = 0x0;
	for (;;) {
		wdt_reset();
		usbPoll();
		// Read ADC data
		reportBuffer.axisData[0] = adcConvertChannel(5) >> 2;
		reportBuffer.axisData[1] = adcConvertChannel(6) >> 2;
		reportBuffer.axisData[2] = adcConvertChannel(7) >> 2;
		// Check the toggle switches
		uchar newToggleStatus = ~PINC & 0x03;
		if ((newToggleStatus ^ oldToggleStatus) & 0x2) {
			toggleStatus[0] = 1;
			toggleTimeout[0] = 16;
		}
		if ((newToggleStatus ^ oldToggleStatus) & 0x1) {
			toggleStatus[1] = 1;
			toggleTimeout[1] = 16;
		}
		oldToggleStatus = newToggleStatus;
		// Compose button info
		// NOTE: Negate this if your inputs are not using a pull-up resistor
		reportBuffer.buttons = ~PINC & 0x7C;
		reportBuffer.buttons |= (toggleStatus[0] << 1) | toggleStatus[1];
		// Send the data
		if(usbInterruptIsReady()) {
			usbSetInterrupt((void*)&reportBuffer, sizeof(reportBuffer));
		}
	}
}

ISR(TIMER0_OVF_vect) {
	// Reset the toggle switch value so that it is only 1 for a short time after
	// the switch has been flipped
	if (toggleTimeout[0] > 0) {
		toggleTimeout[0]--;
		if (toggleTimeout[0] == 0) {
			toggleStatus[0] = 0;
		}
	}
	if (toggleTimeout[1] > 0) {
		toggleTimeout[1]--;
		if (toggleTimeout[1] == 0) {
			toggleStatus[1] = 0;
		}
	}
}
