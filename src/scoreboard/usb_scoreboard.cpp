#include "usb_scoreboard.h"
#include <libftdi1/ftdi.h>
#include <string.h>

void USBScoreboard::DeleteInstance() { delete this; }

IScoreboard *USBScoreboard::GetInstance() {
  USBScoreboard *uRes = 0;

  uRes = new USBScoreboard();
  
  // Try to init, if this fails, we fail
  if (!uRes->USBInit() || !uRes->Init()) {
    uRes->DeleteInstance();
    uRes = 0;
  }

  return uRes;
}


bool USBScoreboard::USBInit() {
  if ((ftdi = ftdi_new()) == NULL) { return -1; }
  if (ftdi_usb_open_desc(ftdi, 0x0403, 0x6001, "Daphne_USB", NULL)<0) { return -2; }
  if (ftdi_set_baudrate(ftdi, 1000000)<0) { return -3; }
  return 0;
}

void USBScoreboard::USBShutdown() {
  ftdi_usb_close(ftdi);
  ftdi_free(ftdi);
  ftdi=NULL;
}

USBScoreboard::USBScoreboard() { }

USBScoreboard::~USBScoreboard() { USBShutdown(); }

void USBScoreboard::Invalidate() { }

bool USBScoreboard::RepaintIfNeeded() { return false; }

bool USBScoreboard::set_digit(unsigned int uValue, WhichDigit which) {
  unsigned int addr, bank;

  uValue &= 0x0f   ;                    // Mask to allowable values
  bank = (which & 0x08) ? 0x08 : 0x10;  // Bank write select
  addr = which & 0x07;                  // Mask to allowable address values

  // Construct buffer to write to USB
  cbuf[0] = clk0l | nobank | addr;         // Clk low,  writes high
  cbuf[1] = clksh | nobank | addr;         // Clk high, writes high
  cbuf[2] = clk0l | bank   | addr;         // Clk low,  one write low
  cbuf[3] = clksh | bank   | addr;         // Clk high, one write low
  cbuf[4] = clk1l |          uValue;       // Clk low
  cbuf[5] = clksh |          uValue;       // Clk high
  cbuf[6] = clk0l | nobank | addr;         // Clk low,  writes high
  cbuf[7] = clksh | nobank | addr;         // Clk high, writes high

  // Write data to port
  ftdi_write_data(ftdi, cbuf, 8);
  return true;
}
 
bool USBScoreboard::is_repaint_needed() { return false; }

bool USBScoreboard::get_digit(unsigned int &uValue, WhichDigit which) {
	uValue = m_DigitValues[which];
	return true;
}

bool USBScoreboard::ChangeVisibility(bool bDontCare) { return false; }
