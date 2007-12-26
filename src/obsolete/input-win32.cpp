
// NOTE: This requires DirectX 7

#include <stdio.h>
#include "input-win32.h"
#include "lair.h"
#include "win32.h"
#include "conout.h"
#include "keybrd.h"

LPDIRECTINPUT7			g_pdi;
LPDIRECTINPUTDEVICE     g_pKeyboard;		// DirectInput device for our keyboard
LPDIRECTINPUTDEVICE2	g_pJoystick = NULL;	// DirectInput device for our joystick
DIDEVCAPS            g_diDevCaps;			// something to do with joystick properties

#define DINPUT_BUFFERSIZE       16      /* Number of buffer elements */

extern BOOL g_fPaused;	// never use extern's but this code is only temporary anyway hehe

//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated joystick. If we find one, create a
//       device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext )
{
	HRESULT hr = 0;

	// I did this to get rid of warnings, nothing more ...
	if (pContext)
	{}

	// Obtain an interface to the enumerated joystick.






	hr = g_pdi->CreateDeviceEx( pdidInstance->guidInstance, IID_IDirectInputDevice2,
	                            (VOID**)&g_pJoystick, NULL );

	// If it failed, then we can't use this joystick. (Maybe the user unplugged
	// it while we were in the middle of enumerating it.)
	if( FAILED(hr) )
		return DIENUM_CONTINUE;


	// Stop enumeration. Note: we're just taking the first joystick we get. You
	// could store all the enumerated joysticks and let the user pick.
	return DIENUM_STOP;
}




//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext )
{
	//    HWND hDlg = (HWND)pContext;

	DIPROPRANGE diprg = { 0 };
	diprg.diph.dwSize       = sizeof(DIPROPRANGE);
	diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	diprg.diph.dwHow        = DIPH_BYOFFSET;
	diprg.diph.dwObj        = pdidoi->dwOfs; // Specify the enumerated axis
	diprg.lMin              = -JOY_AXIS_MAX;
	diprg.lMax              = +JOY_AXIS_MAX;

	// I did this to get rid of warnings, nothing more
	if (pContext)
	{}

	// Set the range for the axis






	if( FAILED( g_pJoystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) )
		return DIENUM_STOP;

	return DIENUM_CONTINUE;
}








/****************************************************************************
 *
 *      DIInit
 *
 *      Initialize the DirectInput variables.
 *
 *      This entails the following four functions:
 *
 *          DirectInputCreate
 *          IDirectInput::CreateDevice
 *          IDirectInputDevice::SetDataFormat
 *          IDirectInputDevice::SetCooperativeLevel
 *
 *      Reading buffered data requires another function:
 *
 *          IDirectInputDevice::SetProperty(DIPROP_BUFFERSIZE)
 *
 ****************************************************************************/

