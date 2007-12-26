// DAPHNE Front-End
// This contains a bunch of Windows specific code but eventually I hope to do away with it =]

#include <windows.h>
#include <windowsx.h>	// for the neat ComboBox functions
#include <stdio.h>
#include <process.h>
#include "win32.h"
#include "..\resource.h"
#include "..\daphne.h"
#include "..\preset.h"

#define CMD_ARRAY_SIZE 81

char c_szClassName[] = "DaphneClass";
BOOL g_fPaused = TRUE;       /* Should I be paused? */

char g_szText[2] = " ";	// a dummy string the we print as an excuse to clear the screen

HINSTANCE g_hinst = 0;	// holds our current instance
HWND Our_Hwnd = 0;	// holds our window handle (hwnd)

char *g_cmd_array_ptrs[CMD_ARRAY_SIZE] = { 0 };	// holds pointer info
char g_cmd_array[CMD_ARRAY_SIZE][81] = { 0 };	// holds command line stuff
int g_cmd_index = 0;
int g_quitflag = 0;	// whether to quit or launch daphne

/////////////////
// "shared" variables

unsigned char g_dl_switch_a = 0;
unsigned char g_dl_switch_b = 0;
unsigned char g_ace_skill = 0;
unsigned int g_latency = 0;
char g_file_mask[81] = { 0 };

unsigned char g_ldp_type = LDP_SONY;
unsigned char g_serial_port = 0;
unsigned char g_baud_rate = 0;
unsigned char g_frame_mod = MOD_NONE;
unsigned char g_rsb = 0;

///////////////////////////////////////////////////////

void set_preset(int preset)
{
	g_dl_switch_a = presets[preset].dl_switch_a;
	g_dl_switch_b = presets[preset].dl_switch_b;
	g_ace_skill = presets[preset].ace_skill;
	g_latency = presets[preset].latency;
	strcpy(g_file_mask, presets[preset].file_mask);
}

// load initial values from a file and set them
void load_settings()
{

	set_preset(0);	// replace this with code to load current settings

}

// saves current settings to a file
void save__settings()
{

	// nothing here right now

}

///////////////////////////////////////////////

HWND get_main_hwnd()
// returns our handle
{

	return(Our_Hwnd);

}

// return the global hinstance
HINSTANCE get_g_hinst()
{

	return(g_hinst);

}

// give an error message with the 'OK' button

void Complain(
    HWND hwndOwner,
    LPCSTR pszMessage
)
{

	char s[81] = { 0 };
	sprintf(s, "DAPHNE's Front End got an error!");

    MessageBox(hwndOwner, pszMessage, s, MB_OK);

}


/*
// Called when Windows wants us to repaint our window
LRESULT Ex_OnPaint(HWND hwnd)
{

    PAINTSTRUCT ps;
	RECT current_window = { 0 };
    HDC hdc = BeginPaint(hwnd, &ps);

	// there's nothing to repaint now
    if (hdc)
	{
        EndPaint(hwnd, &ps);
    }

    return 0;

}
*/

LRESULT CALLBACK
Ex_WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{

	LRESULT result = 0;

    switch (msg) {

	case WM_CREATE:
		break;
//    case WM_PAINT:
//		result = Ex_OnPaint(hwnd);
//		break;
//    case WM_ACTIVATE:
//        g_fPaused = wParam == WA_INACTIVE;
//        Ex_SyncAcquire();
//        break;
    case WM_DESTROY:
		g_quitflag = 1;
        PostQuitMessage(0);
        break;
	default:
		result = DefWindowProc(hwnd, msg, wParam, lParam);
		break;
    }

    return(result);

}





////////////////////////////////////////////////////
// Global variables used by DaphneSetup
// Thanks to Gerg Hermann for this more elegant way of doing things

