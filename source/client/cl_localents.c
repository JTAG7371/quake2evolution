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


// cl_localents.c -- client side local entities


#include "cl_local.h"


#define MAX_LOCAL_ENTITIES		512

typedef enum {
	LE_FADE,
	LE_SCALE,
	LE_SCALE_FADE,
	LE_MOVE_FADE,
	LE_MOVE_SCALE,
	LE_MOVE_SCALE_FADE,
	LE_ENTITY,
	LE_EJECT_BRASS,
	LE_BLOOD,
	LE_WATER_WAKE,
	LE_NUKE_SHOCKWAVE,
} leType_t;

typedef enum {
	LE_STATIONARY				= 1,
	LE_INWATER					= 2,
	LE_BOUNCESOUND				= 4,
	LE_LEAVEMARK				= 8,
	LE_INSTANT					= 16
} leFlags_t;

typedef struct localEntity_s {
	struct			localEntity_s *prev, *next;
	leType_t		leType;
	leFlags_t		leFlags;

	int				startTime;
	int				endTime;

	vec3_t			origin;
	vec3_t			velocity;
	float			gravity;
	float			radius;
	color_t			color;

	float			bounceFactor;
	struct sfx_s	*bounceSound;

	struct shader_s	*markShader;
	struct shader_s	*remapShader;

	float			light;
	vec3_t			lightColor;

	entity_t		entity;
} localEntity_t;

static localEntity_t	cl_activeLocalEntities;
static localEntity_t	*cl_freeLocalEntities;
static localEntity_t	cl_localEntities[MAX_LOCAL_ENTITIES];

static byte				cl_colorPalette[256][3];


/*
 =================
 CL_FreeLocalEntitity
 =================
*/
static void CL_FreeLocalEntity (localEntity_t *le){

	if (!le->prev)
		return;

	le->prev->next = le->next;
	le->next->prev = le->prev;

	le->next = cl_freeLocalEntities;
	cl_freeLocalEntities = le;
}

/*
 =================
 CL_AllocLocalEntity

 Will always succeed, even if it requires freeing an old active entity
 =================
*/
static localEntity_t *CL_AllocLocalEntity (void){

	localEntity_t	*le;

	if (!cl_freeLocalEntities)
		CL_FreeLocalEntity(cl_activeLocalEntities.prev);

	le = cl_freeLocalEntities;
	cl_freeLocalEntities = cl_freeLocalEntities->next;

	memset(le, 0, sizeof(localEntity_t));

	le->next = cl_activeLocalEntities.next;
	le->prev = &cl_activeLocalEntities;
	cl_activeLocalEntities.next->prev = le;
	cl_activeLocalEntities.next = le;

	return le;
}

