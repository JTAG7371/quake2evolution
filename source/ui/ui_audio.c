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


#define ART_BACKGROUND			"ui/misc/ui_sub_options"
#define ART_BANNER				"ui/banners/audio_t"
#define ART_TEXT1				"ui/text/audio_text_p1"
#define ART_TEXT2				"ui/text/audio_text_p2"

#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_TEXT1			2
#define ID_TEXT2			3
#define ID_TEXTSHADOW1		4
#define ID_TEXTSHADOW2		5

#define ID_CANCEL			6
#define ID_APPLY			7

#define ID_CDMUSIC			8
#define ID_CDVOLUME			9
#define ID_AUDIODEVICE		10
#define ID_MASTERVOLUME		11
#define ID_SFXVOLUME		12
#define ID_MUSICVOLUME		13
#define ID_ROLLOFFFACTOR	14
#define ID_DOPPLERFACTOR	15
#define ID_EAX				16

#define MAX_AUDIODEVICES	16

static const char	*uiAudioYesNo[] = {
	"False",
	"True"
};

typedef struct {
	float	cdMusic;
	float	cdVolume;
	float	audioDevice;
	float	masterVolume;
	float	sfxVolume;
	float	musicVolume;
	float	rolloffFactor;
	float	dopplerFactor;
	float	eax;
} uiAudioValues_t;

static uiAudioValues_t		uiAudioInitial;

typedef struct {
	char					*audioDeviceList[MAX_AUDIODEVICES];
	int						numAudioDevices;

	menuFramework_s			menu;

	menuBitmap_s			background;
	menuBitmap_s			banner;

	menuBitmap_s			textShadow1;
	menuBitmap_s			textShadow2;
	menuBitmap_s			text1;
	menuBitmap_s			text2;

	menuBitmap_s			cancel;
	menuBitmap_s			apply;

	menuSpinControl_s		cdMusic;
	menuSpinControl_s		cdVolume;
	menuSpinControl_s		audioDevice;
	menuSpinControl_s		masterVolume;
	menuSpinControl_s		sfxVolume;
	menuSpinControl_s		musicVolume;
	menuSpinControl_s		rolloffFactor;
	menuSpinControl_s		dopplerFactor;
	menuSpinControl_s		eax;
} uiAudio_t;

static uiAudio_t			uiAudio;


/*
 =================
 UI_Audio_GetDeviceList
 =================
*/
static void UI_Audio_GetDeviceList (void){

	char	*ptr;

	if (!uiStatic.alConfig.deviceList || !uiStatic.alConfig.deviceName){
		uiAudio.audioDeviceList[0] = "DirectSound3D";
		uiAudio.audioDeviceList[1] = "DirectSound";
		uiAudio.audioDeviceList[2] = "MMSYSTEM";

		uiAudio.numAudioDevices = 3;

		uiAudio.audioDevice.maxValue = uiAudio.numAudioDevices - 1;
		uiAudio.audioDevice.curValue = uiAudio.numAudioDevices - 1;

		return;
	}

	ptr = (char *)uiStatic.alConfig.deviceList;
	while (*ptr){
		uiAudio.audioDeviceList[uiAudio.numAudioDevices++] = ptr;

		if (!Q_stricmp(uiStatic.alConfig.deviceName, ptr))
			uiAudio.audioDevice.curValue = uiAudio.numAudioDevices - 1;

		ptr += strlen(ptr) + 1;
	}

	uiAudio.audioDevice.maxValue = uiAudio.numAudioDevices - 1;
}

/*
 =================
 UI_Audio_GetConfig
 =================
*/
static void UI_Audio_GetConfig (void){

	UI_Audio_GetDeviceList();

	if (!Cvar_VariableInteger("cd_noCD"))
		uiAudio.cdMusic.curValue = 1;

	uiAudio.cdVolume.curValue = Cvar_VariableValue("cd_volume");

	uiAudio.masterVolume.curValue = Cvar_VariableValue("s_masterVolume");

	uiAudio.sfxVolume.curValue = Cvar_VariableValue("s_sfxVolume");

	uiAudio.musicVolume.curValue = Cvar_VariableValue("s_musicVolume");

	uiAudio.rolloffFactor.curValue = Cvar_VariableValue("s_rolloffFactor");

	uiAudio.dopplerFactor.curValue = Cvar_VariableValue("s_dopplerFactor");

	if (Cvar_VariableInteger("s_ext_eax"))
		uiAudio.eax.curValue = 1;

	// Save initial values
	uiAudioInitial.cdMusic = uiAudio.cdMusic.curValue;
	uiAudioInitial.cdVolume = uiAudio.cdVolume.curValue;
	uiAudioInitial.audioDevice = uiAudio.audioDevice.curValue;
	uiAudioInitial.masterVolume = uiAudio.masterVolume.curValue;
	uiAudioInitial.sfxVolume = uiAudio.sfxVolume.curValue;
	uiAudioInitial.musicVolume = uiAudio.musicVolume.curValue;
	uiAudioInitial.rolloffFactor = uiAudio.rolloffFactor.curValue;
	uiAudioInitial.dopplerFactor = uiAudio.dopplerFactor.curValue;
	uiAudioInitial.eax = uiAudio.eax.curValue;
}

