#pragma once
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include<stack>
//仅仅为了支持两个Unicode表情
void set_console_mode_text();
void set_console_mode_wtext();
void restore_console_mode();

inline  std::stack<int> console_mode_stack;
inline int console_old_mode = 0;

#define A_START  set_console_mode_text();
#define A_STOP   do{	\
						std::cout.flush();\
						restore_console_mode();\
				}while (0);

#define W_START  set_console_mode_wtext();
#define W_STOP   do{	\
						std::wcout.flush();\
						restore_console_mode();\
				}while (0);