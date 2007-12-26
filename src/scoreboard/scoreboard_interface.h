#ifndef SCOREBOARD_INTERFACE_H
#define SCOREBOARD_INTERFACE_H

// scoreboard interface class
class IScoreboard
{
	friend class ScoreboardFactory;

public:
	typedef enum
	{
		PLAYER1_0 = 0,
		PLAYER1_1,
		PLAYER1_2,
		PLAYER1_3,
		PLAYER1_4,
		PLAYER1_5,
		PLAYER2_0,
		PLAYER2_1,
		PLAYER2_2,
		PLAYER2_3,
		PLAYER2_4,
		PLAYER2_5,
		LIVES0,
		LIVES1,
		CREDITS1_0,
		CREDITS1_1,
		DIGIT_COUNT	// this must stay the end!
	} WhichDigit;

	// This must be called when you're done using the scoreboard.
	void PreDeleteInstance();

	// Notifies the scoreboard that it has been clobbered and needs to repainted even
	//  if nothing has changed.
	// This is mostly used for when we hide the console and need to redraw the image-based
	//  scoreboard again.
	virtual void Invalidate() = 0;

	// Redraw the entire scoreboard if needed.
	// Returns true if scoreboard was redrawn, or false if it wasn't necessary.
	virtual bool RepaintIfNeeded() = 0;

	// Displays (or hides) the scoreboard.
	// Returns true if visibility was changed or false if the visibility wasn't changed, or
	//  the scoreboard doesn't support changing visibility.
	virtual bool ChangeVisibility(bool bVisible) = 0;

	// convenience, backwards-compatibility functions
	bool update_player_score(unsigned int digit, unsigned int value, unsigned int player);
	bool update_player_lives(unsigned int value, unsigned int player);
	bool update_credits(unsigned int digit, unsigned int value);

	// sets a digit to a particular value
	// returns false if digit is out of range, or value is invalid
	// (This needs to be virtual for ScoreboardCollection class can override)
	virtual bool pre_set_digit(unsigned int uValue, WhichDigit which);

	// returns true if a repaint is needed (the return value may change after pre_set_digit is called)
	virtual bool is_repaint_needed() = 0;

	// Retrieves current scoreboard digit value
	// (returns false if digit is out of range)
	// Used for testing.
	// (This needs to be virtual for ScoreboardCollection class can override)
	virtual bool pre_get_digit(unsigned int &uValue, WhichDigit which);

	// Clears all digits on the scoreboard
	// Returns true on success
	bool Clear();

protected:

	virtual void DeleteInstance() = 0;

	IScoreboard();
	virtual ~IScoreboard();

	// this inits all LED's on the scoreboard.  I couldn't put this inside the constructor,
	//  because I need to call set_digit
	bool Init();

	// sets a digit to a particular value
	// returns false if digit is out of range, or value is invalid
	virtual bool set_digit(unsigned int uValue, WhichDigit which) = 0;

	// Sets a digit, taking into account the flickering 'A' used by SAE.
	// This is shared by the image scoreboard and overlay scoreboard, which is why it's
	//  here.
	bool set_digit_w_sae(unsigned int uValue, WhichDigit which);

	virtual bool get_digit(unsigned int &uValue, WhichDigit which) = 0;

	// keeps track of current state
	unsigned int m_DigitValues[DIGIT_COUNT];

	// whether a repaint is needed
	bool m_bNeedsRepaint;

	// whether we're using an annunciator board
	// (if we are, we need to always pass all 0xC codes to the player1 and lives LEDs, even if the last value sent was a 0xC)
	bool m_bUsingAnnunciator;

private:

	// whether Init() was called
	bool m_bInitialized;

};

#endif // SCOREBOARD_INTERFACE_H