/*
 =================
 UI_Audio_SetConfig
 =================
*/
static void UI_Audio_SetConfig (void){

	Cvar_SetInteger("cd_noCD", !((int)uiAudio.cdMusic.curValue));

	Cvar_SetValue("cd_volume", uiAudio.cdVolume.curValue);

	Cvar_Set("s_alDevice", uiAudio.audioDeviceList[(int)uiAudio.audioDevice.curValue]);

	Cvar_SetValue("s_masterVolume", uiAudio.masterVolume.curValue);

	Cvar_SetValue("s_sfxVolume", uiAudio.sfxVolume.curValue);

	Cvar_SetValue("s_musicVolume", uiAudio.musicVolume.curValue);

	Cvar_SetValue("s_rolloffFactor", uiAudio.rolloffFactor.curValue);

	Cvar_SetValue("s_dopplerFactor", uiAudio.dopplerFactor.curValue);

	Cvar_SetInteger("s_ext_eax", (int)uiAudio.eax.curValue);

	// Restart sound subsystem
	Cbuf_ExecuteText(EXEC_NOW, "snd_restart\n");
}

/*
 =================
 UI_Audio_UpdateConfig
 =================
*/
static void UI_Audio_UpdateConfig (void){

	static char	cdVolumeText[8];
	static char	masterVolumeText[8];
	static char	sfxVolumeText[8];
	static char musicVolumeText[8];
	static char	rolloffFactorText[8];
	static char	dopplerFactorText[8];

	uiAudio.cdMusic.generic.name = uiAudioYesNo[(int)uiAudio.cdMusic.curValue];

	Q_snprintfz(cdVolumeText, sizeof(cdVolumeText), "%.1f", uiAudio.cdVolume.curValue);
	uiAudio.cdVolume.generic.name = cdVolumeText;

	uiAudio.audioDevice.generic.name = uiAudio.audioDeviceList[(int)uiAudio.audioDevice.curValue];

	Q_snprintfz(masterVolumeText, sizeof(masterVolumeText), "%.1f", uiAudio.masterVolume.curValue);
	uiAudio.masterVolume.generic.name = masterVolumeText;

	Q_snprintfz(sfxVolumeText, sizeof(sfxVolumeText), "%.1f", uiAudio.sfxVolume.curValue);
	uiAudio.sfxVolume.generic.name = sfxVolumeText;

	Q_snprintfz(musicVolumeText, sizeof(musicVolumeText), "%.1f", uiAudio.musicVolume.curValue);
	uiAudio.musicVolume.generic.name = musicVolumeText;

	Q_snprintfz(rolloffFactorText, sizeof(rolloffFactorText), "%.1f", uiAudio.rolloffFactor.curValue);
	uiAudio.rolloffFactor.generic.name = rolloffFactorText;

	Q_snprintfz(dopplerFactorText, sizeof(dopplerFactorText), "%.1f", uiAudio.dopplerFactor.curValue);
	uiAudio.dopplerFactor.generic.name = dopplerFactorText;

	uiAudio.eax.generic.name = uiAudioYesNo[(int)uiAudio.eax.curValue];

	// Some settings can be updated here
	Cvar_SetInteger("cd_noCD", !((int)uiAudio.cdMusic.curValue));

	Cvar_SetValue("cd_volume", uiAudio.cdVolume.curValue);

	Cvar_SetValue("s_masterVolume", uiAudio.masterVolume.curValue);

	Cvar_SetValue("s_sfxVolume", uiAudio.sfxVolume.curValue);

	Cvar_SetValue("s_musicVolume", uiAudio.musicVolume.curValue);

	Cvar_SetValue("s_rolloffFactor", uiAudio.rolloffFactor.curValue);

	Cvar_SetValue("s_dopplerFactor", uiAudio.dopplerFactor.curValue);

	// See if the apply button should be enabled or disabled
	uiAudio.apply.generic.flags |= QMF_GRAYED;

	if (uiAudioInitial.audioDevice != uiAudio.audioDevice.curValue){
		uiAudio.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}

	if (uiAudioInitial.eax != uiAudio.eax.curValue){
		uiAudio.apply.generic.flags &= ~QMF_GRAYED;
		return;
	}
}

