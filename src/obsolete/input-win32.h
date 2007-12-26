
#include <dinput.h>

#define JOY_AXIS_MAX 1000	// maximum range of any joystick axis
#define JOY_AXIS_MID 500	// what range we have to exceed on joystick to have a move acknowledge

BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* pContext );
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                VOID* pContext );
BOOL DIInit(HWND hwnd);
void DITerm(void);
char win32_getkey();
void win32_check_keyboard();
void win32_check_joystick();
void Ex_SyncAcquire();

