#ifdef USB_SCOREBOARD
#ifndef USB_SCOREBOARD_H
#define USB_SCOREBOARD_H

#include "scoreboard_interface.h"
#include <libftdi1/ftdi.h>

#define clk0l  0x40
#define clk1l  0x80
#define clksh  0xc0
#define nobank 0x18

class ScoreboardFactory;

class USBScoreboard : public IScoreboard
{
	// allow ScoreboardFactory to call our GetInstance
	friend class ScoreboardFactory;

public:
	// I made GetInstance public so my 'clear_hw_scoreboard' program could access this
	//  directly without linking against the other scoreboard types.
	static IScoreboard *GetInstance();

	void DeleteInstance();

	void Invalidate();

	bool RepaintIfNeeded();

	bool ChangeVisibility(bool bVisible);

	bool set_digit(unsigned int uValue, WhichDigit which);

	bool is_repaint_needed();

	bool get_digit(unsigned int &uValue, WhichDigit which);

protected:
private:
	// initialize the parallel port
	bool USBInit();

	// shutdown the parallel port
	void USBShutdown();

	USBScoreboard();
	virtual ~USBScoreboard();

	unsigned char cbuf[8];
	struct ftdi_context *ftdi;
};

#endif // USB_SCOREBOARD_H
#endif //USB_SCOREBOARD