/*
 =================
 CL_AddFade
 =================
*/
static void CL_AddFade (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.shaderRGBA[0] = le->color[0] * time;
	le->entity.shaderRGBA[1] = le->color[1] * time;
	le->entity.shaderRGBA[2] = le->color[2] * time;
	le->entity.shaderRGBA[3] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddScale
 =================
*/
static void CL_AddScale (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddScaleFade
 =================
*/
static void CL_AddScaleFade (localEntity_t *le){

	float	time, light;

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	le->entity.shaderRGBA[0] = le->color[0] * time;
	le->entity.shaderRGBA[1] = le->color[1] * time;
	le->entity.shaderRGBA[2] = le->color[2] * time;
	le->entity.shaderRGBA[3] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddMoveFade
 =================
*/
static void CL_AddMoveFade (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0);

	time = (cl.time - le->startTime) * 0.001;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.shaderRGBA[0] = le->color[0] * time;
	le->entity.shaderRGBA[1] = le->color[1] * time;
	le->entity.shaderRGBA[2] = le->color[2] * time;
	le->entity.shaderRGBA[3] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddMoveScale
 =================
*/
static void CL_AddMoveScale (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0);

	time = (cl.time - le->startTime) * 0.001;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddMoveScaleFade
 =================
*/
static void CL_AddMoveScaleFade (localEntity_t *le){

	float	time, gravity, light;

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0);

	time = (cl.time - le->startTime) * 0.001;

	le->entity.origin[0] = le->origin[0] + le->velocity[0] * time;
	le->entity.origin[1] = le->origin[1] + le->velocity[1] * time;
	le->entity.origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	le->entity.shaderRGBA[0] = le->color[0] * time;
	le->entity.shaderRGBA[1] = le->color[1] * time;
	le->entity.shaderRGBA[2] = le->color[2] * time;
	le->entity.shaderRGBA[3] = le->color[3] * time;

	R_AddEntityToScene(&le->entity);

	if (le->light){
		light = (float)(cl.time - le->startTime) / (le->endTime - le->startTime);
		light = le->light * (1.0 - light);

		R_AddLightToScene(le->entity.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2]);
	}
}

/*
 =================
 CL_AddEntity
 =================
*/
static void CL_AddEntity (localEntity_t *le){

	R_AddEntityToScene(&le->entity);

	if (le->leFlags & LE_INSTANT)
		CL_FreeLocalEntity(le);
}

/*
 =================
 CL_AddEjectBrass
 =================
*/
static void CL_AddEjectBrass (localEntity_t *le){

	float		time, gravity;
	vec3_t		origin, velocity;
	int			contents;
	vec3_t		mins, maxs;
	trace_t		trace;

	if (cl_brassTime->integer <= 0){
		CL_FreeLocalEntity(le);
		return;
	}

	if (le->leFlags & LE_STATIONARY){
		// Entity is stationary
		R_AddEntityToScene(&le->entity);
		return;
	}

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0);

	// Calculate origin
	time = (cl.time - le->startTime) * 0.001;
	
	origin[0] = le->origin[0] + le->velocity[0] * time;
	origin[1] = le->origin[1] + le->velocity[1] * time;
	origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	// Trace a line from previous origin to new origin
	R_ModelBounds(le->entity.model, mins, maxs);

	trace = CL_Trace(le->entity.origin, mins, maxs, origin, cl.clientNum, MASK_SOLID, true, NULL);
	if (trace.fraction != 0.0 && trace.fraction != 1.0){
		// Reflect velocity
		time = cl.time - (cls.frameTime + cls.frameTime * trace.fraction) * 1000;
		time = (time - le->startTime) * 0.001;

		VectorSet(velocity, le->velocity[0], le->velocity[1], le->velocity[2] + gravity * time);
		VectorReflect(velocity, trace.plane.normal, le->velocity);
		VectorScale(le->velocity, le->bounceFactor, le->velocity);

		// Check for stop
		if (trace.plane.normal[2] > 0 && le->velocity[2] < 1)
			le->leFlags |= LE_STATIONARY;

		// Reset
		le->startTime = cl.time;
		VectorCopy(trace.endpos, le->origin);

		// Play a bounce sound
		if (le->leFlags & LE_BOUNCESOUND){
			S_StartSound(trace.endpos, 0, CHAN_AUTO, le->bounceSound, 1, ATTN_NORM, 0);

			// Only play it once, otherwise it gets too noisy
			le->leFlags &= ~LE_BOUNCESOUND;
		}

		VectorCopy(trace.endpos, le->entity.origin);
		VectorCopy(trace.endpos, le->entity.oldOrigin);

		R_AddEntityToScene(&le->entity);
		return;
	}

	if (!(le->leFlags & LE_INWATER)){
		// If just entered a water volume, add friction
		contents = CL_PointContents(origin, -1);
		if (contents & MASK_WATER){
			if (contents & CONTENTS_WATER){
				VectorScale(le->velocity, 0.25, le->velocity);
				le->gravity *= 0.25;
			}
			if (contents & CONTENTS_SLIME){
				VectorScale(le->velocity, 0.20, le->velocity);
				le->gravity *= 0.20;
			}
			if (contents & CONTENTS_LAVA){
				VectorScale(le->velocity, 0.10, le->velocity);
				le->gravity *= 0.10;
			}

			// Don't check again later
			le->leFlags |= LE_INWATER;
		}
	}

	// Still in free fall
	VectorCopy(origin, le->entity.origin);
	VectorCopy(origin, le->entity.oldOrigin);

	R_AddEntityToScene(&le->entity);
}

