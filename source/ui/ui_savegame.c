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


#include "ui_local.h"


#define ART_BACKGROUND			"ui/misc/ui_sub_singleplayer"
#define ART_BANNER				"ui/banners/savegame_t"
#define ART_LISTBACK			"ui/segments/files_box"
#define ART_LEVELSHOTBLUR		"ui/segments/sp_mapshot"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_SAVE				3

#define ID_LISTBACK			4
#define ID_GAMETITLE		5
#define ID_LISTGAMES		6
#define ID_LEVELSHOT		20

#define ID_NEWGAME			21
#define ID_LOADGAME			22
#define ID_DELETEGAME		23

typedef struct {
	char					map[80];
	char					time[8];
	char					date[8];
	char					name[32];
	qboolean				valid;
} uiSaveGameGame_t;

static color_t				uiSaveGameColor = {0, 76, 127, 255};

typedef struct {
	uiSaveGameGame_t		games[14];
	int						currentGame;

	int						currentLevelShot;
	int						fadeTime;

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			back;
	menuBitmap_s			save;

	menuBitmap_s			listBack;
	menuAction_s			gameTitle;
	menuAction_s			listGames[14];	
	menuBitmap_s			levelShot;

	menuBitmap_s			newGame;
	menuBitmap_s			loadGame;
	menuBitmap_s			deleteGame;
} uiSaveGame_t;

static uiSaveGame_t			uiSaveGame;


/*
 =================
 UI_SaveGame_GetGameList
 =================
*/
static void UI_SaveGame_GetGameList (void){

	int		i;
	char	name[MAX_QPATH];
	char	*buffer;

	for (i = 0; i < 14; i++){
		Q_snprintfz(name, sizeof(name), "save/save%i/server.ssv", i);
		FS_LoadFile(name, (void **)&buffer);
		if (!buffer){
			Q_strncpyz(uiSaveGame.games[i].map, "", sizeof(uiSaveGame.games[i].map));
			Q_strncpyz(uiSaveGame.games[i].time, "", sizeof(uiSaveGame.games[i].time));
			Q_strncpyz(uiSaveGame.games[i].date, "", sizeof(uiSaveGame.games[i].date));
			Q_strncpyz(uiSaveGame.games[i].name, "<EMPTY>", sizeof(uiSaveGame.games[i].name));
			uiSaveGame.games[i].valid = false;

			continue;
		}

		if (strstr(buffer, "ENTERING")){
			memcpy(uiSaveGame.games[i].map, buffer+32, 80-32);
			Q_strncpyz(uiSaveGame.games[i].time, "", sizeof(uiSaveGame.games[i].time));
			Q_strncpyz(uiSaveGame.games[i].date, "", sizeof(uiSaveGame.games[i].date));
			memcpy(uiSaveGame.games[i].name, buffer, 32);
		}
		else {
			memcpy(uiSaveGame.games[i].map, buffer+32, 80-32);
			memcpy(uiSaveGame.games[i].time, buffer, 5);
			memcpy(uiSaveGame.games[i].date, buffer+6, 5);
			memcpy(uiSaveGame.games[i].name, buffer+13, 32-13);
		}
		uiSaveGame.games[i].valid = true;

		FS_FreeFile(buffer);
	}

	// Select first empty slot
	for (i = 0; i < 14; i++){
		if (!uiSaveGame.games[i].valid){
			uiSaveGame.listGames[i].generic.color = uiSaveGameColor;
			uiSaveGame.currentGame = i;
			break;
		}
	}

	// Couldn't find an empty slot, so select first
	if (i == 14){
		uiSaveGame.listGames[i].generic.color = uiSaveGameColor;
		uiSaveGame.currentGame = 0;
	}
}

/*
 =================
 UI_SaveGame_Callback
 =================
*/
static void UI_SaveGame_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event != QM_ACTIVATED)
		return;

	if (item->type == QMTYPE_ACTION){
		// Reset color, get current game, set color
		uiSaveGame.listGames[uiSaveGame.currentGame].generic.color = uiColorWhite;
		uiSaveGame.currentGame = item->id - ID_LISTGAMES;
		uiSaveGame.listGames[uiSaveGame.currentGame].generic.color = uiSaveGameColor;

		// Restart levelshot animation
		uiSaveGame.currentLevelShot = 0;
		uiSaveGame.fadeTime = uiStatic.realTime;
		return;
	}
	
	switch (item->id){
	case ID_BACK:
		if (cls.state == CA_ACTIVE)
			UI_InGame_Menu();
		else
			UI_Main_Menu();

		break;
	case ID_SAVE:
		if (Com_ServerState()){
			Cbuf_ExecuteText(EXEC_APPEND, va("savegame save%i\n", uiSaveGame.currentGame));
			UI_CloseMenu();;
		}
		break;
	case ID_NEWGAME:
		UI_SinglePlayer_Menu();
		break;
	case ID_LOADGAME:
		UI_LoadGame_Menu();
		break;
	case ID_DELETEGAME:
		Cbuf_ExecuteText(EXEC_NOW, va("deletegame save%i\n", uiSaveGame.currentGame));
		UI_SaveGame_GetGameList();
		break;
	}
}

