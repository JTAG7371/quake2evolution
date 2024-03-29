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
#define ART_BANNER				"ui/banners/loadgame_t"
#define ART_LISTBACK			"ui/segments/files_box"
#define ART_LEVELSHOTBLUR		"ui/segments/sp_mapshot"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_LOAD				3

#define ID_LISTBACK			4
#define ID_GAMETITLE		5
#define ID_LISTGAMES		6
#define ID_LEVELSHOT		20

#define ID_NEWGAME			21
#define ID_SAVEGAME			22
#define ID_DELETEGAME		23

typedef struct {
	char					map[80];
	char					time[8];
	char					date[8];
	char					name[32];
	qboolean				valid;
} uiLoadGameGame_t;

static color_t				uiLoadGameColor = {0, 76, 127, 255};

typedef struct {
	uiLoadGameGame_t		games[14];
	int						currentGame;

	int						currentLevelShot;
	int						fadeTime;

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			back;
	menuBitmap_s			load;

	menuBitmap_s			listBack;
	menuAction_s			gameTitle;
	menuAction_s			listGames[14];	
	menuBitmap_s			levelShot;

	menuBitmap_s			newGame;
	menuBitmap_s			saveGame;
	menuBitmap_s			deleteGame;
} uiLoadGame_t;

static uiLoadGame_t			uiLoadGame;


/*
 =================
 UI_LoadGame_GetGameList
 =================
*/
static void UI_LoadGame_GetGameList (void){

	int		i;
	char	name[MAX_QPATH];
	char	*buffer;

	for (i = 0; i < 14; i++){
		Q_snprintfz(name, sizeof(name), "save/save%i/server.ssv", i);
		FS_LoadFile(name, (void **)&buffer);
		if (!buffer){
			Q_strncpyz(uiLoadGame.games[i].map, "", sizeof(uiLoadGame.games[i].map));
			Q_strncpyz(uiLoadGame.games[i].time, "", sizeof(uiLoadGame.games[i].time));
			Q_strncpyz(uiLoadGame.games[i].date, "", sizeof(uiLoadGame.games[i].date));
			Q_strncpyz(uiLoadGame.games[i].name, "<EMPTY>", sizeof(uiLoadGame.games[i].name));
			uiLoadGame.games[i].valid = false;

			continue;
		}

		if (strstr(buffer, "ENTERING")){
			memcpy(uiLoadGame.games[i].map, buffer+32, 80-32);
			Q_strncpyz(uiLoadGame.games[i].time, "", sizeof(uiLoadGame.games[i].time));
			Q_strncpyz(uiLoadGame.games[i].date, "", sizeof(uiLoadGame.games[i].date));
			memcpy(uiLoadGame.games[i].name, buffer, 32);
		}
		else {
			memcpy(uiLoadGame.games[i].map, buffer+32, 80-32);
			memcpy(uiLoadGame.games[i].time, buffer, 5);
			memcpy(uiLoadGame.games[i].date, buffer+6, 5);
			memcpy(uiLoadGame.games[i].name, buffer+13, 32-13);
		}
		uiLoadGame.games[i].valid = true;

		FS_FreeFile(buffer);
	}

	// Select first valid slot
	for (i = 0; i < 14; i++){
		if (uiLoadGame.games[i].valid){
			uiLoadGame.listGames[i].generic.color = uiLoadGameColor;
			uiLoadGame.currentGame = i;
			break;
		}
	}

	// Couldn't find a valid slot, so gray load button
	if (i == 14)
		uiLoadGame.load.generic.flags |= QMF_GRAYED;
}

/*
 =================
 UI_LoadGame_Callback
 =================
*/
static void UI_LoadGame_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event != QM_ACTIVATED)
		return;

	if (item->type == QMTYPE_ACTION){
		// Reset color, get current game, set color
		uiLoadGame.listGames[uiLoadGame.currentGame].generic.color = uiColorWhite;
		uiLoadGame.currentGame = item->id - ID_LISTGAMES;
		uiLoadGame.listGames[uiLoadGame.currentGame].generic.color = uiLoadGameColor;

		// Restart levelshot animation
		uiLoadGame.currentLevelShot = 0;
		uiLoadGame.fadeTime = uiStatic.realTime;
		return;
	}
	
	switch (item->id){
	case ID_BACK:
		if (cls.state == CA_ACTIVE)
			UI_InGame_Menu();
		else
			UI_Main_Menu();

		break;
	case ID_LOAD:
		if (uiLoadGame.games[uiLoadGame.currentGame].valid)
			Cbuf_ExecuteText(EXEC_APPEND, va("loadgame save%i\n", uiLoadGame.currentGame));

		break;
	case ID_NEWGAME:
		UI_SinglePlayer_Menu();
		break;
	case ID_SAVEGAME:
		UI_SaveGame_Menu();
		break;
	case ID_DELETEGAME:
		Cbuf_ExecuteText(EXEC_NOW, va("deletegame save%i\n", uiLoadGame.currentGame));
		UI_LoadGame_GetGameList();
		break;
	}
}