/*
 =================
 UI_Audio_Callback
 =================
*/
static void UI_Audio_Callback (void *self, int event){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (event == QM_CHANGED){
		UI_Audio_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_Audio_SetConfig();
		UI_Audio_GetConfig();
		UI_Audio_UpdateConfig();
		break;
	}
}

/*
 =================
 UI_Audio_Ownerdraw
 =================
*/
static void UI_Audio_Ownerdraw (void *self){

	menuCommon_s	*item = (menuCommon_s *)self;

	if (uiAudio.menu.items[uiAudio.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, UI_MOVEBOX);

	if (item->flags & QMF_GRAYED)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorDkGrey, ((menuBitmap_s *)self)->pic);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorWhite, ((menuBitmap_s *)self)->pic);
}

/*
 =================
 UI_Audio_Init
 =================
*/
static void UI_Audio_Init (void){

	memset(&uiAudio, 0, sizeof(uiAudio_t));

	uiAudio.background.generic.id				= ID_BACKGROUND;
	uiAudio.background.generic.type				= QMTYPE_BITMAP;
	uiAudio.background.generic.flags			= QMF_INACTIVE;
	uiAudio.background.generic.x				= 0;
	uiAudio.background.generic.y				= 0;
	uiAudio.background.generic.width			= 1024;
	uiAudio.background.generic.height			= 768;
	uiAudio.background.pic						= ART_BACKGROUND;

	uiAudio.banner.generic.id					= ID_BANNER;
	uiAudio.banner.generic.type					= QMTYPE_BITMAP;
	uiAudio.banner.generic.flags				= QMF_INACTIVE;
	uiAudio.banner.generic.x					= 0;
	uiAudio.banner.generic.y					= 66;
	uiAudio.banner.generic.width				= 1024;
	uiAudio.banner.generic.height				= 46;
	uiAudio.banner.pic							= ART_BANNER;

	uiAudio.textShadow1.generic.id				= ID_TEXTSHADOW1;
	uiAudio.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiAudio.textShadow1.generic.flags			= QMF_INACTIVE;
	uiAudio.textShadow1.generic.x				= 182;
	uiAudio.textShadow1.generic.y				= 170;
	uiAudio.textShadow1.generic.width			= 256;
	uiAudio.textShadow1.generic.height			= 256;
	uiAudio.textShadow1.generic.color			= uiColorBlack;
	uiAudio.textShadow1.pic						= ART_TEXT1;

	uiAudio.textShadow2.generic.id				= ID_TEXTSHADOW2;
	uiAudio.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiAudio.textShadow2.generic.flags			= QMF_INACTIVE;
	uiAudio.textShadow2.generic.x				= 182;
	uiAudio.textShadow2.generic.y				= 426;
	uiAudio.textShadow2.generic.width			= 256;
	uiAudio.textShadow2.generic.height			= 256;
	uiAudio.textShadow2.generic.color			= uiColorBlack;
	uiAudio.textShadow2.pic						= ART_TEXT2;

	uiAudio.text1.generic.id					= ID_TEXT1;
	uiAudio.text1.generic.type					= QMTYPE_BITMAP;
	uiAudio.text1.generic.flags					= QMF_INACTIVE;
	uiAudio.text1.generic.x						= 180;
	uiAudio.text1.generic.y						= 168;
	uiAudio.text1.generic.width					= 256;
	uiAudio.text1.generic.height				= 256;
	uiAudio.text1.pic							= ART_TEXT1;

	uiAudio.text2.generic.id					= ID_TEXT2;
	uiAudio.text2.generic.type					= QMTYPE_BITMAP;
	uiAudio.text2.generic.flags					= QMF_INACTIVE;
	uiAudio.text2.generic.x						= 180;
	uiAudio.text2.generic.y						= 424;
	uiAudio.text2.generic.width					= 256;
	uiAudio.text2.generic.height				= 256;
	uiAudio.text2.pic							= ART_TEXT2;

	uiAudio.cancel.generic.id					= ID_CANCEL;
	uiAudio.cancel.generic.type					= QMTYPE_BITMAP;
	uiAudio.cancel.generic.x					= 310;
	uiAudio.cancel.generic.y					= 656;
	uiAudio.cancel.generic.width				= 198;
	uiAudio.cancel.generic.height				= 38;
	uiAudio.cancel.generic.callback				= UI_Audio_Callback;
	uiAudio.cancel.generic.ownerdraw			= UI_Audio_Ownerdraw;
	uiAudio.cancel.pic							= UI_CANCELBUTTON;

	uiAudio.apply.generic.id					= ID_APPLY;
	uiAudio.apply.generic.type					= QMTYPE_BITMAP;
	uiAudio.apply.generic.x						= 516;
	uiAudio.apply.generic.y						= 656;
	uiAudio.apply.generic.width					= 198;
	uiAudio.apply.generic.height				= 38;
	uiAudio.apply.generic.callback				= UI_Audio_Callback;
	uiAudio.apply.generic.ownerdraw				= UI_Audio_Ownerdraw;
	uiAudio.apply.pic							= UI_APPLYBUTTON;

	uiAudio.cdMusic.generic.id					= ID_CDMUSIC;
	uiAudio.cdMusic.generic.type				= QMTYPE_SPINCONTROL;
	uiAudio.cdMusic.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.cdMusic.generic.x					= 580;
	uiAudio.cdMusic.generic.y					= 160;
	uiAudio.cdMusic.generic.width				= 198;
	uiAudio.cdMusic.generic.height				= 30;
	uiAudio.cdMusic.generic.callback			= UI_Audio_Callback;
	uiAudio.cdMusic.generic.statusText			= "Enable/Disable CD Audio music";
	uiAudio.cdMusic.minValue					= 0;
	uiAudio.cdMusic.maxValue					= 1;
	uiAudio.cdMusic.range						= 1;

	uiAudio.cdVolume.generic.id					= ID_CDVOLUME;
	uiAudio.cdVolume.generic.type				= QMTYPE_SPINCONTROL;
	uiAudio.cdVolume.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.cdVolume.generic.x					= 580;
	uiAudio.cdVolume.generic.y					= 192;
	uiAudio.cdVolume.generic.width				= 198;
	uiAudio.cdVolume.generic.height				= 30;
	uiAudio.cdVolume.generic.callback			= UI_Audio_Callback;
	uiAudio.cdVolume.generic.statusText			= "Set CD Audio volume level";
	uiAudio.cdVolume.minValue					= 0.0;
	uiAudio.cdVolume.maxValue					= 1.0;
	uiAudio.cdVolume.range						= 0.1;

	uiAudio.audioDevice.generic.id				= ID_AUDIODEVICE;
	uiAudio.audioDevice.generic.type			= QMTYPE_SPINCONTROL;
	uiAudio.audioDevice.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.audioDevice.generic.x				= 580;
	uiAudio.audioDevice.generic.y				= 256;
	uiAudio.audioDevice.generic.width			= 198;
	uiAudio.audioDevice.generic.height			= 30;
	uiAudio.audioDevice.generic.callback		= UI_Audio_Callback;
	uiAudio.audioDevice.generic.statusText		= "Select audio output device";
	uiAudio.audioDevice.minValue				= 0;
	uiAudio.audioDevice.maxValue				= 0;
	uiAudio.audioDevice.range					= 1;

	uiAudio.masterVolume.generic.id				= ID_MASTERVOLUME;
	uiAudio.masterVolume.generic.type			= QMTYPE_SPINCONTROL;
	uiAudio.masterVolume.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.masterVolume.generic.x				= 580;
	uiAudio.masterVolume.generic.y				= 288;
	uiAudio.masterVolume.generic.width			= 198;
	uiAudio.masterVolume.generic.height			= 30;
	uiAudio.masterVolume.generic.callback		= UI_Audio_Callback;
	uiAudio.masterVolume.generic.statusText		= "Set master volume level";
	uiAudio.masterVolume.minValue				= 0.0;
	uiAudio.masterVolume.maxValue				= 1.0;
	uiAudio.masterVolume.range					= 0.1;

	uiAudio.sfxVolume.generic.id				= ID_SFXVOLUME;
	uiAudio.sfxVolume.generic.type				= QMTYPE_SPINCONTROL;
	uiAudio.sfxVolume.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.sfxVolume.generic.x					= 580;
	uiAudio.sfxVolume.generic.y					= 320;
	uiAudio.sfxVolume.generic.width				= 198;
	uiAudio.sfxVolume.generic.height			= 30;
	uiAudio.sfxVolume.generic.callback			= UI_Audio_Callback;
	uiAudio.sfxVolume.generic.statusText		= "Set sound effects volume level";
	uiAudio.sfxVolume.minValue					= 0.0;
	uiAudio.sfxVolume.maxValue					= 1.0;
	uiAudio.sfxVolume.range						= 0.1;

	uiAudio.musicVolume.generic.id				= ID_MUSICVOLUME;
	uiAudio.musicVolume.generic.type			= QMTYPE_SPINCONTROL;
	uiAudio.musicVolume.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.musicVolume.generic.x				= 580;
	uiAudio.musicVolume.generic.y				= 352;
	uiAudio.musicVolume.generic.width			= 198;
	uiAudio.musicVolume.generic.height			= 30;
	uiAudio.musicVolume.generic.callback		= UI_Audio_Callback;
	uiAudio.musicVolume.generic.statusText		= "Set background music volume level";
	uiAudio.musicVolume.minValue				= 0.0;
	uiAudio.musicVolume.maxValue				= 1.0;
	uiAudio.musicVolume.range					= 0.1;

	uiAudio.rolloffFactor.generic.id			= ID_ROLLOFFFACTOR;
	uiAudio.rolloffFactor.generic.type			= QMTYPE_SPINCONTROL;
	uiAudio.rolloffFactor.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.rolloffFactor.generic.x				= 580;
	uiAudio.rolloffFactor.generic.y				= 384;
	uiAudio.rolloffFactor.generic.width			= 198;
	uiAudio.rolloffFactor.generic.height		= 30;
	uiAudio.rolloffFactor.generic.callback		= UI_Audio_Callback;
	uiAudio.rolloffFactor.generic.statusText	= "Set sound rolloff factor";
	uiAudio.rolloffFactor.minValue				= 0.0;
	uiAudio.rolloffFactor.maxValue				= 10.0;
	uiAudio.rolloffFactor.range					= 1.0;

	uiAudio.dopplerFactor.generic.id			= ID_DOPPLERFACTOR;
	uiAudio.dopplerFactor.generic.type			= QMTYPE_SPINCONTROL;
	uiAudio.dopplerFactor.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.dopplerFactor.generic.x				= 580;
	uiAudio.dopplerFactor.generic.y				= 416;
	uiAudio.dopplerFactor.generic.width			= 198;
	uiAudio.dopplerFactor.generic.height		= 30;
	uiAudio.dopplerFactor.generic.callback		= UI_Audio_Callback;
	uiAudio.dopplerFactor.generic.statusText	= "Set sound doppler factor";
	uiAudio.dopplerFactor.minValue				= 0.0;
	uiAudio.dopplerFactor.maxValue				= 10.0;
	uiAudio.dopplerFactor.range					= 1.0;

	uiAudio.eax.generic.id						= ID_EAX;
	uiAudio.eax.generic.type					= QMTYPE_SPINCONTROL;
	uiAudio.eax.generic.flags					= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiAudio.eax.generic.x						= 580;
	uiAudio.eax.generic.y						= 448;
	uiAudio.eax.generic.width					= 198;
	uiAudio.eax.generic.height					= 30;
	uiAudio.eax.generic.callback				= UI_Audio_Callback;
	uiAudio.eax.generic.statusText				= "Enable/Disable environmental audio extensions";
	uiAudio.eax.minValue						= 0;
	uiAudio.eax.maxValue						= 1;
	uiAudio.eax.range							= 1;

	UI_Audio_GetConfig();

	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.background);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.banner);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.textShadow1);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.textShadow2);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.text1);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.text2);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.cancel);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.apply);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.cdMusic);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.cdVolume);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.audioDevice);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.masterVolume);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.sfxVolume);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.musicVolume);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.rolloffFactor);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.dopplerFactor);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.eax);
}

/*
 =================
 UI_Audio_Precache
 =================
*/
void UI_Audio_Precache (void){

	R_RegisterShaderNoMip(ART_BACKGROUND);
	R_RegisterShaderNoMip(ART_BANNER);
	R_RegisterShaderNoMip(ART_TEXT1);
	R_RegisterShaderNoMip(ART_TEXT2);
}

/*
 =================
 UI_Audio_Menu
 =================
*/
void UI_Audio_Menu (void){

	UI_Audio_Precache();
	UI_Audio_Init();

	UI_Audio_UpdateConfig();

	UI_PushMenu(&uiAudio.menu);
}
