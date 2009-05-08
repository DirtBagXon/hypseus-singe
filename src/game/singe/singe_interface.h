#ifndef SINGE_INTERFACE_H
#define SINGE_INTERFACE_H

// increase this number every time you change something in this file!!!
#define SINGE_INTERFACE_API_VERSION 3

// info provided to Singe from Daphne
struct singe_in_info
{
	// the API version (THIS MUST COME FIRST!)
	unsigned int uVersion;

	// FUNCTIONS:

	// shuts down daphne
	void (*set_quitflag)();

	// print a line to the debug console, and the log file (and to stdout on some platforms)
	void (*printline)(const char *);
	
	// notifies daphne of the last error that singe got (so daphne can display it)
	void (*set_last_error)(const char *);

	// From video/video.h
	Uint16 (*get_video_width)();
	Uint16 (*get_video_height)();
	void (*draw_string)(const char*, int, int, SDL_Surface*);
	
	// From sound/samples.h
	int (*samples_play_sample)(Uint8 *pu8Buf, unsigned int uLength, unsigned int uChannels, int iSlot, void (*finishedCallback)(Uint8 *pu8Buf, unsigned int uSlot));

	// Laserdisc Control Functions
	void (*enable_audio1)();
	void (*enable_audio2)();
	void (*disable_audio1)();
	void (*disable_audio2)();
	void (*request_screenshot)();
	void (*set_search_blanking)(bool enabled);
	void (*set_skip_blanking)(bool enabled);
	bool (*pre_change_speed)(unsigned int uNumerator, unsigned int uDenominator);
	unsigned int (*get_current_frame)();
	void (*pre_play)();
	void (*pre_pause)();
	void (*pre_stop)();
	bool (*pre_search)(const char *, bool block_until_search_finished);
	void (*framenum_to_frame)(Uint16, char *);
	bool (*pre_skip_forward)(Uint16);
	bool (*pre_skip_backward)(Uint16);
	void (*pre_step_forward)();
	void (*pre_step_backward)();

	// VARIABLES:
	
	// VLDP Interface
	struct vldp_in_info        *g_local_info;
	const struct vldp_out_info *g_vldp_info;
};

// info provided from Singe to Daphne
struct singe_out_info
{
	// the API version (THIS MUST COME FIRST!)
	unsigned int uVersion;

	// FUNCTIONS:
	void (*sep_call_lua)(const char *func, const char *sig, ...);
	void (*sep_do_blit)(SDL_Surface *srfDest);
	void (*sep_do_mouse_move)(Uint16 x, Uint16 y, Sint16 xrel, Sint16 yrel);
	void (*sep_error)(const char *fmt, ...);
	void (*sep_print)(const char *fmt, ...);
	void (*sep_set_static_pointers)(double *m_disc_fps, unsigned int *m_uDiscFPKS);
	void (*sep_set_surface)(int width, int height);
	void (*sep_shutdown)(void);
	void (*sep_startup)(const char *script);
	
	////////////////////////////////////////////////////////////
};

// if you want to build singe as a DLL, then define SINGE_DLL in your DLL project's preprocessor defs
#ifdef SINGE_DLL
#define SINGE_EXPORT __declspec(dllexport)
#else
// otherwise SINGE_EXPORT is blank
#define SINGE_EXPORT
#endif

extern "C"
{
SINGE_EXPORT const struct singe_out_info *singeproxy_init(const struct singe_in_info *in_info);
}

#endif // SINGE_INTERFACE_H