/*
 =================
 UI_LoadGame_Ownerdraw
 =================
*/
static void UI_LoadGame_Ownerdraw (void *self){

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
			time = uiLoadGame.games[item->id - ID_LISTGAMES].time;
			date = uiLoadGame.games[item->id - ID_LISTGAMES].date;
			name = uiLoadGame.games[item->id - ID_LISTGAMES].name;
			centered = !uiLoadGame.games[item->id - ID_LISTGAMES].valid;

			if (strstr(uiLoadGame.games[item->id - ID_LISTGAMES].name, "ENTERING"))
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

			if (uiLoadGame.games[uiLoadGame.currentGame].map[0]){
				char	pathTGA[MAX_QPATH], pathJPG[MAX_QPATH];

				if (uiStatic.realTime - uiLoadGame.fadeTime >= 3000){
					uiLoadGame.fadeTime = uiStatic.realTime;

					uiLoadGame.currentLevelShot++;
					if (uiLoadGame.currentLevelShot == 3)
						uiLoadGame.currentLevelShot = 0;
				}

				prev = uiLoadGame.currentLevelShot - 1;
				if (prev < 0)
					prev = 2;

				color[3] = Clamp((float)(uiStatic.realTime - uiLoadGame.fadeTime) / 1000, 0.0, 1.0) * 255;

				Q_snprintfz(pathTGA, sizeof(pathTGA), "ui/menu_levelshots/%s_1.tga", uiLoadGame.games[uiLoadGame.currentGame].map);
				Q_snprintfz(pathJPG, sizeof(pathJPG), "ui/menu_levelshots/%s_1.jpg", uiLoadGame.games[uiLoadGame.currentGame].map);

				if (FS_LoadFile(pathTGA, NULL) == -1 && FS_LoadFile(pathJPG, NULL) == -1)
					UI_DrawPic(x, y, w, h, uiColorWhite, "ui/menu_levelshots/unknownmap");
				else {
					UI_DrawPic(x, y, w, h, uiColorWhite, va("ui/menu_levelshots/%s_%i", uiLoadGame.games[uiLoadGame.currentGame].map, prev+1));
					UI_DrawPic(x, y, w, h, color, va("ui/menu_levelshots/%s_%i", uiLoadGame.games[uiLoadGame.currentGame].map, uiLoadGame.currentLevelShot+1));
				}
			}
			else
				UI_DrawPic(x, y, w, h, uiColorWhite, "ui/menu_levelshots/unknownmap");

			// Draw the blurred frame
			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
		}
		else {
			if (uiLoadGame.menu.items[uiLoadGame.menu.cursor] == self)
				UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
			else
				UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

			UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
		}
	}
}