// this must be kept consistent with the data in preset.h
static char *rgszPresets[] = 
	{
	"0: Dragon's Lair Enhanced, Easy",
	"1: Dragon's Lair, Easy, 5 lives",
	"2: Dragon's Lair, Hard, 5 lives",
	"3: Dragon's Lair, Very Hard, 5 lives",
	"4: Space Ace, Cadet, Immortal",
	"5: Space Ace, Captain, Immortal",
	"6: Space Ace, Ace, Immortal",
	"7: Space Ace, Cadet, 5 lives",
	"8: Space Ace, Captain, 5 lives",
	"9: Space Ace, Ace, 5 lives",
	"10: Space Ace, no skill, 5 lives"
	};

static int cSzPresets = (sizeof(rgszPresets)/sizeof(char *));

// this must be kept in the same order as the enumerations in daphne.h
static char *rgszLDP[] =
	{
	"Sony",
	"Pioneer (Common)",
	"Pioneer LD-V6000's",
	"Hitachi VIP9550",
	"VLDP (doesn't work)",
	"SMPEG (doesn't work)",
	"Sony + VLDP (for sync tests)",
	"Sony + SMPEG (for sync tests)",
	"No LDP (for testing)"
	};

static int cSzLDP = (sizeof(rgszLDP)/sizeof(char *));

static char *rgszCOMPort[] =
	{
	"COM 1",
	"COM 2"
	};

static int cSzCOMPort = (sizeof(rgszCOMPort)/sizeof(char *));

static char *rgszBaud[] =
	{
	"9600",
	"4800"
	};

static int cSzBaud = (sizeof(rgszBaud)/sizeof(char *));

// this must be kept in the same order as the enumerations in daphne.h
static char *rgszSkill[] =
	{
	"Cadet",
	"Captain",
	"Space Ace",
	"Unspecified"
	};

static int cSzSkill = (sizeof(rgszSkill)/sizeof(char *));

// this must be kept in the same order as the enumerations in daphne.h
static char *rgszFrameMods[] =
{
		"None",
		"Space Ace '91",
		"PAL Dragon's Lair '83",
		"PAL Space Ace '83",
		"PAL Dragon's Lair Amiga"
};

static int cSzFrameMods = (sizeof(rgszFrameMods)/sizeof(char *));

static char *rgszRealScoreboard[] =
{
		"None",
		"LPT1",
		"LPT2"
};

static int cSzRealScoreboard = (sizeof(rgszRealScoreboard)/sizeof(char *));

// The dialog box that pops up to configure stuff in DAPHNE
BOOL CALLBACK DaphneSetup(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	BOOL result = false;	// we should return false when the dialog is first drawn
	long i = 0;

	// I did this to get rid of warnings
	if (lParam)
	{
	}

	switch(message)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDCANCEL:
			EndDialog(hdwnd, 0);
			g_quitflag = 1;
			result = true;
			break;
        case IDOK:
			// if all the changes the user proposed were valid, then accept them and boot the ROM!
			if (store_setup_changes(hdwnd))
			{
				EndDialog(hdwnd, 0);
			}
			result = true;
			break;
		case IDC_APPLY:
			i = SendDlgItemMessage(hdwnd, IDC_PRESET, CB_GETCURSEL, 0, 0);
			if (i != CB_ERR)
			{
				::set_preset(i);
			}
			else
			{
				::Complain(hdwnd, "You don't have anything to apply!");
			}
			refresh_daphne_setup(hdwnd);
			result = true;
			break;
		} // end inner switch
		break;
	case WM_INITDIALOG: // this gets called when the dialog box is created, so we do all our initialization in here
		{
			int i = 0;

			for(i = 0; i < cSzPresets; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_PRESET), rgszPresets[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_PRESET), 0);
			load_settings();	// set the initial values to appear in the dialog box

			for(i = 0; i < cSzLDP; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_LDP), rgszLDP[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_LDP), g_ldp_type);

			for(i = 0; i < cSzCOMPort; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_COMPORT), rgszCOMPort[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_COMPORT), g_serial_port);

			for(i = 0; i < cSzBaud; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_BAUD), rgszBaud[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_BAUD), g_baud_rate);

			for(i = 0; i < cSzSkill; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_SKILL), rgszSkill[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_SKILL), g_ace_skill);

			for (i = 0; i < cSzFrameMods; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_FRAMEMODS), rgszFrameMods[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_FRAMEMODS), g_frame_mod);

			for (i = 0; i < cSzRealScoreboard; i++)
			{
				ComboBox_AddString(GetDlgItem(hdwnd, IDC_SCOREBOARD), rgszRealScoreboard[i]);
			}

			ComboBox_SetCurSel(GetDlgItem(hdwnd, IDC_SCOREBOARD), g_rsb);

			refresh_daphne_setup(hdwnd);
			result = true;
		}
		break;
	default:
		break;
	} // end switch
	
	return(result);

}