/*
 =================
 CL_AddBlood
 =================
*/
static void CL_AddBlood (localEntity_t *le){

	float		time, gravity;
	vec3_t		origin;
	int			contents;
	trace_t		trace;

	if (!cl_blood->integer){
		CL_FreeLocalEntity(le);
		return;
	}

	gravity = le->gravity * (cl.playerState->pmove.gravity / 800.0);

	// Calculate origin
	time = (cl.time - le->startTime) * 0.001;
	
	origin[0] = le->origin[0] + le->velocity[0] * time;
	origin[1] = le->origin[1] + le->velocity[1] * time;
	origin[2] = le->origin[2] + le->velocity[2] * time + (gravity * time * time);

	// Calculate radius and color
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	le->entity.radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	le->entity.shaderRGBA[0] = le->color[0] * time;
	le->entity.shaderRGBA[1] = le->color[1] * time;
	le->entity.shaderRGBA[2] = le->color[2] * time;
	le->entity.shaderRGBA[3] = le->color[3] * time;

	// Trace a line from previous origin to new origin
	trace = CL_Trace(le->entity.origin, vec3_origin, vec3_origin, origin, cl.clientNum, MASK_SOLID, true, NULL);
	if (trace.fraction != 0.0 && trace.fraction != 1.0){
		VectorCopy(trace.endpos, le->entity.origin);

		R_AddEntityToScene(&le->entity);

		// Add a decal if needed
		if (le->leFlags & LE_LEAVEMARK){
			CL_ImpactMark(trace.endpos, trace.plane.normal, rand() % 360, le->entity.radius * 2.5, 1, 1, 1, 1, false, le->markShader, false);

			// Only add it once, otherwise it gets too much overdraw
			le->leFlags &= ~LE_LEAVEMARK;
		}

		// No longer needed, so free it now
		CL_FreeLocalEntity(le);
		return;
	}

	// Still in free fall
	VectorCopy(origin, le->entity.origin);

	if (!(le->leFlags & LE_INWATER)){
		// If completely underwater, make a blood cloud
		origin[2] += le->entity.radius;

		contents = CL_PointContents(origin, -1);
		if (contents & MASK_WATER){
			if (contents & CONTENTS_WATER){
				VectorScale(le->velocity, 0.25, le->velocity);
				le->gravity *= 0.125;
			}
			if (contents & CONTENTS_SLIME){
				VectorScale(le->velocity, 0.20, le->velocity);
				le->gravity *= 0.10;
			}
			if (contents & CONTENTS_LAVA){
				VectorScale(le->velocity, 0.10, le->velocity);
				le->gravity *= 0.05;
			}

			le->startTime = cl.time;
			le->endTime = le->startTime + 750 + (rand() % 250);

			VectorCopy(le->entity.origin, le->origin);
			le->radius = le->entity.radius * 2.5;
			le->entity.customShader = le->remapShader;

			// Don't leave marks underwater
			le->leFlags &= ~LE_LEAVEMARK;

			// Don't check again later
			le->leFlags |= LE_INWATER;
		}
	}

	R_AddEntityToScene(&le->entity);
}