BOOL
DIInit(
    HWND hwnd
)
{
	HRESULT hr;
	HRESULT joystick_result = 0;	// we use this for all of our joystick function results

	/*
	 *  Register with the DirectInput subsystem and get a pointer
	 *  to a IDirectInput interface we can use.
	 *
	 *  Parameters:
	 *
	 *      g_hinst
	 *
	 *          Instance handle to our application or DLL.
	 *
	 *      DIRECTINPUT_VERSION
	 *
	 *          The version of DirectInput we were designed for.
	 *          We take the value from the <dinput.h> header file.
	 *
	 *      &g_pdi
	 *
	 *          Receives pointer to the IDirectInput interface
	 *          that was created.
	 *
	 *      NULL
	 *
	 *          We do not use OLE aggregation, so this parameter
	 *          must be NULL.
	 *
	 */

	hr = DirectInputCreateEx( get_g_hinst(), DIRECTINPUT_VERSION,IID_IDirectInput7, (LPVOID*)&g_pdi, NULL );


	if (FAILED(hr))
	{
		Complain(hwnd, "DirectInputCreate");
		return FALSE;
	}

	/*
	 *  Obtain an interface to the system keyboard device.
	 *
	 *  Parameters:
	 *
	 *      GUID_SysKeyboard
	 *
	 *          The instance GUID for the device we wish to access.
	 *          GUID_SysKeyboard is a predefined instance GUID that
	 *          always refers to the system keyboard device.
	 *
	 *      &g_pKeyboard
	 *
	 *          Receives pointer to the IDirectInputDevice interface
	 *          that was created.
	 *
	 *      NULL
	 *
	 *          We do not use OLE aggregation, so this parameter
	 *          must be NULL.
	 *
	 */
	hr = g_pdi->CreateDevice(GUID_SysKeyboard, &g_pKeyboard, NULL);

	if (FAILED(hr))
	{
		Complain(hwnd, "CreateDevice");
		return FALSE;
	}

	// look for a simple joystick
	joystick_result = g_pdi->EnumDevices( DIDEVTYPE_JOYSTICK, EnumJoysticksCallback,
	                                      NULL, DIEDFL_ATTACHEDONLY );

	// If we can't even enumaterate devices, we're aborting...
	if( FAILED(joystick_result) )
	{
		Complain(hwnd, "Could not enumerate any devices");
		return FALSE;
	}

	// if we detected a joystick, finish performing initializations
	if (g_pJoystick != NULL)
	{

		// let DAPHNE know we have a joystick
		set_joystick_present();

		// Set the data format to "simple joystick" - a predefined data format
		//
		// A data format specifies which controls on a device we are interested in,
		// and how they should be reported. This tells DInput that we will be
		// passing a DIJOYSTATE structure to IDirectInputDevice::GetDeviceState().
		joystick_result = g_pJoystick->SetDataFormat( &c_dfDIJoystick );
		if( FAILED(joystick_result) )
		{
			Complain(hwnd, "Joystick SetData Format Error");

			return FALSE;
		}

		// Set the cooperative level to let DInput know how this device should
		// interact with the system and with other DInput applications.
		//		joystick_result = g_pJoystick->SetCooperativeLevel( hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND );
		joystick_result = g_pJoystick->SetCooperativeLevel( hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND );

		if( FAILED(joystick_result) )
		{
			Complain(hwnd, "Joystick SetCooperativeLevel Error");
			return FALSE;
		}

		// Determine how many axis the joystick has (so we don't error out setting
		// properties for unavailable axis)
		g_diDevCaps.dwSize = sizeof(DIDEVCAPS);
		joystick_result = g_pJoystick->GetCapabilities(&g_diDevCaps);
		if ( FAILED(joystick_result) )
		{
			Complain(hwnd, "Joystick GetCapabilities Error");
			return FALSE;
		}

		// Enumerate the axes of the joystick and set the range of each axis. Note:
		// we could just use the defaults, but we're just trying to show an example
		// of enumerating device objects (axes, buttons, etc.).

		g_pJoystick->EnumObjects( EnumAxesCallback, (VOID*)hwnd, DIDFT_AXIS );

	}

	/*
	 *  Set the data format to "keyboard format".
	 *
	 *  A data format specifies which controls on a device we
	 *  are interested in, and how they should be reported.
	 *
	 *  This tells DirectInput that we are interested in all keys
	 *  on the device, and they should be reported as DirectInput
	 *  DIK_* codes.
	 *
	 *  Parameters:
	 *
	 *      c_dfDIKeyboard
	 *
	 *          Predefined data format which describes
	 *          an array of 256 bytes, one per scancode.
	 */
	hr = g_pKeyboard->SetDataFormat(&c_dfDIKeyboard);

	if (FAILED(hr))
	{
		Complain(hwnd, "SetDataFormat");
		return FALSE;
	}


	/*
	 *  Set the cooperativity level to let DirectInput know how
	 *  this device should interact with the system and with other
	 *  DirectInput applications.
	 *
	 *  Parameters:
	 *
	 *      DISCL_NONEXCLUSIVE
	 *
	 *          Retrieve keyboard data when acquired, not interfering
	 *          with any other applications which are reading keyboard
	 *          data.
	 *
	 *      DISCL_FOREGROUND
	 *
	 *          If the user switches away from our application,
	 *          automatically release the keyboard back to the system.
	 *
	 */
	hr = g_pKeyboard->SetCooperativeLevel(hwnd,
	                                      //                                       DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	                                      DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
	// DISCL_FOREGROUND is useful for debugging
	// DISCL_BACKGROUND is useful for playing with uncooperative VLDP

	if (FAILED(hr))
	{
		Complain(hwnd, "SetCooperativeLevel");
		return FALSE;
	}

	/*
	 *  IMPORTANT STEP IF YOU WANT TO USE BUFFERED DEVICE DATA!
	 *
	 *  DirectInput uses unbuffered I/O (buffer size = 0) by default.
	 *  If you want to read buffered data, you need to set a nonzero
	 *  buffer size.
	 *
	 *  Set the buffer size to DINPUT_BUFFERSIZE (defined above) elements.
	 *
	 *  The buffer size is a DWORD property associated with the device.
	 */
	DIPROPDWORD dipdw =
	    {
	        {
	            sizeof(DIPROPDWORD),        // diph.dwSize
	            sizeof(DIPROPHEADER),       // diph.dwHeaderSize
	            0,                          // diph.dwObj
	            DIPH_DEVICE,                // diph.dwHow
	        },
	        DINPUT_BUFFERSIZE,              // dwData
	    };

	hr = g_pKeyboard->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
	{
		Complain(hwnd, "Set buffer size");
		return FALSE;
	}

	return TRUE;

}

/****************************************************************************
 *
 *      DITerm
 *
 *      Terminate our usage of DirectInput.
 *
 ****************************************************************************/

void
DITerm(void)
{

	/*
	 *  Destroy any lingering IDirectInputDevice object.
	 */
	if (g_pKeyboard)
	{

		/*
		 *  Cleanliness is next to godliness.  Unacquire the device
		 *  one last time just in case we got really confused and tried
		 *  to exit while the device is still acquired.
		 */
		g_pKeyboard->Unacquire();

		g_pKeyboard->Release();
		g_pKeyboard = NULL;
	}

	if ( NULL != g_pJoystick )
	{
		// Unacquire the device one last time just in case
		// the app tried to exit while the device is still acquired.
		g_pJoystick->Unacquire();
		g_pJoystick->Release();
		g_pJoystick = NULL;
	}


	/*
	 *  Destroy any lingering IDirectInput object.
	 */
	if (g_pdi)
	{
		g_pdi->Release();
		g_pdi = NULL;
	}

}





// Checks the keyboard for input and returns the character that was entered in ASCII form
// This is used in conjunction with the routines inside conin.cpp
char win32_getkey()
{

	char result = 0;

	// If we have successfully initialized DirectInput
	if (g_pKeyboard)
	{

		DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE]; /* Receives buffered data */
		DWORD cod = 0;
		HRESULT hr = 0;

again2:
		;
		cod = DINPUT_BUFFERSIZE;
		hr = g_pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
		                                rgod, &cod, 0);
		if (hr != DI_OK)
		{
			if (hr == DIERR_INPUTLOST)
			{
				hr = g_pKeyboard->Acquire();
				if (SUCCEEDED(hr))
				{
					goto again2;	// yeah, I winced when I saw this here too.. Microsoft programming for you =] --MATT
				}
			}
			else if (hr == DI_BUFFEROVERFLOW)
			{
				printline("DirectInput: Buffer Overflow");
			}
			else if (hr == DIERR_INVALIDPARAM)
			{
				printline("DirectInput: Invalid Parameter");
			}
			else if (hr == DIERR_NOTACQUIRED)
			{
				//				printline("DirectInput: Not Acquired");
			}
			else if (hr == DIERR_NOTBUFFERED)
			{
				printline("DirectInput: Not Buffered");
			}
			else if (hr == DIERR_NOTINITIALIZED)
			{
				printline("DirectInput: Not Initialized");
			}
			else
			{
				printline("DirectInput: Unknown Error!");
			}
		}

		if (SUCCEEDED(hr) && cod > 0)
		{
			DWORD iod = 0;

			for (iod = 0; iod < cod; iod++)
			{

				// if a key was pressed
				if (rgod[iod].dwData & 0x80)
				{
					switch(rgod[iod].dwOfs)
					{
					case 2:	// 1
					case 3:	// 2
					case 4:	// 3
					case 5:	// 4
					case 6:	// 5
					case 7:	// 6
					case 8:	// 7
					case 9:	// 8
					case 0xA:	// 9
						result = rgod[iod].dwOfs + ('0' - 1);	// convert to ASCII number
						break;
					case 0xB:	// 0
						result = '0';
						break;
					case 0xC:	// -
						result = '-';
						break;
					case 0xD:
						result = '=';
						break;
					case 0x10:	// Q
						result = 'Q';
						break;
					case 0x11:	// W
						result = 'W';
						break;
					case 0x12:	// E
						result = 'E';
						break;
					case 0x13:	// R
						result = 'R';
						break;
					case 0x14:	// T
						result = 'T';
						break;
					case 0x15:	// Y
						result = 'Y';
						break;
					case 0x16:	// U
						result = 'U';
						break;
					case 0x17:	// I
						result = 'I';
						break;
					case 0x18:	// O
						result = 'O';
						break;
					case 0x19:	// P
						result = 'P';
						break;
					case 0x1C:	// Enter
						result = 13;		// carriage return
						break;
					case 0x1E:	// A
						result = 'A';
						break;
					case 0x1F:	// S
						result = 'S';
						break;
					case 0x20:	// D
						result = 'D';
						break;
					case 0x21:	// F
						result = 'F';
						break;
					case 0x22:	// G
						result = 'G';
						break;
					case 0x23:	// H
						result = 'H';
						break;
					case 0x24:	// J
						result = 'J';
						break;
					case 0x25:
						result = 'K';
						break;
					case 0x26:
						result = 'L';
						break;
					case 0x2C:
						result = 'Z';
						break;
					case 0x2D:
						result = 'X';
						break;
					case 0x2E:	// C
						result = 'C';
						break;
					case 0x2F:
						result = 'V';
						break;
					case 0x30:
						result = 'B';
						break;
					case 0x31:
						result = 'N';
						break;
					case 0x32:
						result = 'M';
						break;
					case 0x35:	// / or ?
						result = '?';	// force question mark for now...
						break;
					case 0x39:	// spacebar
						result = ' ';
						break;
					}
				}	// end if key was pressed

				// key was released, we don't care about that ...
				else
				{}	// end if key was released
			}	// end for loop
		}	// end if successful input read
	}	// end if we have DirectInput keybooard access






	else
	{
		printline("We don't have directinput, dangit!");
	}

	return(result);

}





