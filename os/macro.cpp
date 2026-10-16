#include"macro.h"

void set_console_mode_text() {
	console_mode_stack.push(_setmode(_fileno(stdout), _O_TEXT));
}

void set_console_mode_wtext() {
	console_mode_stack.push(_setmode(_fileno(stdout), _O_WTEXT));
}

void restore_console_mode() {
	if (console_mode_stack.empty())
		return;

	console_old_mode = _setmode(_fileno(stdout), console_mode_stack.top());
	console_mode_stack.pop();
}