/*
 =================
 CL_AddWaterWake
 =================
*/
static void CL_AddWaterWake (localEntity_t *le){

	polyVert_t	verts[4];
	vec3_t		axis[3];
	float		time, radius;
	int			i;

	// Calculate radius
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	radius = le->radius * (1.0 - time) + (le->radius * 0.75);

	// Find axes
	VectorCopy(le->velocity, axis[0]);
	MakeNormalVectors(axis[0], axis[1], axis[2]);

	// Scale the axes by radius
	VectorScale(axis[1], radius, axis[1]);
	VectorScale(axis[2], radius, axis[2]);

	// Build vertices
	verts[0].xyz[0] = le->origin[0] + axis[1][0] + axis[2][0];
	verts[0].xyz[1] = le->origin[1] + axis[1][1] + axis[2][1];
	verts[0].xyz[2] = le->origin[2] + axis[1][2] + axis[2][2];
	verts[1].xyz[0] = le->origin[0] - axis[1][0] + axis[2][0];
	verts[1].xyz[1] = le->origin[1] - axis[1][1] + axis[2][1];
	verts[1].xyz[2] = le->origin[2] - axis[1][2] + axis[2][2];
	verts[2].xyz[0] = le->origin[0] - axis[1][0] - axis[2][0];
	verts[2].xyz[1] = le->origin[1] - axis[1][1] - axis[2][1];
	verts[2].xyz[2] = le->origin[2] - axis[1][2] - axis[2][2];
	verts[3].xyz[0] = le->origin[0] + axis[1][0] - axis[2][0];
	verts[3].xyz[1] = le->origin[1] + axis[1][1] - axis[2][1];
	verts[3].xyz[2] = le->origin[2] + axis[1][2] - axis[2][2];

	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 0;
	verts[3].st[1] = 1;

	for (i = 0; i < 4; i++){
		verts[i].modulate[0] = le->color[0] * time;
		verts[i].modulate[1] = le->color[1] * time;
		verts[i].modulate[2] = le->color[2] * time;
		verts[i].modulate[3] = le->color[3] * time;
	}

	R_AddPolyToScene(cl.media.waterWakeShader, 4, verts);
}

/*
 =================
 CL_AddNukeShockwave
 =================
*/
static void CL_AddNukeShockwave (localEntity_t *le){

	polyVert_t	verts[4];
	vec3_t		axis[3];
	float		time, radius;
	int			i;

	// Calculate radius
	time = (le->endTime - cl.time) / (float)(le->endTime - le->startTime);
	if (time > 1.0)
		time = 1.0;

	radius = (cl.time - le->startTime) * 2;

	// Find axes
	VectorSet(axis[0], 0, 0, 1);
	MakeNormalVectors(axis[0], axis[1], axis[2]);

	// Scale the axes by radius
	VectorScale(axis[1], radius, axis[1]);
	VectorScale(axis[2], radius, axis[2]);

	// Build vertices
	verts[0].xyz[0] = le->origin[0] + axis[1][0] + axis[2][0];
	verts[0].xyz[1] = le->origin[1] + axis[1][1] + axis[2][1];
	verts[0].xyz[2] = le->origin[2] + axis[1][2] + axis[2][2];
	verts[1].xyz[0] = le->origin[0] - axis[1][0] + axis[2][0];
	verts[1].xyz[1] = le->origin[1] - axis[1][1] + axis[2][1];
	verts[1].xyz[2] = le->origin[2] - axis[1][2] + axis[2][2];
	verts[2].xyz[0] = le->origin[0] - axis[1][0] - axis[2][0];
	verts[2].xyz[1] = le->origin[1] - axis[1][1] - axis[2][1];
	verts[2].xyz[2] = le->origin[2] - axis[1][2] - axis[2][2];
	verts[3].xyz[0] = le->origin[0] + axis[1][0] - axis[2][0];
	verts[3].xyz[1] = le->origin[1] + axis[1][1] - axis[2][1];
	verts[3].xyz[2] = le->origin[2] + axis[1][2] - axis[2][2];

	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 0;
	verts[3].st[1] = 1;

	for (i = 0; i < 4; i++){
		verts[i].modulate[0] = le->color[0] * time;
		verts[i].modulate[1] = le->color[1] * time;
		verts[i].modulate[2] = le->color[2] * time;
		verts[i].modulate[3] = le->color[3] * time;
	}

	R_AddPolyToScene(cl.media.nukeShockwaveShader, 4, verts);
}

/*
 =================
 CL_Explosion
 =================
*/
void CL_Explosion (const vec3_t org, const vec3_t dir, float radius, float rotation, float light, float lightRed, float lightGreen, float lightBlue, struct shader_s *shader){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->leType = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	le->radius = radius;
	MakeRGBA(le->color, 255, 255, 255, 255);
	le->light = light;
	le->lightColor[0] = lightRed;
	le->lightColor[1] = lightGreen;
	le->lightColor[2] = lightBlue;

	le->entity.entityType = ET_SPRITE;
	VectorMA(org, radius * 0.5, dir, le->entity.origin);
	le->entity.rotation = rotation;
	le->entity.customShader = shader;
	le->entity.shaderTime = le->startTime * 0.001;
}