// just a shortcut to SendDlgItemMessage

void add_to_combo(HWND hdwnd, int item, const char *text)
{
	SendDlgItemMessage(hdwnd, item, CB_ADDSTRING, 0, (LPARAM) text);
}


// sets/clears a checkbox based on is_set (1 = check it, 0 = clear it)
void change_checkbox(HWND hdwnd, int item, int is_set)
{

	if (is_set)
	{
		SendDlgItemMessage(hdwnd, item, BM_SETCHECK, BST_CHECKED, 0);
	}
	else
	{
		SendDlgItemMessage(hdwnd, item, BM_SETCHECK, BST_UNCHECKED, 0);
	}
}


// returns true if the checkbox is checked or 0 if it is not checked
bool get_checkbox(HWND hdwnd, int item)
{

	bool result = false;

	if (::SendDlgItemMessage(hdwnd, item, BM_GETCHECK, 0, 0) == BST_CHECKED)
	{
		result = true;
	}

	return(result);

}


// refreshes Daphne's setup dialog box (expert settings) with the current settings in memory
void refresh_daphne_setup(HWND hdwnd)
{

	char s[81] = { 0 };
	unsigned char dip = 0;	// temporary holding place for dip switch values

	SetDlgItemText(hdwnd, IDC_FILEMASK, g_file_mask);
	itoa(g_latency, s, 10);
	SetDlgItemText(hdwnd, IDC_LATENCY, s);
	SendDlgItemMessage(hdwnd, IDC_SKILL, CB_SETCURSEL, g_ace_skill, 0);

	// Below is where we set the dip switches
	// Unfortunately, this is a serious hack because I just want to get it done and can't think of any elegant way
	// to do it right now
	// The reason every value is XOR'd with 1 is because for some reason in Dragon's Lair
	// an ON dip switch is 0 and an OFF dip switch is 1, but a checked checkbox is 1 and an unchecked checkbox is 0
	// And as you should know, XOR'ing a bit with 1 will negate it

	dip = g_dl_switch_a;
	dip ^= 0xFF;
	change_checkbox(hdwnd, IDC_A0, (dip & 1));
	change_checkbox(hdwnd, IDC_A1, ((dip & 2) >> 1));
	change_checkbox(hdwnd, IDC_A2, ((dip & 4) >> 2));
	change_checkbox(hdwnd, IDC_A3, ((dip & 8) >> 3));
	change_checkbox(hdwnd, IDC_A4, ((dip & 16) >> 4));
	change_checkbox(hdwnd, IDC_A5, ((dip & 32) >> 5));
	change_checkbox(hdwnd, IDC_A6, ((dip & 64) >> 6));
	change_checkbox(hdwnd, IDC_A7, ((dip & 128) >> 7));

	dip = g_dl_switch_b;
	dip ^= 0xFF;
	change_checkbox(hdwnd, IDC_B0, (dip & 1));
	change_checkbox(hdwnd, IDC_B1, ((dip & 2) >> 1));
	change_checkbox(hdwnd, IDC_B2, ((dip & 4) >> 2));
	change_checkbox(hdwnd, IDC_B3, ((dip & 8) >> 3));
	change_checkbox(hdwnd, IDC_B4, ((dip & 16) >> 4));
	change_checkbox(hdwnd, IDC_B5, ((dip & 32) >> 5));
	change_checkbox(hdwnd, IDC_B6, ((dip & 64) >> 6));
	change_checkbox(hdwnd, IDC_B7, ((dip & 128) >> 7));

}