/*
 =================
 UI_LoadGame_Init
 =================
*/
static void UI_LoadGame_Init (void){

	int		i, y;

	memset(&uiLoadGame, 0, sizeof(uiLoadGame_t));

	uiLoadGame.fadeTime = uiStatic.realTime;

	uiLoadGame.background.generic.id			= ID_BACKGROUND;
	uiLoadGame.background.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.background.generic.flags			= QMF_INACTIVE;
	uiLoadGame.background.generic.x				= 0;
	uiLoadGame.background.generic.y				= 0;
	uiLoadGame.background.generic.width			= 1024;
	uiLoadGame.background.generic.height		= 768;
	uiLoadGame.background.pic					= ART_BACKGROUND;

	uiLoadGame.banner.generic.id				= ID_BANNER;
	uiLoadGame.banner.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.banner.generic.flags				= QMF_INACTIVE;
	uiLoadGame.banner.generic.x					= 0;
	uiLoadGame.banner.generic.y					= 66;
	uiLoadGame.banner.generic.width				= 1024;
	uiLoadGame.banner.generic.height			= 46;
	uiLoadGame.banner.pic						= ART_BANNER;

	uiLoadGame.back.generic.id					= ID_BACK;
	uiLoadGame.back.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.back.generic.x					= 310;
	uiLoadGame.back.generic.y					= 656;
	uiLoadGame.back.generic.width				= 198;
	uiLoadGame.back.generic.height				= 38;
	uiLoadGame.back.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.back.generic.ownerdraw			= UI_LoadGame_Ownerdraw;
	uiLoadGame.back.pic							= UI_BACKBUTTON;

	uiLoadGame.load.generic.id					= ID_LOAD;
	uiLoadGame.load.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.load.generic.x					= 516;
	uiLoadGame.load.generic.y					= 656;
	uiLoadGame.load.generic.width				= 198;
	uiLoadGame.load.generic.height				= 38;
	uiLoadGame.load.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.load.generic.ownerdraw			= UI_LoadGame_Ownerdraw;
	uiLoadGame.load.pic							= UI_LOADBUTTON;

	uiLoadGame.listBack.generic.id				= ID_LISTBACK;
	uiLoadGame.listBack.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.listBack.generic.flags			= QMF_INACTIVE;
	uiLoadGame.listBack.generic.x				= 42;
	uiLoadGame.listBack.generic.y				= 146;
	uiLoadGame.listBack.generic.width			= 462;
	uiLoadGame.listBack.generic.height			= 476;
	uiLoadGame.listBack.pic						= ART_LISTBACK;

	uiLoadGame.gameTitle.generic.id				= ID_GAMETITLE;
	uiLoadGame.gameTitle.generic.type			= QMTYPE_ACTION;
	uiLoadGame.gameTitle.generic.flags			= QMF_INACTIVE;
	uiLoadGame.gameTitle.generic.x				= 42;
	uiLoadGame.gameTitle.generic.y				= 146;
	uiLoadGame.gameTitle.generic.width			= 462;
	uiLoadGame.gameTitle.generic.height			= 24;
	uiLoadGame.gameTitle.generic.ownerdraw		= UI_LoadGame_Ownerdraw;

	for (i = 0, y = 182; i < 14; i++, y += 32){
		uiLoadGame.listGames[i].generic.id			= ID_LISTGAMES+i;
		uiLoadGame.listGames[i].generic.type		= QMTYPE_ACTION;
		uiLoadGame.listGames[i].generic.flags		= QMF_SILENT;
		uiLoadGame.listGames[i].generic.x			= 42;
		uiLoadGame.listGames[i].generic.y			= y;
		uiLoadGame.listGames[i].generic.width		= 462;
		uiLoadGame.listGames[i].generic.height		= 24;
		uiLoadGame.listGames[i].generic.callback	= UI_LoadGame_Callback;
		uiLoadGame.listGames[i].generic.ownerdraw	= UI_LoadGame_Ownerdraw;
	}

	uiLoadGame.levelShot.generic.id				= ID_LEVELSHOT;
	uiLoadGame.levelShot.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.levelShot.generic.flags			= QMF_INACTIVE;
	uiLoadGame.levelShot.generic.x				= 568;
	uiLoadGame.levelShot.generic.y				= 208;
	uiLoadGame.levelShot.generic.width			= 414;
	uiLoadGame.levelShot.generic.height			= 206;
	uiLoadGame.levelShot.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.levelShot.pic					= ART_LEVELSHOTBLUR;

	uiLoadGame.newGame.generic.id				= ID_NEWGAME;
	uiLoadGame.newGame.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.newGame.generic.x				= 676;
	uiLoadGame.newGame.generic.y				= 468;
	uiLoadGame.newGame.generic.width			= 198;
	uiLoadGame.newGame.generic.height			= 38;
	uiLoadGame.newGame.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.newGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.newGame.pic						= UI_NEWGAMEBUTTON;

	uiLoadGame.saveGame.generic.id				= ID_SAVEGAME;
	uiLoadGame.saveGame.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.saveGame.generic.x				= 676;
	uiLoadGame.saveGame.generic.y				= 516;
	uiLoadGame.saveGame.generic.width			= 198;
	uiLoadGame.saveGame.generic.height			= 38;
	uiLoadGame.saveGame.generic.callback		= UI_LoadGame_Callback;
	uiLoadGame.saveGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.saveGame.pic						= UI_SAVEBUTTON;

	uiLoadGame.deleteGame.generic.id			= ID_DELETEGAME;
	uiLoadGame.deleteGame.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.deleteGame.generic.x				= 676;
	uiLoadGame.deleteGame.generic.y				= 564;
	uiLoadGame.deleteGame.generic.width			= 198;
	uiLoadGame.deleteGame.generic.height		= 38;
	uiLoadGame.deleteGame.generic.callback		= UI_LoadGame_Callback;
	uiLoadGame.deleteGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.deleteGame.pic					= UI_DELETEBUTTON;

	UI_LoadGame_GetGameList();

	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.background);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.banner);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.back);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.load);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.listBack);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.gameTitle);

	for (i = 0; i < 14; i++)
		UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.listGames[i]);

	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.levelShot);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.newGame);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.saveGame);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.deleteGame);
}

/*
 =================
 UI_LoadGame_Precache
 =================
*/
void UI_LoadGame_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
	R_RegisterShaderNoMip(ART_LISTBACK);
	R_RegisterShaderNoMip(ART_LEVELSHOTBLUR);
}

/*
 =================
 UI_LoadGame_Menu
 =================
*/
void UI_LoadGame_Menu (void){

	UI_LoadGame_Precache();
	UI_LoadGame_Init();

	UI_PushMenu(&uiLoadGame.menu);
}