/*
 =================
 CL_WaterSplash
 =================
*/
void CL_WaterSplash (const vec3_t org, const vec3_t dir){

	localEntity_t	*le;

	// Plume
	le = CL_AllocLocalEntity();
	le->leType = LE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 300;
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.entityType = ET_BEAM;
	VectorCopy(org, le->entity.origin);
	VectorMA(le->entity.origin, 15, dir, le->entity.oldOrigin);
	le->entity.frame = 7.5;
	le->entity.oldFrame = 15;
	le->entity.customShader = cl.media.waterPlumeShader;

	// Spray
	le = CL_AllocLocalEntity();
	le->leType = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 400;
	le->radius = 6 + (3 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.entityType = ET_SPRITE;
	VectorMA(org, 3, dir, le->entity.origin);
	le->entity.rotation = rand() % 360;
	le->entity.customShader = cl.media.waterSprayShader;

	// Wake
	le = CL_AllocLocalEntity();
	le->leType = LE_WATER_WAKE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	VectorCopy(org, le->origin);
	VectorCopy(dir, le->velocity);
	le->radius = 10 + (5 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);
}

/*
 =================
 CL_ExplosionWaterSplash
 =================
*/
void CL_ExplosionWaterSplash (const vec3_t org){

	localEntity_t	*le;
	vec3_t			above;
	trace_t			trace;

	// Make sure it is close enough to a water (or slime) surface
	VectorSet(above, org[0], org[1], org[2] + 80);

	trace = CL_Trace(above, vec3_origin, vec3_origin, org, cl.clientNum, CONTENTS_WATER | CONTENTS_SLIME, true, NULL);
	if (trace.fraction == 0.0 || trace.fraction == 1.0)
		return;

	// Spawn splash particles
	CL_SplashParticles(trace.endpos, trace.plane.normal, 768, 30, 50);

	// Plume
	le = CL_AllocLocalEntity();
	le->leType = LE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 300;
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.entityType = ET_BEAM;
	VectorCopy(trace.endpos, le->entity.origin);
	VectorMA(le->entity.origin, 60, trace.plane.normal, le->entity.oldOrigin);
	le->entity.frame = 80;
	le->entity.oldFrame = 60;
	le->entity.customShader = cl.media.waterPlumeShader;

	// Spray
	le = CL_AllocLocalEntity();
	le->leType = LE_SCALE_FADE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 400;
	le->radius = 40 + (20 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);

	le->entity.entityType = ET_SPRITE;
	VectorMA(trace.endpos, 20, trace.plane.normal, le->entity.origin);
	le->entity.rotation = rand() % 360;
	le->entity.customShader = cl.media.waterSprayShader;

	// Wake
	le = CL_AllocLocalEntity();
	le->leType = LE_WATER_WAKE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1000;
	VectorCopy(trace.endpos, le->origin);
	VectorCopy(trace.plane.normal, le->velocity);
	le->radius = 40 + (20 * crand());
	MakeRGBA(le->color, 255, 255, 255, 255);
}

/*
 =================
 CL_Sprite
 =================
*/
void CL_Sprite (const vec3_t org, float radius, struct shader_s *shader){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->leType = LE_ENTITY;
	le->leFlags = LE_INSTANT;

	le->startTime = cl.time;
	le->endTime = le->startTime + 1;

	le->entity.entityType = ET_SPRITE;
	VectorCopy(org, le->entity.origin);
	le->entity.radius = radius;
	le->entity.customShader = shader;
	MakeRGBA(le->entity.shaderRGBA, 255, 255, 255, 255);
}

/*
 =================
 CL_LaserBeam
 =================
*/
void CL_LaserBeam (const vec3_t start, const vec3_t end, int width, int color, byte alpha, int duration, struct shader_s *shader){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->leType = LE_ENTITY;
	le->leFlags = (duration == 1) ? LE_INSTANT : 0;

	le->startTime = cl.time;
	le->endTime = le->startTime + duration;

	le->entity.entityType = ET_BEAM;
	VectorCopy(start, le->entity.origin);
	VectorCopy(end, le->entity.oldOrigin);
	le->entity.frame = width;
	le->entity.oldFrame = 50;
	le->entity.customShader = shader;

	// HACK: the four beam colors are encoded in 32 bits
	color = (color >> ((rand() % 4) * 8)) & 0xff;

	le->entity.shaderRGBA[0] = cl_colorPalette[color][0];
	le->entity.shaderRGBA[1] = cl_colorPalette[color][1];
	le->entity.shaderRGBA[2] = cl_colorPalette[color][2];
	le->entity.shaderRGBA[3] = alpha;
}

/*
 =================
 CL_MachinegunEjectBrass
 =================
*/
void CL_MachinegunEjectBrass (centity_t *cent, int count, float x, float y, float z){

	localEntity_t	*le;
	vec3_t			velocity;
	vec3_t			angles, axis[3];
	int				i;

	if (cl_brassTime->integer <= 0)
		return;

	if (cent->current.number == cl.clientNum){
		if (cl_hand->integer == 2 && !cl_thirdPerson->integer)
			return;

		if (cl_hand->integer == 1 && !cl_thirdPerson->integer)
			y = -y;

		if (cl.playerState->pmove.pm_flags & PMF_DUCKED)
			z -= 24;
	}

	AnglesToAxis(cent->current.angles, axis);

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->leType = LE_EJECT_BRASS;
		le->leFlags = LE_BOUNCESOUND;

		le->startTime = cl.time;
		le->endTime = le->startTime + cl_brassTime->integer + (cl_brassTime->integer / 4) * frand();

		cent->current.origin[0] = le->origin[0] + (x * axis[0][0] + y * axis[1][0] + z * axis[2][0]);
		cent->current.origin[1] = le->origin[1] + (x * axis[0][1] + y * axis[1][1] + z * axis[2][1]);
		cent->current.origin[2] = le->origin[2] + (x * axis[0][2] + y * axis[1][2] + z * axis[2][2]);

		VectorSet(velocity, 20 * crand(), -50 + 40 * crand(), 100 + 50 * crand());

		if (cent->current.number == cl.clientNum){
			if (cl_hand->integer == 1 && !cl_thirdPerson->integer)
				velocity[1] = -velocity[1];
		}

		le->velocity[0] = velocity[0] * axis[0][0] + velocity[1] * axis[1][0] + velocity[2] * axis[2][0];
		le->velocity[1] = velocity[0] * axis[0][1] + velocity[1] * axis[1][1] + velocity[2] * axis[2][1];
		le->velocity[2] = velocity[0] * axis[0][2] + velocity[1] * axis[1][2] + velocity[2] * axis[2][2];

		le->gravity = -400;

		le->bounceFactor = 1.2;
		le->bounceSound = clMedia.sfx.machinegunBrass[(rand()&10)];

		angles[0] = cent->current.angles[0] + (rand() & 31);
		angles[1] = cent->current.angles[1] + (rand() & 31);
		angles[2] = cent->current.angles[2] + (rand() & 31);

		le->entity.entityType = ET_MODEL;
		le->entity.model = clMedia.machinegunBrassModel;
		VectorCopy(le->origin, le->entity.origin);
		AnglesToAxis(angles, le->entity.axis);
		MakeRGBA(le->entity.shaderRGBA, 255, 255, 255, 255);
	}
}