// checks to see if all of the changes the user has proposed during setup are valid
// if they are valid, it makes the changes and returns true
// else it returns false and doesn't make any changes
bool store_setup_changes(HWND hdwnd)
{

	bool result = true;
	long index = 0;	// result from SendDlgItemMessage command
	unsigned char ldp_candidate = 0;	// which LDP they're using
	unsigned int baud_candidate = 0;	// which baud rate they're using
	unsigned int port_candidate = 0;	// which serial port they're using
	unsigned int skill_candidate = 0;	// which skill they're using for space ace
	char mask_candidate[30] = { 0 };
	char latency_candidate[30] = { 0 };
	char s[81] = { 0 };	// temp string
	bool is_pal_dl = false;
	bool is_pal_sa = false;
	bool is_pal_dl_amiga = false;
	bool is_sa91 = false;
	unsigned char switchA_candidate = 0, switchB_candidate = 0;	// all switches on
	unsigned char rsb_exists = 0, rsb_port = 0;	// realscoreboard stuff

	// check to make sure the LDP field has something defined in it
	index = SendDlgItemMessage(hdwnd, IDC_LDP, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR)
	{
		ldp_candidate = static_cast<unsigned char>(index);

		switch(ldp_candidate)
		{
		case LDP_PIONEER:
			Complain(hdwnd, "The Pioneer driver has not been tested on all models and may not work on some of them.");
			break;
		case LDP_VLDP:
		case LDP_SONY_VLDP:
		case LDP_SMPEG:
		case LDP_SONY_SMPEG:
			Complain(hdwnd, "WARNING: This option does NOT work.");
			break;
		}
	}

	else
	{
		Complain(hdwnd, "You must specify your laserdisc player!");

		result = false;
	}

	// test COM port field
	index = SendDlgItemMessage(hdwnd, IDC_COMPORT, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR)
	{
		port_candidate = index;
	}

	else
	{
		Complain(hdwnd, "You must specify your serial port!");
		result = false;
	}

	// test baud rate field
	index = SendDlgItemMessage(hdwnd, IDC_BAUD, CB_GETCURSEL, 0, 0);
	if (index != CB_ERR)
	{
		if (index == 0)
		{
			baud_candidate = 9600;
		}
		else
		{
			baud_candidate = 4800;
		}
	}
	else
	{
		Complain(hdwnd, "You must specify your baud rate!");
		result = false;
	}

	index = SendDlgItemMessage(hdwnd, IDC_SKILL, CB_GETCURSEL, 0, 0);

	if (index != CB_ERR)
	{
		skill_candidate = index;
	}
	else
	{
		Complain(hdwnd, "You must specify a skill (even if you're not playing space ace)");
		result = false;
	}

	GetDlgItemText(hdwnd, IDC_FILEMASK, mask_candidate, 29);

	if (mask_candidate[0] == 0)
	{
		Complain(hdwnd, "You must specify a file name mask");
		result = false;
	}


	GetDlgItemText(hdwnd, IDC_LATENCY, latency_candidate, 29);

	if (latency_candidate[0] == 0)
	{
		Complain(hdwnd, "You must specify a search latency (use 0 if you don't know what to do)");
		result = false;
	}

	// test frame modifiers field

	index = SendDlgItemMessage(hdwnd, IDC_FRAMEMODS, CB_GETCURSEL, 0, 0);

	if (index != CB_ERR)
	{
		// Space Ace '91 laserdisc
		if (index == MOD_SA91)
		{
			is_sa91 = true;
		}

		// PAL DL
		else if (index == MOD_PAL_DL)
		{
			is_pal_dl = true;
		}

		else if (index == MOD_PAL_SA)
		{
			is_pal_sa = true;
		}

		else if (index == MOD_PAL_DL_AMIGA)
		{
			is_pal_dl_amiga = true;
		}

		// else they have chosen 'None' so we don't set anything

	}

	else
	{
		Complain(hdwnd, "You must specify a frame modifier (choose None if you don't know what to do)!");
		result = false;
	}

	index = SendDlgItemMessage(hdwnd, IDC_SCOREBOARD, CB_GETCURSEL, 0, 0);

	if (index != CB_ERR)
	{
		// no scoreboard present
		if (index == 0)
		{
			rsb_exists = 0;
		}

		// LPT1
		else if (index == 1)
		{
			rsb_exists = 1;
			rsb_port = 0;
		}

		// LPT2
		else
		{
			rsb_exists = 1;
			rsb_port = 1;
		}
	}

	else
	{
		Complain(hdwnd, "You must specify an external scoreboard choice (choose None if you don't have one)");
		result = false;
	}

	// Copy the values of the dip switch checkboxes into the proper memory locations

	switchA_candidate |= (get_checkbox(hdwnd, IDC_A0)) << 0;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A1)) << 1;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A2)) << 2;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A3)) << 3;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A4)) << 4;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A5)) << 5;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A6)) << 6;
	switchA_candidate |= (get_checkbox(hdwnd, IDC_A7)) << 7;
	switchA_candidate ^= 0xFF;

	switchB_candidate |= (get_checkbox(hdwnd, IDC_B0)) << 0;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B1)) << 1;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B2)) << 2;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B3)) << 3;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B4)) << 4;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B5)) << 5;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B6)) << 6;
	switchB_candidate |= (get_checkbox(hdwnd, IDC_B7)) << 7;
	switchB_candidate ^= 0xFF;

	// if there weren't any problems with the proposed changes, apply them
	if (result == true)
	{
		::sprintf(s, "-aceskill %d", skill_candidate);
		::add_to_cmdline(s);
		::sprintf(s, "-baud %u", baud_candidate);
		::add_to_cmdline(s);
		sprintf(s, "-port %u", port_candidate);
		add_to_cmdline(s);
		switch(ldp_candidate)
		{
		case LDP_SONY:
			sprintf(s, "-sony");
			break;
		case LDP_PIONEER:
			sprintf(s, "-pioneer");
			break;
		case LDP_V6000:
			sprintf(s, "-v6000");
			break;
		case LDP_HITACHI:
			sprintf(s, "-hitachi");
			break;
		case LDP_VLDP:
			sprintf(s, "-vldp");
			break;
		case LDP_SMPEG:
			sprintf(s, "-smpeg");
			break;
		case LDP_SONY_VLDP:
			sprintf(s, "-sonyvldp");
			break;
		case LDP_SONY_SMPEG:
			sprintf(s, "-sonysmpeg");
			break;
		case LDP_NONE:
			sprintf(s, "-noldp");
			break;
		default:
			Complain(Our_Hwnd, "Bug, unknown LDP type!");
			break;
		}
		add_to_cmdline(s);

		sprintf(s, "-filemask %s", mask_candidate);
		add_to_cmdline(s);

		sprintf(s, "-latency %s", latency_candidate);
		add_to_cmdline(s);

		sprintf(s, "-dl_switcha %d", switchA_candidate);
		add_to_cmdline(s);

		sprintf(s, "-dl_switchb %d", switchB_candidate);
		add_to_cmdline(s);

		// if they specified a real sccoreboard
		if (rsb_exists)
		{
			strcpy(s, "-scoreboard");
			add_to_cmdline(s);

			sprintf(s, "-scoreport %d", rsb_port);
			add_to_cmdline(s);
		}

		// if they are using a PAL Dragon's Lair disc
		if (is_pal_dl)
		{
			strcpy(s, "-pal_dl");
			add_to_cmdline(s);
		}

		// if they are using a Space Ace '91 disc
		else if (is_sa91)
		{
			strcpy(s, "-spaceace91");
			add_to_cmdline(s);
		}
		else if (is_pal_sa)
		{
			strcpy(s, "-pal_sa");
			add_to_cmdline(s);
		}
		else if (is_pal_dl_amiga)
		{
			strcpy(s, "-pal_dl_amiga");
			add_to_cmdline(s);
		}
	}

	return(result);

}