/*
 =================
 UI_SaveGame_Ownerdraw
 =================
*/
static void UI_SaveGame_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (item->type == QMTYPE_ACTION){
		color_t		color;
		char		*time, *date, *name;
		qboolean	centered;

		if (item->id == ID_GAMETITLE){
			time = "Time";
			date = "Date";
			name = "Map Name";
			centered = false;
		}
		else {
			time = uiSaveGame.games[item->id - ID_LISTGAMES].time;
			date = uiSaveGame.games[item->id - ID_LISTGAMES].date;
			name = uiSaveGame.games[item->id - ID_LISTGAMES].name;
			centered = !uiSaveGame.games[item->id - ID_LISTGAMES].valid;

			if (strstr(uiSaveGame.games[item->id - ID_LISTGAMES].name, "ENTERING"))
				centered = true;
		}

		if (!centered){
			UI_DrawString(item->x, item->y, 82*uiStatic.scaleX, item->height, time, item->color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawString(item->x + 83*uiStatic.scaleX, item->y, 82*uiStatic.scaleX, item->height, date, item->color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawString(item->x + 83*uiStatic.scaleX + 83*uiStatic.scaleX, item->y, 296*uiStatic.scaleX, item->height, name, item->color, true, item->charWidth, item->charHeight, 1, true);
		}
		else
			UI_DrawString(item->x, item->y, item->width, item->height, name, item->color, true, item->charWidth, item->charHeight, 1, true);

		if (self != UI_ItemAtCursor(item->parent))
			return;

		*(unsigned *)color = *(unsigned *)item->color;
		color[3] = 255 * (0.5 + 0.5 * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		if (!centered){
			UI_DrawString(item->x, item->y, 82*uiStatic.scaleX, item->height, time, color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawString(item->x + 83*uiStatic.scaleX, item->y, 82*uiStatic.scaleX, item->height, date, color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawString(item->x + 83*uiStatic.scaleX + 83*uiStatic.scaleX, item->y, 296*uiStatic.scaleX, item->height, name, color, true, item->charWidth, item->charHeight, 1, true);
		}
		else
			UI_DrawString(item->x, item->y, item->width, item->height, name, color, true, item->charWidth, item->charHeight, 1, true);
	}
	else {
		if (item->id == ID_LEVELSHOT){
			int			x, y, w, h;
			int			prev;
			color_t		color = {255, 255, 255, 255};

			// Draw the levelshot
			x = 570;
			y = 210;
			w = 410;
			h = 202;
		
			UI_ScaleCoords(&x, &y, &w, &h);

			if (uiSaveGame.games[uiSaveGame.currentGame].map[0]){
				char	pathTGA[MAX_QPATH], pathJPG[MAX_QPATH];

				if (uiStatic.realTime - uiSaveGame.fadeTime >= 3000){
					uiSaveGame.fadeTime = uiStatic.realTime;

					uiSaveGame.currentLevelShot++;
					if (uiSaveGame.currentLevelShot == 3)
						uiSaveGame.currentLevelShot = 0;
				}

				prev = uiSaveGame.currentLevelShot - 1;
				if (prev < 0)
					prev = 2;

				color[3] = Clamp((float)(uiStatic.realTime - uiSaveGame.fadeTime) / 1000, 0.0, 1.0) * 255;

				Q_snprintfz(pathTGA, sizeof(pathTGA), "ui/menu_levelshots/%s_1.tga", uiSaveGame.games[uiSaveGame.currentGame].map);
				Q_snprintfz(pathJPG, sizeof(pathJPG), "ui/menu_levelshots/%s_1.jpg", uiSaveGame.games[uiSaveGame.currentGame].map);

				if (FS_LoadFile(pathTGA, NULL) == -1 && FS_LoadFile(pathJPG, NULL) == -1)
					UI_DrawPic(x, y, w, h, uiColorWhite, "ui/menu_levelshots/unknownmap");
				else {
					UI_DrawPic(x, y, w, h, uiColorWhite, va("ui/menu_levelshots/%s_%i", uiSaveGame.games[uiSaveGame.currentGame].map, prev+1));
					UI_DrawPic(x, y, w, h, color, va("ui/menu_levelshots/%s_%i", uiSaveGame.games[uiSaveGame.currentGame].map, uiSaveGame.currentLevelShot+1));
				}
			}
			else
				UI_DrawPic(x, y, w, h, uiColorWhite, "ui/menu_levelshots/unknownmap");

			// Draw the blurred frame
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
		}
		else {
			if (uiSaveGame.menu.items[uiSaveGame.menu.cursor] == self)
				UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
			else
				UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
		}
	}
}

/*
 =================
 UI_SaveGame_Init
 =================
*/
static void UI_SaveGame_Init (void){

	int		i, y;

	memset(&uiSaveGame, 0, sizeof(uiSaveGame_t));

	uiSaveGame.fadeTime = uiStatic.realTime;

	uiSaveGame.background.generic.id			= ID_BACKGROUND;
	uiSaveGame.background.generic.type			= QMTYPE_BITMAP;
	uiSaveGame.background.generic.flags			= QMF_INACTIVE;
	uiSaveGame.background.generic.x				= 0;
	uiSaveGame.background.generic.y				= 0;
	uiSaveGame.background.generic.width			= 1024;
	uiSaveGame.background.generic.height		= 768;
	uiSaveGame.background.pic					= ART_BACKGROUND;

	uiSaveGame.banner.generic.id				= ID_BANNER;
	uiSaveGame.banner.generic.type				= QMTYPE_BITMAP;
	uiSaveGame.banner.generic.flags				= QMF_INACTIVE;
	uiSaveGame.banner.generic.x					= 0;
	uiSaveGame.banner.generic.y					= 66;
	uiSaveGame.banner.generic.width				= 1024;
	uiSaveGame.banner.generic.height			= 46;
	uiSaveGame.banner.pic						= ART_BANNER;

	uiSaveGame.back.generic.id					= ID_BACK;
	uiSaveGame.back.generic.type				= QMTYPE_BITMAP;
	uiSaveGame.back.generic.x					= 310;
	uiSaveGame.back.generic.y					= 656;
	uiSaveGame.back.generic.width				= 198;
	uiSaveGame.back.generic.height				= 38;
	uiSaveGame.back.generic.callback			= UI_SaveGame_Callback;
	uiSaveGame.back.generic.ownerdraw			= UI_SaveGame_Ownerdraw;
	uiSaveGame.back.pic							= UI_BACKBUTTON;

	uiSaveGame.save.generic.id					= ID_SAVE;
	uiSaveGame.save.generic.type				= QMTYPE_BITMAP;
	uiSaveGame.save.generic.x					= 516;
	uiSaveGame.save.generic.y					= 656;
	uiSaveGame.save.generic.width				= 198;
	uiSaveGame.save.generic.height				= 38;
	uiSaveGame.save.generic.callback			= UI_SaveGame_Callback;
	uiSaveGame.save.generic.ownerdraw			= UI_SaveGame_Ownerdraw;
	uiSaveGame.save.pic							= UI_SAVEBUTTON;

	uiSaveGame.listBack.generic.id				= ID_LISTBACK;
	uiSaveGame.listBack.generic.type			= QMTYPE_BITMAP;
	uiSaveGame.listBack.generic.flags			= QMF_INACTIVE;
	uiSaveGame.listBack.generic.x				= 42;
	uiSaveGame.listBack.generic.y				= 146;
	uiSaveGame.listBack.generic.width			= 462;
	uiSaveGame.listBack.generic.height			= 476;
	uiSaveGame.listBack.pic						= ART_LISTBACK;

	uiSaveGame.gameTitle.generic.id				= ID_GAMETITLE;
	uiSaveGame.gameTitle.generic.type			= QMTYPE_ACTION;
	uiSaveGame.gameTitle.generic.flags			= QMF_INACTIVE;
	uiSaveGame.gameTitle.generic.x				= 42;
	uiSaveGame.gameTitle.generic.y				= 146;
	uiSaveGame.gameTitle.generic.width			= 462;
	uiSaveGame.gameTitle.generic.height			= 24;
	uiSaveGame.gameTitle.generic.ownerdraw		= UI_SaveGame_Ownerdraw;

	for (i = 0, y = 182; i < 14; i++, y += 32){
		uiSaveGame.listGames[i].generic.id			= ID_LISTGAMES+i;
		uiSaveGame.listGames[i].generic.type		= QMTYPE_ACTION;
		uiSaveGame.listGames[i].generic.flags		= QMF_SILENT;
		uiSaveGame.listGames[i].generic.x			= 42;
		uiSaveGame.listGames[i].generic.y			= y;
		uiSaveGame.listGames[i].generic.width		= 462;
		uiSaveGame.listGames[i].generic.height		= 24;
		uiSaveGame.listGames[i].generic.callback	= UI_SaveGame_Callback;
		uiSaveGame.listGames[i].generic.ownerdraw	= UI_SaveGame_Ownerdraw;
	}

	uiSaveGame.levelShot.generic.id				= ID_LEVELSHOT;
	uiSaveGame.levelShot.generic.type			= QMTYPE_BITMAP;
	uiSaveGame.levelShot.generic.flags			= QMF_INACTIVE;
	uiSaveGame.levelShot.generic.x				= 568;
	uiSaveGame.levelShot.generic.y				= 208;
	uiSaveGame.levelShot.generic.width			= 414;
	uiSaveGame.levelShot.generic.height			= 206;
	uiSaveGame.levelShot.generic.ownerdraw		= UI_SaveGame_Ownerdraw;
	uiSaveGame.levelShot.pic					= ART_LEVELSHOTBLUR;

	uiSaveGame.newGame.generic.id				= ID_NEWGAME;
	uiSaveGame.newGame.generic.type				= QMTYPE_BITMAP;
	uiSaveGame.newGame.generic.x				= 676;
	uiSaveGame.newGame.generic.y				= 468;
	uiSaveGame.newGame.generic.width			= 198;
	uiSaveGame.newGame.generic.height			= 38;
	uiSaveGame.newGame.generic.callback			= UI_SaveGame_Callback;
	uiSaveGame.newGame.generic.ownerdraw		= UI_SaveGame_Ownerdraw;
	uiSaveGame.newGame.pic						= UI_NEWGAMEBUTTON;

	uiSaveGame.loadGame.generic.id				= ID_LOADGAME;
	uiSaveGame.loadGame.generic.type			= QMTYPE_BITMAP;
	uiSaveGame.loadGame.generic.x				= 676;
	uiSaveGame.loadGame.generic.y				= 516;
	uiSaveGame.loadGame.generic.width			= 198;
	uiSaveGame.loadGame.generic.height			= 38;
	uiSaveGame.loadGame.generic.callback		= UI_SaveGame_Callback;
	uiSaveGame.loadGame.generic.ownerdraw		= UI_SaveGame_Ownerdraw;
	uiSaveGame.loadGame.pic						= UI_LOADBUTTON;

	uiSaveGame.deleteGame.generic.id			= ID_DELETEGAME;
	uiSaveGame.deleteGame.generic.type			= QMTYPE_BITMAP;
	uiSaveGame.deleteGame.generic.x				= 676;
	uiSaveGame.deleteGame.generic.y				= 564;
	uiSaveGame.deleteGame.generic.width			= 198;
	uiSaveGame.deleteGame.generic.height		= 38;
	uiSaveGame.deleteGame.generic.callback		= UI_SaveGame_Callback;
	uiSaveGame.deleteGame.generic.ownerdraw		= UI_SaveGame_Ownerdraw;
	uiSaveGame.deleteGame.pic					= UI_DELETEBUTTON;

	UI_SaveGame_GetGameList();

	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.background);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.banner);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.back);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.save);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.listBack);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.gameTitle);

	for (i = 0; i < 14; i++)
		UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.listGames[i]);

	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.levelShot);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.newGame);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.loadGame);
	UI_AddItem(&uiSaveGame.menu, (void *)&uiSaveGame.deleteGame);
}

/*
 =================
 UI_SaveGame_Precache
 =================
*/
void UI_SaveGame_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
	R_RegisterShaderNoMip(ART_LISTBACK);
	R_RegisterShaderNoMip(ART_LEVELSHOTBLUR);
}

/*
 =================
 UI_SaveGame_Menu
 =================
*/
void UI_SaveGame_Menu (void){

	UI_SaveGame_Precache();
	UI_SaveGame_Init();

	UI_PushMenu(&uiSaveGame.menu);
}
