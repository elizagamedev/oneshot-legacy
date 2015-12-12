#ifndef FUNCS_H
#define FUNCS_H

#include "globals.h"

extern void (* const funcs[])(void);

void func_init();

//When the main window is closed/destroyed
void func_Close();
void func_Destroy();

#endif