////////////////////////////////

void arg_test()
{

	FILE *F = NULL;
	int i = 0;
	char s[81] = { 0 };

	F = fopen("blah.txt", "wb");
	if  (F)
	{
		for (i = 0; i < g_cmd_index; i++)
		{
			sprintf(s, "Arg %d : %s\n", i, g_cmd_array[i]);
			fputs(s, F);
		}
		fclose(F);
	}
}


// Initializes our window that we get from WinMain
HWND AppInit(HINSTANCE hinst, int nCmdShow)
{

    WNDCLASS wc;

	// I put this here JUST to get rid of warnings! nothing more
	if (nCmdShow)
	{
	}

    wc.hCursor        = LoadCursor(0, IDC_ARROW);
    wc.hIcon          = LoadIcon(hinst, "IDI_DAPHNE");
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = c_szClassName;
    wc.hbrBackground  = 0;
    wc.hInstance      = hinst;
    wc.style          = 0;
    wc.lpfnWndProc    = Ex_WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;

    if (!RegisterClass(&wc))
	{
        return NULL;
    }

    HWND hwnd = CreateWindow(
                    c_szClassName,                  // Class name
                    "DAPHNE - First Ever Multiple Arcade Laserdisc Emulator =]",       // Caption
                    WS_OVERLAPPEDWINDOW,            // Style
                    CW_USEDEFAULT, CW_USEDEFAULT,   // Position
                    VID_WIDTH, VID_HEIGHT,   // Size
                    NULL,                           // No parent
                    NULL,                           // No menu
                    g_hinst,                        // inst handle
                    0                               // no params
                    );

//    ShowWindow(hwnd, nCmdShow);

    return hwnd;

}


