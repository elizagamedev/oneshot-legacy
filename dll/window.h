#ifndef WINDOW_H
#define WINDOW_H

#include "globals.h"

// For the kitty escape window
extern HWND wndKitty;

extern BOOL wndKitty_isActive;

void wndKitty_create();
void wndKitty_start();
void wndKitty_destroy();

#endif
