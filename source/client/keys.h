/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#ifndef __KEYS_H__
#define __KEYS_H__


// These are the key numbers that should be passed to Key_Event
// Normal keys should be passed as lowercased ASCII
enum {
	K_TAB = 9,
	K_ENTER	= 13,
	K_ESCAPE = 27,
	K_SPACE = 32,

	K_BACKSPACE	= 127,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_ALT,
	K_CTRL,
	K_SHIFT,
	
	K_INS,
	K_DEL,
	K_PGDN,
	K_PGUP,
	K_HOME,
	K_END,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,
	
	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PGUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PGDN,
	K_KP_INS,
	K_KP_DEL,
	K_KP_SLASH,
	K_KP_STAR,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_ENTER,
	K_KP_NUMLOCK,

	K_PAUSE,
	K_APPS,
	K_CAPSLOCK,

	// Mouse buttons generate virtual keys
	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,

	K_MWHEELDOWN,
	K_MWHEELUP,

	// Joystick buttons
	K_JOY1,
	K_JOY2,
	K_JOY3,
	K_JOY4,

	// Aux keys are for multi-buttoned joysticks to generate so they can
	// use the normal binding process
	K_AUX1,
	K_AUX2,
	K_AUX3,
	K_AUX4,
	K_AUX5,
	K_AUX6,
	K_AUX7,
	K_AUX8,
	K_AUX9,
	K_AUX10,
	K_AUX11,
	K_AUX12,
	K_AUX13,
	K_AUX14,
	K_AUX15,
	K_AUX16,
	K_AUX17,
	K_AUX18,
	K_AUX19,
	K_AUX20,
	K_AUX21,
	K_AUX22,
	K_AUX23,
	K_AUX24,
	K_AUX25,
	K_AUX26,
	K_AUX27,
	K_AUX28,
	K_AUX29,
	K_AUX30,
	K_AUX31,
	K_AUX32
};

typedef enum {
	KEY_GAME,
	KEY_CONSOLE,
	KEY_MESSAGE,
	KEY_MENU
} keyDest_t;

int			Key_StringToKeyNum (const char *string);
char		*Key_KeyNumToString (int keyNum);
void		Key_SetBinding (int keyNum, const char *binding);
char		*Key_GetBinding (int keyNum);
void		Key_WriteBindings (fileHandle_t f);

keyDest_t	Key_GetKeyDest (void);
void		Key_SetKeyDest (keyDest_t dest);
qboolean	Key_IsDown (int key);
qboolean	Key_IsAnyKeyDown (void);
void		Key_ClearStates (void);
void		Key_Event (int key, qboolean down, unsigned time);
int			Key_MapKey (int key);

void		Key_Init (void);
void		Key_Shutdown (void);


#endif	// __KEYS_H__
