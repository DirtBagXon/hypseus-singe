// header file for win32.cpp



HWND get_main_hwnd();
HINSTANCE get_g_hinst();
void Complain(HWND hwndOwner,LPCSTR pszMessage);
void Ex_SyncAcquire();
BOOL CALLBACK DaphneSetup(HWND hdwnd, UINT message, WPARAM wParam, LPARAM lParam);
void add_to_combo(HWND hdwnd, int item, const char *text);
void change_checkbox(HWND hdwnd, int item, int is_set);
bool get_checkbox(HWND hdwnd, int item);
void refresh_daphne_setup(HWND hdwnd);
bool store_setup_changes(HWND hdwnd);
int add_to_cmdline(char *s);
void free_cmdline();
