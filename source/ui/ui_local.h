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


#ifndef __UI_LOCAL_H
#define __UI_LOCAL_H__


#include "../client/cl_local.h"


#define UI_CURSOR_NORMAL		"ui/misc/cursor"
#define UI_CURSOR_DISABLED		"ui/misc/cursor_denied"
#define UI_CURSOR_TYPING		"ui/misc/cursor_type"
#define UI_LEFTARROW			"ui/arrows/arrow_left_small"
#define UI_LEFTARROWFOCUS		"ui/arrows/arrow_left_small_s"
#define UI_RIGHTARROW			"ui/arrows/arrow_right_small"
#define UI_RIGHTARROWFOCUS		"ui/arrows/arrow_right_small_s"
#define UI_UPARROW				"ui/arrows/arrow_up_big"
#define UI_UPARROWFOCUS			"ui/arrows/arrow_up_big_s"
#define UI_DOWNARROW			"ui/arrows/arrow_down_big"
#define UI_DOWNARROWFOCUS		"ui/arrows/arrow_down_big_s"
#define UI_BACKGROUNDLISTBOX	"ui/segments/list_mid"
#define UI_SELECTIONBOX			"ui/misc/list_sel"
#define UI_BACKGROUNDBOX		"ui/buttons/options2_b"
#define UI_MOVEBOX				"ui/buttons/move_box"
#define UI_MOVEBOXFOCUS			"ui/buttons/move_box_s"
#define UI_BACKBUTTON			"ui/buttons/back_b"
#define UI_LOADBUTTON			"ui/buttons/load_b"
#define UI_SAVEBUTTON			"ui/buttons/save_b"
#define UI_DELETEBUTTON			"ui/buttons/delete_b"
#define UI_CANCELBUTTON			"ui/buttons/cancel_b"
#define UI_APPLYBUTTON			"ui/buttons/apply_b"
#define UI_ACCEPTBUTTON			"ui/buttons/accept_b"
#define UI_PLAYBUTTON			"ui/buttons/play_b"
#define UI_STARTBUTTON			"ui/buttons/fight_b"
#define UI_NEWGAMEBUTTON		"ui/buttons/newgame_b"

#define UI_MAX_MENUDEPTH		8
#define UI_MAX_MENUITEMS		64

#define UI_CURSOR_SIZE			40

#define UI_PULSE_DIVISOR		75
#define UI_BLINK_TIME			250
#define UI_BLINK_MASK			499

#define UI_SMALL_CHAR_WIDTH		10
#define UI_SMALL_CHAR_HEIGHT	20
#define UI_BIG_CHAR_WIDTH		20
#define UI_BIG_CHAR_HEIGHT		40

#define UI_MAX_FIELD_LINE		256

#define UI_MAX_SERVERS			10

// Generic types
typedef enum {
	QMTYPE_SCROLLLIST,
	QMTYPE_SPINCONTROL,
	QMTYPE_FIELD,
	QMTYPE_ACTION,
	QMTYPE_BITMAP
} menuType_t;

// Generic flags
#define QMF_LEFT_JUSTIFY		0x00000001
#define QMF_CENTER_JUSTIFY		0x00000002
#define QMF_RIGHT_JUSTIFY		0x00000004
#define QMF_GRAYED				0x00000008	// Grays and disables
#define QMF_INACTIVE			0x00000010	// Disables any input
#define QMF_HIDDEN				0x00000020	// Doesn't draw
#define QMF_NUMBERSONLY			0x00000040	// Edit field is only numbers
#define QMF_LOWERCASE			0x00000080	// Edit field is all lower case
#define QMF_UPPERCASE			0x00000100	// Edit field is all upper case
#define QMF_BLINKIFFOCUS		0x00000200
#define QMF_PULSEIFFOCUS		0x00000400
#define QMF_HIGHLIGHTIFFOCUS	0x00000800
#define QMF_SMALLFONT			0x00001000
#define QMF_BIGFONT				0x00002000
#define QMF_DROPSHADOW			0x00004000
#define QMF_SILENT				0x00008000	// Don't play sounds
#define QMF_HASMOUSEFOCUS		0x00010000
#define QMF_MOUSEONLY			0x00020000	// Only mouse input allowed
#define QMF_FOCUSBEHIND			0x00040000	// Focus draws behind normal item