// Checks the keyboard for input (Uses DirectInput and a bunch of code from a Microsoft guy)
// This is used for gameplay (arrow keys, spacebar, etc)
void win32_check_keyboard()
{

	// If we have successfully initialized DirectInput
	if (g_pKeyboard)
	{

		DIDEVICEOBJECTDATA rgod[DINPUT_BUFFERSIZE]; /* Receives buffered data */
		DWORD cod = 0;
		HRESULT hr = 0;

again:
		;
		cod = DINPUT_BUFFERSIZE;
		hr = g_pKeyboard->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
		                                rgod, &cod, 0);
		if (hr != DI_OK)
		{
			/*
			 *  We got an error or we got DI_BUFFEROVERFLOW.
			 *
			 *  Either way, it means that continuous contact with the
			 *  device has been lost, either due to an external
			 *  interruption, or because the buffer overflowed
			 *  and some events were lost.
			 *
			 *  Consequently, if a button was pressed at the time
			 *  the buffer overflowed or the connection was broken,
			 *  the corresponding "up" message might have been lost.
			 *
			 *  But since our simple sample doesn't actually have
			 *  any state associated with button up or down events,
			 *  there is no state to reset.  (In a real game, ignoring
			 *  the buffer overflow would result in the game thinking
			 *  a key was held down when in fact it isn't; it's just
			 *  that the "up" event got lost because the buffer
			 *  overflowed.)
			 *
			 *  If we want to be cleverer, we could do a
			 *  GetDeviceState() and compare the current state
			 *  against the state we think the device is in,
			 *  and process all the states that are currently
			 *  different from our private state.
			 *
			 */

			/* << insert recovery code here if you need any >> */

			if (hr == DIERR_INPUTLOST)
			{
				hr = g_pKeyboard->Acquire();
				if (SUCCEEDED(hr))
				{
					goto again;	// yeah, I winced when I saw this here too.. Microsoft programming for you =] --MATT
				}
			}
			else if (hr == DI_BUFFEROVERFLOW)
			{
				printline("DirectInput: Buffer Overflow");
			}
			else if (hr == DIERR_INVALIDPARAM)
			{
				printline("DirectInput: Invalid Parameter");
			}
			else if (hr == DIERR_NOTACQUIRED)
			{
				printline("DirectInput: Not Acquired");
			}
			else if (hr == DIERR_NOTBUFFERED)
			{
				printline("DirectInput: Not Buffered");
			}
			else if (hr == DIERR_NOTINITIALIZED)
			{
				printline("DirectInput: Not Initialized");
			}
			else
			{
				printline("DirectInput: Unknown Error!");
			}
		}

		/*
		 *  In order for it to be worth our while to parse the
		 *  buffer elements, the GetDeviceData must have succeeded,
		 *  and we must have received some data at all.
		 */
		if (SUCCEEDED(hr) && cod > 0)
		{
			DWORD iod = 0;

			/*
			 *  Study each of the buffer elements and process them.
			 *
			 *  Since we really don't do anything, our "processing"
			 *  consists merely of squirting the name into our
			 *  local buffer.
			 */
			for (iod = 0; iod < cod; iod++)
			{

				// if a key was pressed
				if (rgod[iod].dwData & 0x80)
				{
					switch(rgod[iod].dwOfs)
					{
					case 0x10:	// Q
						set_quitflag();
						break;
					case 0x2E:	// C
						kb_insert_coin();
						break;
					case 0x48:
					case 0xC8:	// up
						up_enable();
						break;
					case 0x4D:
					case 0xCD:	// right
						right_enable();
						break;
					case 0x50:
					case 0xD0:	// down
						down_enable();
						break;
					case 0x4B:
					case 0xCB:	// left
						left_enable();
						break;
					case 0x39:	// spacebar
						sword_enable();
						break;
					case 0x02:	// 1
						player1_enable();
						break;
					case 0x03:	// 2
						player2_enable();
						break;
					default:
						break;
					}
				}	// end if key was pressed

				// key was released
				else
				{
					switch(rgod[iod].dwOfs)
					{
					case 0x48:
					case 0xC8:	// up
						up_disable();
						break;
					case 0x4D:
					case 0xCD:	// right
						right_disable();
						break;
					case 0x50:
					case 0xD0:	// down
						down_disable();
						break;
					case 0x4B:
					case 0xCB:	// left
						left_disable();
						break;
					case 0x39:	// spacebar
						sword_disable();
						break;
					case 0x02:	// 1
						player1_disable();
						break;
					case 0x03:	// 2
						player2_disable();
						break;
					default:
						break;
					}
				}	// end if key was released
			}	// end for loop
		}	// end if successful input read
	}	// end if we have DirectInput keybooard access






	else
	{
		printline("We don't have directinput, dangit!");
	}

}