/*
 =================
 CL_ShotgunEjectBrass
 =================
*/
void CL_ShotgunEjectBrass (const centity_t *cent, int count, float x, float y, float z){

	localEntity_t	*le;
	vec3_t			velocity;
	vec3_t			angles, axis[3];
	int				i;

	if (cl_brassTime->integer <= 0)
		return;

	if (cent->current.number == cl.clientNum){
		if (cl_hand->integer == 2 && !cl_thirdPerson->integer)
			return;

		if (cl_hand->integer == 1 && !cl_thirdPerson->integer)
			y = -y;

		if (cl.playerState->pmove.pm_flags & PMF_DUCKED)
			z -= 24;
	}

	AnglesToAxis(cent->current.angles, axis);

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->leType = LE_EJECT_BRASS;
		le->leFlags = LE_BOUNCESOUND;

		le->startTime = cl.time;
		le->endTime = le->startTime + cl_brassTime->integer + (cl_brassTime->integer / 4) * frand();

		le->origin[0] = cent->current.origin[0] + (x * axis[0][0] + y * axis[1][0] + z * axis[2][0]);
		le->origin[1] = cent->current.origin[1] + (x * axis[0][1] + y * axis[1][1] + z * axis[2][1]);
		le->origin[2] = cent->current.origin[2] + (x * axis[0][2] + y * axis[1][2] + z * axis[2][2]);

		VectorSet(velocity, 60 + 60 * crand(), 10 * crand(), 100 + 50 * crand());

		if (count == 2){
			if (i == 0)
				velocity[1] += 40;
			else
				velocity[1] -= 40;
		}

		if (cent->current.number == cl.clientNum){
			if (cl_hand->integer == 1 && !cl_thirdPerson->integer)
				velocity[1] = -velocity[1];
		}

		le->velocity[0] = velocity[0] * axis[0][0] + velocity[1] * axis[1][0] + velocity[2] * axis[2][0];
		le->velocity[1] = velocity[0] * axis[0][1] + velocity[1] * axis[1][1] + velocity[2] * axis[2][1];
		le->velocity[2] = velocity[0] * axis[0][2] + velocity[1] * axis[1][2] + velocity[2] * axis[2][2];

		le->gravity = -300;

		le->bounceFactor = 0.8;
		le->bounceSound = clMedia.sfx.shotgunBrass[(rand()&2)];

		angles[0] = cent->current.angles[0] + (rand() & 31);
		angles[1] = cent->current.angles[1] + (rand() & 31);
		angles[2] = cent->current.angles[2] + (rand() & 31);

		le->entity.entityType = ET_MODEL;
		le->entity.model = clMedia.shotgunBrassModel;
		VectorCopy(le->origin, le->entity.origin);
		AnglesToAxis(angles, le->entity.axis);
		MakeRGBA(le->entity.shaderRGBA, 255, 255, 255, 255);
	}
}