// Callback notifications
#define QM_GOTFOCUS				1
#define QM_LOSTFOCUS			2
#define QM_ACTIVATED			3
#define QM_CHANGED				4

typedef struct {
	int				cursor;
	int				cursorPrev;

	void			*items[UI_MAX_MENUITEMS];
	int				numItems;

	void			(*drawFunc) (void);
	const char		*(*keyFunc) (int key);
} menuFramework_s;

typedef struct {
	menuType_t		type;
	const char		*name;
	int				id;

	unsigned		flags;

	int				x;
	int				y;
	int				width;
	int				height;

	int				x2;
	int				y2;
	int				width2;
	int				height2;

	byte			*color;
	byte			*focusColor;

	int				charWidth;
	int				charHeight;

	const char		*statusText;

	menuFramework_s	*parent;

	void			(*callback) (void *self, int event);
	void			(*ownerdraw) (void *self);
} menuCommon_s;

typedef struct {
	menuCommon_s	generic;

	const char		*background;
	const char		*upArrow;
	const char		*upArrowFocus;
	const char		*downArrow;
	const char		*downArrowFocus;
	const char		**itemNames;
	int				numItems;
	int				curItem;
	int				topItem;
	int				numRows;
} menuScrollList_s;

typedef struct {
	menuCommon_s	generic;

	const char		*background;
	const char		*leftArrow;
	const char		*rightArrow;
	const char		*leftArrowFocus;
	const char		*rightArrowFocus;
	float			minValue;
	float			maxValue;
	float			curValue;
	float			range;
} menuSpinControl_s;

typedef struct {
	menuCommon_s	generic;

	const char		*background;
	int				maxLenght;
	int				visibleLength;
	char			buffer[UI_MAX_FIELD_LINE];
	int				length;
	int				cursor;
} menuField_s;

typedef struct {
	menuCommon_s	generic;

	const char		*background;
} menuAction_s;

typedef struct {
	menuCommon_s	generic;

	const char		*pic;
	const char		*focusPic;
} menuBitmap_s;

void		UI_ScrollList_Init (menuScrollList_s *sl);
const char	*UI_ScrollList_Key (menuScrollList_s *sl, int key);
void		UI_ScrollList_Draw (menuScrollList_s *sl);

void		UI_SpinControl_Init (menuSpinControl_s *sc);
const char	*UI_SpinControl_Key (menuSpinControl_s *sc, int key);
void		UI_SpinControl_Draw (menuSpinControl_s *sc);

void		UI_Field_Init (menuField_s *f);
const char	*UI_Field_Key (menuField_s *f, int key);
void		UI_Field_Draw (menuField_s *f);

void		UI_Action_Init (menuAction_s *t);
const char	*UI_Action_Key (menuAction_s *t, int key);
void		UI_Action_Draw (menuAction_s *t);

void		UI_Bitmap_Init (menuBitmap_s *b);
const char	*UI_Bitmap_Key (menuBitmap_s *b, int key);
void		UI_Bitmap_Draw (menuBitmap_s *b);

// =====================================================================
// Main menu interface

extern cvar_t		*ui_precache;
extern cvar_t		*ui_sensitivity;
extern cvar_t		*ui_singlePlayerSkill;

typedef struct {
	char		name[MAX_QPATH];
	cinHandle_t	handle;
} uiCin_t;

typedef struct {
	menuFramework_s	*menuActive;
	menuFramework_s	*menuStack[UI_MAX_MENUDEPTH];
	int				menuDepth;

	uiCin_t			cinematics[MAX_CINEMATICS];

	netAdr_t		serverAddresses[UI_MAX_SERVERS];
	char			serverNames[UI_MAX_SERVERS][80];
	int				numServers;

	glConfig_t		glConfig;
	alConfig_t		alConfig;

	float			scaleX;
	float			scaleY;

	int				cursorX;
	int				cursorY;
	int				realTime;
	qboolean		firstDraw;
	qboolean		enterSound;
	qboolean		visible;

	qboolean		initialized;
} uiStatic_t;

extern uiStatic_t	uiStatic;