// WINMAIN!
int PASCAL WinMain(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR szCmdLine, int nCmdShow)
{

    MSG msg;
    msg.wParam = 0;         /* In case something goes horribly wrong */

	g_hinst = hinst;

    HWND hwnd = AppInit(hinst, nCmdShow);

	// I did this JUST to get rid of compiler warnings, don't be confused ...
	if (hinstPrev)
	{
	}

    if (hwnd)
	{
		Our_Hwnd = hwnd;
		add_to_cmdline("daphne.exe");	// first element for new command line is filename to execute
		DialogBox(hinst, "MYDIALOG", hwnd, (DLGPROC) DaphneSetup);

		if (!g_quitflag)
		{
			for (int i = 0; i < g_cmd_index; i++)
			{
				g_cmd_array_ptrs[i] = g_cmd_array[i];
			}

			if (spawnvp(P_NOWAIT, g_cmd_array[0], g_cmd_array_ptrs) < 0)
			{
				Complain(Our_Hwnd, "Could not execute main daphne program");
			}
		}
	}

    return msg.wParam;

}


// adds an entry to the growing command line
// returns 1 on success, 0 on failure
int add_to_cmdline(char *s)
{

	int result = 0;

	if (strlen(s) > 0)
	{
		if ((g_cmd_index < CMD_ARRAY_SIZE))
		{
			strcpy(g_cmd_array[g_cmd_index], s);
			result = 1;
			g_cmd_index++;
		}
		else
		{
			Complain(Our_Hwnd, "Could not allocate memory for command line");
		}
	}
	else
	{
		Complain(Our_Hwnd, "Bad strlen in add_to_cmdline");
	}

	return(result);
}