// checks for joystick input and acts upon it
void win32_check_joystick()
{

	HRESULT     joystick_result = 0;
	DIJOYSTATE  js = { 0 };           // DInput joystick state
	static bool x_axis_in_use = false;	// true if joystick is left or right
	static bool y_axis_in_use = false;	// true if joystick is up or down
	static bool sword_in_use = false;	// true if sword button is being depressed

	// if our joystick has been initialized
	if( g_pJoystick )
	{

		// Poll the device to read the current state
		joystick_result = g_pJoystick->Poll();

		if ( FAILED(joystick_result) )
		{
			Complain(get_main_hwnd(), "Could not poll the joystick!");
			::set_quitflag();
		}

		// Get the input's device state
		joystick_result = g_pJoystick->GetDeviceState( sizeof(DIJOYSTATE), &js );

		if( joystick_result == DIERR_INPUTLOST )
		{
			// DInput is telling us that the input stream has been
			// interrupted. We aren't tracking any state between polls, so
			// we don't have any special reset that needs to be done. We
			// just re-acquire and try again.
			joystick_result = g_pJoystick->Acquire();
			// no check for failure at this point
		}

		// if they're moving up






		if (js.lY < -JOY_AXIS_MID)
		{
			up_enable();
			y_axis_in_use = true;
		}
		// if they're moving down
		else if (js.lY > JOY_AXIS_MID)
		{
			down_enable();
			y_axis_in_use = true;
		}

		// if they just barely stopped moving up or down
		// we don't want to be sending up_disable and down_disable messages contiously in case they happen to be
		// using the keyboard
		else if (y_axis_in_use == true)
		{
			up_disable();
			down_disable();
			y_axis_in_use = false;
		}

		// if they're moving right
		if (js.lX > JOY_AXIS_MID)
		{
			right_enable();
			x_axis_in_use = true;
		}
		// if they're moving left
		else if (js.lX < -JOY_AXIS_MID)
		{
			left_enable();
			x_axis_in_use = true;
		}
		// if they just barely stopped moving right or left
		// we don't want to be sending disable messages contiously in case they happen to be
		// using the keyboard
		else if (x_axis_in_use == true)
		{
			right_disable();
			left_disable();
			x_axis_in_use = false;
		}

		// if they're pressing the sword button
		if (js.rgbButtons[0] & 0x80)
		{
			sword_enable();
			sword_in_use = true;
		}

		// if they just barely stopped pressing the sword button
		// we don't want to be sending disable messages contiously in case they happen to be
		// using the keyboard
		else if (sword_in_use == true)
		{
			sword_disable();
			sword_in_use = false;
		}
	}	// end if we have a joystick

}


// Acquire or unacquire the keyboard and joystick, depending on the the g_fPaused
// flag.  This synchronizes the device with our internal view of
// the world.







void Ex_SyncAcquire()
{

	if (g_fPaused)
	{
		//		printline("Unacquiring DirectInput");
		if (g_pKeyboard)
		{
			//			g_pKeyboard->Unacquire();
		}
		if (g_pJoystick)
		{
			//			g_pJoystick->Unacquire();
		}
	}
	else
	{
		printline("Acquiring DirectInput");
		if (g_pKeyboard)
		{
			g_pKeyboard->Acquire();
		}
		if (g_pJoystick)
		{
			g_pJoystick->Acquire();
		}
	}
}