/*
 =================
 CL_Bleed
 =================
*/
void CL_Bleed (const vec3_t org, const vec3_t dir, int count, qboolean green){

	localEntity_t	*le;
	int				type = green & 1;
	int				i;

	if (!cl_blood->integer)
		return;

	for (i = 0; i < count; i++){
		le = CL_AllocLocalEntity();
		le->leType = LE_BLOOD;

		le->startTime = cl.time;
		le->endTime = le->startTime + 750 + (rand() % 250);

		le->origin[0] = org[0] + crand() * 3;
		le->origin[1] = org[1] + crand() * 3;
		le->origin[2] = org[2] + crand() * 3;
		le->velocity[0] = dir[0] * 30 + crand() * 15;
		le->velocity[1] = dir[1] * 30 + crand() * 15;
		le->velocity[2] = dir[2] * 10 + crand() * 5;

		le->gravity = -300;
		le->radius = 6 + (6 * crand());
		MakeRGBA(le->color, 255, 255, 255, 255);
		le->remapShader = cl.media.bloodCloudShader[type];

		if (!(i % 4)){
			le->leFlags |= LE_LEAVEMARK;
			le->markShader = clMedia.bloodMarkMaterials[type][rand() % 8];
		}

		le->entity.entityType = ET_SPRITE;
		VectorCopy(le->origin, le->entity.origin);
		le->entity.rotation = rand() % 360;
		le->entity.customShader = cl.media.bloodSplatShader[type];
	}
}

