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


#ifndef __S_LOCAL_H__
#define __S_LOCAL_H__


#ifdef _WIN32
#include "../win32/winquake.h"
#endif

#include "AL/al.h"
#include "AL/alc.h"

#include "cl_local.h"

#include "include/eax.h"
#include "include/vorbisfile.h"

#include "qal.h"

#ifdef _WIN32
#include "../win32/alw_win.h"
#endif


typedef struct {
	int					rate;
	int					width;
	int					channels;
	int					samples;
} wavInfo_t;

typedef struct sfx_s {
	char				name[MAX_QPATH];
	qboolean			defaulted;
	qboolean			loaded;

	int					samples;
	int					rate;
	unsigned			format;
	unsigned			bufferNum;

	struct sfx_s		*nextHash;
} sfx_t;

typedef struct {
	char				introName[MAX_QPATH];
	char				loopName[MAX_QPATH];
	qboolean			looping;
	fileHandle_t		file;
	int					start;
	int					rate;
	unsigned			format;
	void				*vorbisFile;
} bgTrack_t;

// A playSound will be generated by each call to S_StartSound.
// When the mixer reaches playSound->beginTime, the playSound will be
// assigned to a channel.
typedef struct playSound_s {
	struct playSound_s	*prev, *next;
	sfx_t				*sfx;
	int					entNum;
	int					entChannel;
	qboolean			fixedPosition;	// Use position instead of fetching entity's origin
	vec3_t				position;		// Only use if fixedPosition is set
	float				volume;
	float				attenuation;
	int					beginTime;		// Begin at this time
} playSound_t;

typedef struct {
	qboolean			streaming;
	sfx_t				*sfx;			// NULL if unused
	int					entNum;			// To allow overriding a specific sound
	int					entChannel;
	int					startTime;		// For overriding oldest sounds
	qboolean			loopSound;		// Looping sound
	int					loopNum;		// Looping entity number
	int					loopFrame;		// For stopping looping sounds
	qboolean			fixedPosition;	// Use position instead of fetching entity's origin
	vec3_t				position;		// Only use if fixedPosition is set
	float				volume;
	float				distanceMult;
	unsigned			sourceNum;		// OpenAL source
} channel_t;

typedef struct {
	vec3_t				position;
	vec3_t				velocity;
	float				orientation[6];
} listener_t;

extern qboolean			s_initialized;

extern cvar_t	*s_initSound;
extern cvar_t	*s_show;
extern cvar_t	*s_alDriver;
extern cvar_t	*s_alDevice;
extern cvar_t	*s_allowExtensions;
extern cvar_t	*s_ext_eax;
extern cvar_t	*s_ignoreALErrors;
extern cvar_t	*s_masterVolume;
extern cvar_t	*s_sfxVolume;
extern cvar_t	*s_musicVolume;
extern cvar_t	*s_minDistance;
extern cvar_t	*s_maxDistance;
extern cvar_t	*s_rolloffFactor;
extern cvar_t	*s_dopplerFactor;
extern cvar_t	*s_dopplerVelocity;

void		S_SoundList_f (void);
qboolean	S_LoadSound (sfx_t *sfx);
sfx_t		*S_FindSound (const char *name);

void		S_StreamBackgroundTrack (void);

channel_t	*S_PickChannel (int entNum, int entChannel);

/*
 =======================================================================

 IMPLEMENTATION SPECIFIC FUNCTIONS

 =======================================================================
*/

extern alConfig_t		alConfig;

#ifdef _WIN32

#define AL_DRIVER_OPENAL	"OpenAL32"

#define ALimp_Init						ALW_Init
#define ALimp_Shutdown					ALW_Shutdown

#else

#error "ALimp_* not available for this platform"

#endif


#endif	// __S_LOCAL_H__