extern const char	*uiSoundIn;
extern const char	*uiSoundMove;
extern const char	*uiSoundOut;
extern const char	*uiSoundBuzz;
extern const char	*uiSoundNull;

extern color_t		uiColorWhite;
extern color_t		uiColorLtGrey;
extern color_t		uiColorMdGrey;
extern color_t		uiColorDkGrey;
extern color_t		uiColorBlack;
extern color_t		uiColorRed;
extern color_t		uiColorGreen;
extern color_t		uiColorBlue;
extern color_t		uiColorYellow;
extern color_t		uiColorCyan;
extern color_t		uiColorMagenta;

void		UI_ScaleCoords (int *x, int *y, int *w, int *h);
qboolean	UI_CursorInRect (int x, int y, int w, int h);
void		UI_DrawPic (int x, int y, int w, int h, const color_t color, const char *pic);
void		UI_FillRect (int x, int y, int w, int h, const color_t color);
void		UI_DrawString (int x, int y, int w, int h, const char *string, const color_t color, qboolean forceColor, int charW, int charH, int justify, qboolean shadow);
cinHandle_t	UI_PlayCinematic (const char *name);
void		UI_DrawCinematic (cinHandle_t handle, int x, int y, int width, int height);
void		UI_StopCinematic (cinHandle_t handle);
void		UI_StartSound (const char *sound);

void		UI_AddItem (menuFramework_s *menu, void *item);
void		UI_CursorMoved (menuFramework_s *menu);
void		UI_SetCursor (menuFramework_s *menu, int cursor);
void		UI_SetCursorToItem (menuFramework_s *menu, void *item);
void		*UI_ItemAtCursor (menuFramework_s *menu);
void		UI_AdjustCursor (menuFramework_s *menu, int dir);
void		UI_DrawMenu (menuFramework_s *menu);
const char	*UI_DefaultKey (menuFramework_s *menu, int key);
const char	*UI_ActivateItem (menuFramework_s *menu, menuCommon_s *item);
void		UI_RefreshServerList (void);

void		UI_CloseMenu (void);
void		UI_PushMenu (menuFramework_s *menu);
void		UI_PopMenu (void);

// Precache
void		UI_Main_Precache (void);
void		UI_InGame_Precache (void);
	void		UI_SinglePlayer_Precache (void);
		void		UI_LoadGame_Precache (void);
		void		UI_SaveGame_Precache (void);
	void		UI_MultiPlayer_Precache (void);
	void		UI_Options_Precache (void);
		void		UI_PlayerSetup_Precache (void);
		void		UI_Controls_Precache (void);
		void		UI_GameOptions_Precache (void);
		void		UI_Audio_Precache (void);
		void		UI_Video_Precache (void);
			void		UI_Advanced_Precache (void);
		void		UI_Performance_Precache (void);
		void		UI_Network_Precache (void);
		void		UI_Defaults_Precache (void);
	void		UI_Cinematics_Precache (void);
	void		UI_Demos_Precache (void);
	void		UI_Mods_Precache (void);
	void		UI_Quit_Precache (void);
		void		UI_Credits_Precache (void);

void		UI_GoToSite_Precache (void);

// Menus
void		UI_Main_Menu (void);
void		UI_InGame_Menu (void);
	void		UI_SinglePlayer_Menu (void);
		void		UI_LoadGame_Menu (void);
		void		UI_SaveGame_Menu (void);
	void		UI_MultiPlayer_Menu (void);
	void		UI_Options_Menu (void);
		void		UI_PlayerSetup_Menu (void);
		void		UI_Controls_Menu (void);
		void		UI_GameOptions_Menu (void);
		void		UI_Audio_Menu (void);
		void		UI_Video_Menu (void);
			void		UI_Advanced_Menu (void);
		void		UI_Performance_Menu (void);
		void		UI_Network_Menu (void);
		void		UI_Defaults_Menu (void);
	void		UI_Cinematics_Menu (void);
	void		UI_Demos_Menu (void);
	void		UI_Mods_Menu (void);
	void		UI_Quit_Menu (void);
		void		UI_Credits_Menu (void);

void		UI_GoToSite_Menu (const char *webAddress);


#endif	// __UI_LOCAL_H__