/*
 =================
 CL_BloodTrail
 =================
*/
void CL_BloodTrail (const vec3_t start, const vec3_t end, qboolean green){

	localEntity_t	*le;
	vec3_t			move, vec;
	float			len, dec;
	int				type = green & 1;
	int				i;

	if (!cl_blood->integer)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 10 + (20 * type);
	VectorScale(vec, dec, vec);

	i = 0;

	while (len > 0){
		len -= dec;
		i++;

		le = CL_AllocLocalEntity();
		le->leType = LE_BLOOD;

		le->startTime = cl.time;
		le->endTime = le->startTime + 750 + (rand() % 250);

		le->origin[0] = move[0] + crand() * 3;
		le->origin[1] = move[1] + crand() * 3;
		le->origin[2] = move[2] + crand() * 3;
		le->velocity[0] = vec[0] * 30 + crand() * 15;
		le->velocity[1] = vec[1] * 30 + crand() * 15;
		le->velocity[2] = vec[2] * 10 + crand() * 5;

		le->gravity = -300;
		le->radius = 6 + (3 * crand());
		MakeRGBA(le->color, 255, 255, 255, 255);
		le->remapShader = cl.media.bloodCloudShader[type];

		if (!(i % 2)){
			le->leFlags |= LE_LEAVEMARK;
			le->markShader = clMedia.bloodMarkMaterials[type][rand() % 8];
		}

		le->entity.entityType = ET_SPRITE;
		VectorCopy(le->origin, le->entity.origin);
		le->entity.rotation = rand() % 360;
		le->entity.customShader = cl.media.bloodSplatShader[type];

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_NukeShockwave
 =================
*/
void CL_NukeShockwave (const vec3_t org){

	localEntity_t	*le;

	le = CL_AllocLocalEntity();
	le->leType = LE_NUKE_SHOCKWAVE;

	le->startTime = cl.time;
	le->endTime = le->startTime + 500;

	VectorSet(le->origin, org[0], org[1], org[2] + 30);
	MakeRGBA(le->color, 255, 255, 255, 255);
}

/*
 =================
 CL_ClearLocalEntities
 =================
*/
void CL_ClearLocalEntities (void){

	int		i;
	byte	palette[] = {
#include "../refresh/palette.h"
	};

	memset(cl_localEntities, 0, sizeof(cl_localEntities));

	cl_activeLocalEntities.next = &cl_activeLocalEntities;
	cl_activeLocalEntities.prev = &cl_activeLocalEntities;
	cl_freeLocalEntities = cl_localEntities;

	for (i = 0; i < MAX_LOCAL_ENTITIES - 1; i++)
		cl_localEntities[i].next = &cl_localEntities[i+1];

	for (i = 0; i < 256; i++){
		cl_colorPalette[i][0] = palette[i*3+0];
		cl_colorPalette[i][1] = palette[i*3+1];
		cl_colorPalette[i][2] = palette[i*3+2];
	}
}

/*
 =================
 CL_AddLocalEntities
 =================
*/
void CL_AddLocalEntities (void){

	localEntity_t	*le, *prev;

	for (le = cl_activeLocalEntities.prev; le != &cl_activeLocalEntities; le = prev){
		// Grab prev now, so if the entity is freed we still have it
		prev = le->prev;

		if (cl.time >= le->endTime){
			CL_FreeLocalEntity(le);
			continue;
		}

		switch (le->leType){
		case LE_FADE:
			CL_AddFade(le);
			break;
		case LE_SCALE:
			CL_AddScale(le);
			break;
		case LE_SCALE_FADE:
			CL_AddScaleFade(le);
			break;
		case LE_MOVE_FADE:
			CL_AddMoveFade(le);
			break;
		case LE_MOVE_SCALE:
			CL_AddMoveScale(le);
			break;
		case LE_MOVE_SCALE_FADE:
			CL_AddMoveScaleFade(le);
			break;
		case LE_ENTITY:
			CL_AddEntity(le);
			break;
		case LE_EJECT_BRASS:
			CL_AddEjectBrass(le);
			break;
		case LE_BLOOD:
			CL_AddBlood(le);
			break;
		case LE_WATER_WAKE:
			CL_AddWaterWake(le);
			break;
		case LE_NUKE_SHOCKWAVE:
			CL_AddNukeShockwave(le);
			break;
		default:
			Com_Error(ERR_DROP, "CL_AddLocalEntities: bad leType (%i)", le->leType);
		}
	}
}
