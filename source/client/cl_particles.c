/*
===========================================================================
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2009-2010 Quake 2 Evolution.

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
===========================================================================
*/

//
// cl_particles.c
//

#include "cl_local.h"


#define PARTICLE_BOUNCE			1
#define PARTICLE_FRICTION		2
#define PARTICLE_VERTEXLIGHT	4
#define PARTICLE_STRETCH		8
#define PARTICLE_UNDERWATER		16
#define PARTICLE_INSTANT		32

typedef struct cparticle_s {
	struct		cparticle_s *next;

	struct		            shader_s *shader;
	int			            time;
	int			            flags;

	vec3_t		            org;
	vec3_t                  angle;
	vec3_t		            vel;
	vec3_t		            accel;
	vec4_t		            color,		            colorVel;
	float		            size,                   sizeVel;
	float		            length,                 lengthVel;
	float		            rotation;
	float		            bounceFactor;
	float                   alpha, alphaVel; // ::REMOVE ME::

	vec3_t		            lastOrg;

	qboolean				(*preThink)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time);
	qboolean				bPreThinkNext;
	float					lastPreThinkTime;
	float					nextPreThinkTime;
	vec3_t					lastPreThinkOrigin;

	qboolean				(*think)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time);
	qboolean				bThinkNext;
	float					lastThinkTime;
	float					nextThinkTime;
	vec3_t					lastThinkOrigin;

	qboolean				(*postThink)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time);
	qboolean				bPostThinkNext;
	float					lastPostThinkTime;
	float					nextPostThinkTime;
	vec3_t					lastPostThinkOrigin;
} cparticle_t;

static cparticle_t	*cl_activeParticles;
static cparticle_t	*cl_freeParticles;
static cparticle_t	cl_particleList[MAX_PARTICLES];

static vec3_t		cl_particleVelocities[NUM_VERTEX_NORMALS];
static vec3_t		cl_particlePalette[256];

/*
 ==================
 CL_AllocParticle
 ==================
*/
static cparticle_t *CL_AllocParticle ()
{
	cparticle_t	*p;

	if (!cl_freeParticles)
		return NULL;

	if (cl_particleLOD->integer > 1)
	{
		if (!(rand() % cl_particleLOD->integer))
			return NULL;
	}

	p = cl_freeParticles;
	cl_freeParticles = p->next;
	p->next = cl_activeParticles;
	cl_activeParticles = p;

	return p;
}

/*
 ==================
 CL_FreeParticle
 ==================
*/
static void CL_FreeParticle (cparticle_t *p)
{
	p->next = cl_freeParticles;
	cl_freeParticles = p;
}

/*
 ==================
 CL_SpawnParticle
 ==================
*/
void CL_SpawnParticle (float org0,						       float org1,						       float org2,
					   float angle0,					       float angle1,				           float angle2,
					   float vel0,						       float vel1,					           float vel2,
					   float accel0,					       float accel1,				           float accel2,
					   float red,						       float green,				               float blue,
					   float redVel,					       float greenVel,				           float blueVel,
					   unsigned int type,
					   float alpha,					           float alphaVel, 
					   float size,						       float sizeVel,
					   unsigned int flags,
					   float bounce,
					   float rotation,
					   float length,                           float lengthVel,
					   qboolean (*preThink)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time),
					   qboolean (*think)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time),
					   qboolean (*postThink)(struct cparticle_t *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec3_t color, float *size, float *rotation, float *time))
{
	cparticle_t   *p = CL_AllocParticle ();
	if (!p)
		return;

	// Time
	p->time = (float)cl.time;

	// Origin
	Vec3Set (p->org, org0, org1, org2);
	Vec3Copy (p->org, p->lastOrg);

	// Angle
	Vec3Set (p->angle, angle0, angle1, angle2);
	 
	// Velocity
	Vec3Set (p->vel, vel0, vel1, vel2);

	// Acceleration
	Vec3Set (p->accel, accel0, accel1, accel2);

	// Color and alpha
	Vec4Set (p->color, red, green, blue, alpha);
	Vec4Set (p->colorVel, redVel, greenVel, blueVel, alphaVel);

	// Particle texture type
	p->shader = clMedia.particleTable[type%PT_PICTOTAL];

	// Size
	p->size = size;
	p->sizeVel = sizeVel;

	// Flags
	p->flags = flags;

	// Bouncing
	p->bounceFactor = bounce;

	// Rendering flags (TODO!)

	// Rotation
	p->rotation = rotation;

	// Length
	p->length = length;
	p->lengthVel = lengthVel;

	// Think functions
	p->preThink = preThink;
	p->bPreThinkNext = (preThink != NULL);
	if (p->bPreThinkNext)
	{
		p->lastPreThinkTime = p->time;
		p->nextPreThinkTime = p->time;
		Vec3Copy (p->org, p->lastPreThinkOrigin);
	}

	p->think = think;
	p->bThinkNext = (think != NULL);
	if (p->bThinkNext)
	{
		p->lastThinkTime = p->time;
		p->nextThinkTime = p->time;
		Vec3Copy (p->org, p->lastThinkOrigin);
	}

	p->postThink = postThink;
	p->bPostThinkNext = (postThink != NULL);
	if (p->bPostThinkNext)
	{
		p->lastPostThinkTime = p->time;
		p->nextPostThinkTime = p->time;
		Vec3Copy (p->org, p->lastPostThinkOrigin);
	}
}

/*
 ==================
 CL_SurfaceParticles

 - FIXME: This will be called in the CL_ParseTempEnt func
 where TE_SHOTGUN and TE_BULLET is also in the footstep func
 ==================
*/
void CL_SurfaceParticles (const vec3_t org, const vec3_t dir)
{
	//cparticle_t   *p;

	// Metal
//	if (p->shader->surfaceType |= SURFACE_TYPE_METAL)
//		CL_BulletParticles (org, dir);
}

/*
 ==================
 CL_ClearParticles
 ==================
*/
void CL_ClearParticles ()
{
	int		i;
	byte	palette[] = {
#include "../refresh/palette.h"
	};

	cl_activeParticles = NULL;
	cl_freeParticles = cl_particleList;

	for (i = 0; i < MAX_PARTICLES; i++)
		cl_particleList[i].next = &cl_particleList[i+1];

	cl_particleList[MAX_PARTICLES-1].next = NULL;

	// Store vertex normals
	for (i = 0; i < NUM_VERTEX_NORMALS; i++)
	{
		cl_particleVelocities[i][0] = (rand() & 255) * 0.01;
		cl_particleVelocities[i][1] = (rand() & 255) * 0.01;
		cl_particleVelocities[i][2] = (rand() & 255) * 0.01;
	}

	// Store palette colors
	for (i = 0; i < 256; i++)
	{
		cl_particlePalette[i][0] = palette[i*3+0] * (1.0/255);
		cl_particlePalette[i][1] = palette[i*3+1] * (1.0/255);
		cl_particlePalette[i][2] = palette[i*3+2] * (1.0/255);
	}
}

/*
 =================
 CL_AddParticles
 =================
*/
#define PART_INSTANT	-1000.0f
void CL_AddParticles (void)
{
	cparticle_t	*p, *next;
	cparticle_t	*active = NULL, *tail = NULL;
	color_t		modulate;
	vec3_t		org, org2, vel;
	vec3_t		ambientLight;
	float		length;
	float		time, gravity, dot;
	int			contents;
	vec3_t		mins, maxs;
	trace_t		trace;

	vec4_t      color;
	qboolean    bHadAThought = false;
	float       timeSquared;
	float       size, rotation = p->rotation;
	int         i;

	if (!cl_particles->integer)
		return;

	gravity = cl.playerState->pmove.gravity / 800.0;

	for (p = cl_activeParticles; p; p = next)
	{
		// Grab next now, so if the particle is freed we still have it
		next = p->next;

		// Alpha and time calcs
		if (p->colorVel[3] > PART_INSTANT)
		{
			time = (cl.time - p->time) * 0.001f;
			color[3] = p->color[3] + time * p->colorVel[3];
		}
		else
		{
			time = 1;
			color[3] = p->color[3];
		}

		// sizeVel calcs
		if (p->colorVel[3] > PART_INSTANT && p->size != p->sizeVel)
		{
			if (p->size > p->sizeVel) // Shrink
				size = p->size - ((p->size - p->sizeVel) * (p->color[3] - color[3]));
			else // Grow
				size = p->size + ((p->sizeVel - p->size) * (p->color[3] - color[3]));
		}
		else
			size = p->size;

		// Length calcs
		length = p->length + p->lengthVel * time;

		// Faded out
		if (color[3] <= 0 || size <= 0 || length <= 0)
		{
			CL_FreeParticle (p);
			continue;
		}

		// Alpha should not reach over 1.0f there for we
		// set it to 1.0f max
		if (color[3] > 1.0)
			color[3] = 1.0f;

		// colorVel calcs
		Vec3Copy(p->color, color);
		if (p->colorVel[3] > PART_INSTANT)
		{
			for (i=0 ; i<3 ; i++)
			{
				if (p->color[i] != p->colorVel[i])
				{
					if (p->color[i] > p->colorVel[i])
						color[i] = p->color[i] - ((p->color[i] - p->colorVel[i]) * (p->color[3] - color[3]));
					else
						color[i] = p->color[i] + ((p->colorVel[i] - p->color[i]) * (p->color[3] - color[3]));
				}
			}
		}

		// Origin
		timeSquared = time * time;
		org[0] = p->org[0] + p->vel[0] * time + p->accel[0] * timeSquared;
		org[1] = p->org[1] + p->vel[1] * time + p->accel[1] * timeSquared;
		org[2] = p->org[2] + p->vel[2] * time + p->accel[2] * timeSquared * gravity;

		// Think functions
		if (p->bPreThinkNext && cl.time >= p->nextPreThinkTime)
		{
			p->bPreThinkNext = p->preThink(p, cl.time-p->lastPreThinkTime, &p->nextPreThinkTime, org, p->lastPreThinkOrigin, p->angle, color, &size, &rotation, &time);
			p->lastPreThinkTime = cl.time;
			Vec3Copy(org, p->lastPreThinkOrigin);

			bHadAThought = true;
		}

		if (p->bThinkNext && cl.time >= p->nextThinkTime)
		{
			p->bThinkNext = p->think(p, cl.time-p->lastThinkTime, &p->nextThinkTime, org, p->lastThinkOrigin, p->angle, color, &size, &rotation, &time);
			p->lastThinkTime = cl.time;
			Vec3Copy(org, p->lastThinkOrigin);

			bHadAThought = true;
		}

		if (p->bPostThinkNext && cl.time >= p->nextPostThinkTime)
		{
			p->bPostThinkNext = p->postThink(p, cl.time-p->lastPostThinkTime, &p->nextPostThinkTime, org, p->lastPostThinkOrigin, p->angle, color, &size, &rotation, &time);
			p->lastPostThinkTime = cl.time;
			Vec3Copy(org, p->lastPostThinkOrigin);

			bHadAThought = true;
		}

		// Check alpha and size after the think function runs
		if (bHadAThought)
		{
			if (color[3] <= TINY_NUMBER)
			{
				goto nextParticle;
			}
			if (size <= TINY_NUMBER)
			{
				goto nextParticle;
			}
		}

		// Underwater particle
		if (p->flags & PARTICLE_UNDERWATER)
		{
			VectorSet (org2, org[0], org[1], org[2] + size);

			// Not underwater
			if (!(CL_PointContents(org2, -1) & MASK_WATER))
			{
				CL_FreeParticle (p);
				continue;
			}
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else 
		{
			tail->next = p;
			tail = p;
		}

        // Water friction affected particle
		if (p->flags & PARTICLE_FRICTION)
		{
			contents = CL_PointContents(org, -1);
			if (contents & MASK_WATER)
			{
				// Add friction
				if (contents & CONTENTS_WATER)
				{
					VectorScale (p->vel, 0.25f, p->vel);
					VectorScale (p->accel, 0.25f, p->accel);
				}

				if (contents & CONTENTS_SLIME)
				{
					VectorScale (p->vel, 0.20f, p->vel);
					VectorScale (p->accel, 0.20f, p->accel);
				}

				if (contents & CONTENTS_LAVA)
				{
					VectorScale (p->vel, 0.10f, p->vel);
					VectorScale (p->accel, 0.10f, p->accel);
				}

				length = 1;

				// Don't add friction again
				p->flags &= ~PARTICLE_FRICTION;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->org);
				VectorCopy (color, p->color);
				p->color[3] = color[3];
				p->size = size;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0;
			}
		}

		// Bouncy particle
		if (p->flags & PARTICLE_BOUNCE)
		{
			VectorSet (mins, -size, -size, -size);
			VectorSet (maxs, size, size, size);

			trace = CL_Trace(p->lastOrg, mins, maxs, org, cl.clientNum, MASK_SOLID, true, NULL);

			if (trace.fraction != 0.0 && trace.fraction != 1.0)
			{
				// Reflect velocity
				time = cl.time - (cls.frameTime + cls.frameTime * trace.fraction) * 1000;
				time = (time - p->time) * 0.001;

				VectorSet (vel, p->vel[0], p->vel[1], p->vel[2] + p->accel[2] * gravity * time);
				VectorReflect (vel, trace.plane.normal, p->vel);
				VectorScale (p->vel, p->bounceFactor, p->vel);

				// Check for stop or slide along the plane
				if (trace.plane.normal[2] > 0 && p->vel[2] < 2)
				{
					if (trace.plane.normal[2] == 1)
					{
						VectorClear (p->vel);
						VectorClear (p->accel);

						p->flags &= ~PARTICLE_BOUNCE;
					}
					else
					{
						// FIXME: check for new plane or free fall
						dot = DotProduct (p->vel, trace.plane.normal);
						VectorMA (p->vel, -dot, trace.plane.normal, p->vel);

						dot = DotProduct (p->accel, trace.plane.normal);
						VectorMA (p->accel, -dot, trace.plane.normal, p->accel);
					}
				}

				VectorCopy(trace.endpos, org);
				length = 1;

				// Reset
				p->time = cl.time;
				VectorCopy(org, p->org);
				VectorCopy(color, p->color);
				p->color[3] = color[3];
				p->size = size;

				// Don't stretch
				p->flags &= ~PARTICLE_STRETCH;

				p->length = length;
				p->lengthVel = 0;
			}
		}

		// Save current origin if needed
		if (p->flags & (PARTICLE_BOUNCE | PARTICLE_STRETCH))
		{
			VectorCopy (p->lastOrg, org2);
			VectorCopy (org, p->lastOrg);	// FIXME: pause
		}

		// Vertex lit particle
		if (p->flags & PARTICLE_VERTEXLIGHT)
		{
			R_LightForPoint (org, ambientLight);

			VectorMultiply (color, ambientLight, color);
		}

		// Clamp color and alpha and convert to byte
		modulate[0] = 255 * Clamp(color[0], 0.0, 1.0);
		modulate[1] = 255 * Clamp(color[1], 0.0, 1.0);
		modulate[2] = 255 * Clamp(color[2], 0.0, 1.0);
		modulate[3] = 255 * Clamp(color[3], 0.0, 1.0);

		// Instant particle
		if (p->flags & PARTICLE_INSTANT)
		{
			p->alpha = 0;
			p->alphaVel = 0;
		}

		// Kill if particle is instant
nextParticle:	
		if (p->colorVel[3] <= PART_INSTANT)
		{
			p->color[3] = 0;
			p->colorVel[3] = 0;
		}

		// Send the particle to the renderer
		R_AddParticleToScene (p->shader, org, org2, size, length, p->rotation, modulate, p->flags);
	}

	cl_activeParticles = active;
}

/*
 ==================
 CL_FindExplosionDir

 - This is a "necessary hack" for explosion decal placement.
 ==================
*/
qboolean CL_FindExplosionDir (vec3_t origin, float radius, vec3_t endPos, vec3_t dir)
{
	static vec3_t	planes[6] = {{0,0,1}, {0,1,0}, {1,0,0}, {0,0,-1}, {0,-1,0}, {-1,0,0}};
	trace_t		    trace;
	vec3_t			tempDir;
	vec3_t			tempOrg;
	float			best = 1.0f;
	int				i;

	Vec3Clear (endPos);
	Vec3Clear (dir);
	for (i = 0; i < 6; i++) 
	{
		Vec3MA (origin, radius * 0.9f, planes[i], tempDir);
		Vec3MA (origin, radius * 0.1f, planes[(i+3)%6], tempOrg);

		CL_PMTraceDecal (&trace, tempOrg, NULL, NULL, tempDir, false, true);
		if (trace.allsolid || trace.fraction == 1.0f)
			continue;

		if (trace.fraction < best)
		{
			best = trace.fraction;
			Vec3Copy (trace.endpos, endPos);
			Vec3Copy (trace.plane.normal, dir);
		}
	}

	// FIXME: why does this "fix" decal normals on xdmt4? Something to do with the fragment planes...
	if (best < 1.0f) 
	{
		byte dirByte = DirToByte (dir);
		ByteToDir (dirByte, dir);
		return true;
	}

	return false;
}

/*
==============================================================================

	THINK FUNCTION ROUTINES
	
==============================================================================
*/  

qboolean pSmokeThink (struct cparticle_s *p, const float deltaTime, float *nextThinkTime, vec3_t org, vec3_t lastOrg, vec3_t angle, vec4_t color, float *size, float *rotation, float *time)
{
	if ((int)p->org[2] & 1)
		p->rotation -= deltaTime * 0.05f * (1.0f - (color[3] * color[3]));
	else
		p->rotation += deltaTime * 0.05f * (1.0f - (color[3] * color[3]));
	*rotation = p->rotation;

	nextThinkTime = cl.time;
	return true;
}

/*
==============================================================================

	ORIGINAL QUAKE 2 PARTICLES	
	
==============================================================================
*/  

/*
 ==================
 CL_BlasterTrail
 ==================
*/
void CL_BlasterTrail (const vec3_t start, const vec3_t end, float r, float g, float b)
{
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integer)
		return;

	// Sparks
	Vec3Copy (start, move);
	Vec3Subtract (end, start, vec);
	len = VectorNormalize (vec);

	dist = 1.5f;
	Vec3Scale (vec, dist, vec);

	for (; len > 0; Vec3Add (move, vec, move))
	{
		len -= dist;

		CL_SpawnParticle (
			move[0] + crand (),					   move[1] + crand (),					   move[2] + crand (),
			0,					                   0,				                       0,
			crand () * 3,						   crand () * 3,					       crand () * 3,
			0,					                   0,				                       0,
			r,						               g,				                       b,
			0,					                   0,				                       0,
			PT_ENERGY,
			1.0f,					               -1.0f / (0.9f + frand () * 0.2f), 
			2.4 + (1.2 * crand ()),				   -2.4f + (1.2f * crand ()),
			0,
			0,
			0,
			1,                                     0,
			NULL,								   NULL,								   NULL);
	}
}

/*
 ==================
 CL_RocketTrail
 ==================
*/
void CL_RocketTrail (const vec3_t start, const vec3_t end)
{
	vec3_t		move, vec;
	float		len, dist;
	int			flags = 0;

	if (!cl_particles->integer)
		return;

	// Check for water
	if (CL_PointContents (end, -1) & MASK_WATER)
	{
		// Found it, so do a bubble trail
		if (CL_PointContents (start, -1) & MASK_WATER)
			CL_BubbleTrail (start, end, 8, 3);

		return;
	}

	// Fire
	Vec3Copy (start, move);
	Vec3Subtract (end, start, vec);
	len = VectorNormalize (vec);

	dist = 0.3f;
	Vec3Scale (vec, dist, vec);

	for (; len > 0; Vec3Add (move, vec, move))
	{
		len -= dist;

		CL_SpawnParticle (
			move[0] + crand (),					   move[1] + crand (),					   move[2] + crand (),
			0,					                   0,				                       0,
			crand () * 5,						   crand () * 5,					       crand () * 5 + (25 + crand () * 5), 
			0,					                   0,				                       0,
			1.0,					               1.0,				                       1.0,
			0,					                   0,				                       0,
			PT_FIRE1,
			0.9f + (crand () * 0.1f),			   -1.0f / (0.15f + (crand () * 0.05f)),
			1.0 + (crand () * 2),				   8 + (frand () * 2),
			0,
			0,
			frand () * 360,
			1,                                     0,
			NULL,								   NULL,								   NULL);
	}
	
	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	Vec3Copy (start, move);
	Vec3Subtract (end, start, vec);
	len = VectorNormalize (vec);

	dist = 10.0f;
	Vec3Scale (vec, dist, vec);

	for (; len > 0; Vec3Add (move, vec, move))
	{
		len -= dist;

		CL_SpawnParticle (
			move[0] + crand (),					   move[1] + crand (),					   move[2] + crand (),
			0,					                   0,				                       0,
			crand () * 5,						   crand () * 5,					       crand () * 5 + (25 + crand () * 5), 
			0,					                   0,				                       0,
			0.15f,					               0.15f,				                   0.15f,
			0.75,					               0.75,				                   0.75,
			PT_SMOKE1 + (rand () & 2), 
			0.4f + (crand () * 0.1f),			   -8.9f / (1.0f + (3 * 10.0f) + (crand () * 0.15f)),
			8 + (crand () * 4),					   40 + (frand () * 20),
			flags,
			0,
			frand () * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}
}

/*
 ==================
 CL_GrenadeTrail
 ==================
*/
void CL_GrenadeTrail (const vec3_t start, const vec3_t end)
{
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integer)
		return;

	// Check for water
	if (CL_PointContents (end, -1) & MASK_WATER)
	{
		// Found it, so do a bubble trail
		if (CL_PointContents (start, -1) & MASK_WATER)
			CL_BubbleTrail (start, end, 16, 1);

		return;
	}
	
	// Smoke puffs
	Vec3Copy (start, move);
	Vec3Subtract (end, start, vec);
	len = VectorNormalize (vec);

	dist = 20;
	Vec3Scale (vec, dist, vec);

	for (; len > 0; Vec3Add (move, vec, move))
	{
		len -= dist;

		CL_SpawnParticle (
			move[0] + crand (),					   move[1] + crand (),					   move[2] + crand (),
			0,					                   0,				                       0,
			crand () * 5,						   crand () * 5,					       crand () * 5 + (25 + crand () * 5), 
			0,					                   0,				                       0,
			0.75,					               0.75,				                   0.75,
			0,					                   0,				                       0,
			PT_SMOKE1 + (rand () & 2), 
			0.4f + (crand () * 0.1f),			   -8.9f / (1.0f + (3 * 10.0f) + (crand () * 0.15f)),
			1 + (crand () * 4),					   0.5 + (frand () * 20),
			PARTICLE_VERTEXLIGHT,
			0,
			frand () * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}
}

/*
 ==================
 CL_BubbleTrail
 ==================
*/
void CL_BubbleTrail (const vec3_t start, const vec3_t end, float dist, float radius)
{
	vec3_t		move, vec;
	float		len;

	if (!cl_particles->integer)
		return;

	Vec3Copy (start, move);
	Vec3Subtract (end, start, vec);
	len = VectorNormalize (vec);

	Vec3Scale(vec, dist, vec);

	for (; len > 0; Vec3Add (move, vec, move))
	{
		len -= dist;

		CL_SpawnParticle (
			move[0] + crand (),					   move[1] + crand (),					   move[2] + crand (),
			0,					                   0,				                       0,
			crand () * 5,						   crand () * 5,					       crand () * 5 + (35 + crand () * 5), 
			0,					                   0,				                       50,
			1,					                   1,				                       1,
			0,					                   0,				                       0,
			PT_BUBBLE, 
			1.0,			                       -(0.4 + frand() * 0.2),
			radius + ((radius * 0.5) * crand()),   0,
			PARTICLE_UNDERWATER,
			0,
			0,
			1,                                     0,
			NULL,						           NULL,								   NULL);
	}
}

/*
 ==================
 CL_BlasterParticles
 ==================
*/
void CL_BlasterParticles (const vec3_t org, const vec3_t dir, float r, float g, float b)
{
	int     flags;
	int		i;

	if (!cl_particles->integer)
		return;

	// Sparks
	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 40; i++)
	{
		CL_SpawnParticle (
			org[0] + dir[0] * 3 + crand (),		   org[1] + dir[1] * 3 + crand (),		   org[2] + dir[2] * 3 + crand (),
			0,					                   0,				                       0,
			dir[0] * 25 + crand () * 20,		   dir[1] * 25 + crand () * 20,			   dir[2] * 25 + crand () * 20,
			0,					                   0,				                       -120 + (40 * crand ()),
			r,						               g,				                       b,
			0,					                   0,				                       0,
			PT_ENERGY,
			1.0f,					               -0.25f / (0.3f + frand () * 0.2f), 
			2.4f + (1.2f * crand ()),			   -1.2f + (0.6f * crand ()),
			flags,
			0.7f,
			0,
			1,                                     0,
			NULL,								   NULL,								   NULL);
	}

	// Dont do steam underwater
	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	// Steam
	for (i = 0; i < 3; i++)
	{
		CL_SpawnParticle (
			org[0] + dir[0] * 5 + crand (),		   org[1] + dir[1] * 5 + crand (),		   org[2] + dir[2] * 5 + crand (),
			0,					                   0,				                       0,
			crand () * 2.5f,		               crand () * 2.5f,			               crand () * 2.5f + (25 + crand () * 5),
			0,					                   0,				                       0,
			r,						               g,				                       b,
			0,					                   0,				                       0,
			PT_ENERGY,
			0.5f,					               -(0.4f + frand () * 0.2f), 
			3 + (1.5f * crand ()),				   5 + (2.5f * crand ()),
			0,
			0,
			frand () * 360,
			1,                                     0,
			NULL,								   NULL,								   NULL);
	}

	// Decal mark

	// Impact wave
}

/*
 ==================
 CL_ExplosionParticles
 ==================
*/
void CL_ExplosionParticles (vec3_t org, float scale, qboolean exploOnly, qboolean inWater)
{
	// Assume "origin" is origin of explosion
	// and "org" is the particle's origin
	vec3_t     sub, velocity, origin;
	vec3_t     endPos, normal;
	qboolean   decal;
	float      distScale, speed = 22.0f;
	int        i, j, flags = 0;

	// Decal mark
	decal = CL_FindExplosionDir (org, 30 * scale, endPos, normal);
	if (!decal)
		Vec3Set (normal, 0, 0, 1);

	// Upwards fire
	j = scale + 6;
	for (i = 1; i < j; i++)
	{
		CL_SpawnParticle (
			org[0] + (crand () * 9) + (normal[0] * 7),			   
			org[1] + (crand () * 9) + (normal[1] * 7),			   
			org[2] + (crand () * 9) + (normal[2] * 7),
			0,					                   0,				                       0,
			normal[0] * 290,					   normal[1] * 290,				           normal[2] * 290,
			0,					                   0,				                       0,
			1.0f,					               1.0f,				                   1.0f,
			0,					                   0,				                       0,
			PT_FIRE1 + (rand () & 2),
			0.95f,					               -6.0f,
			55 + (crand () * 9.5f),				   55 + (crand () * 9.5f),
			0,
			0,
			frand () * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;

	// Upwards smoke
	j = scale + 4;
	for (i = 1; i < j; i++)
	{
		distScale = (float)i/(float)j;

		CL_SpawnParticle (
			org[0] + (crand () * 9) + (normal[0] * 7 * distScale),			   
			org[1] + (crand () * 9) + (normal[1] * 7 * distScale),			   
			org[2] + (crand () * 9) + (normal[2] * 7 * distScale),
			0,					                   0,				                       0,
			normal[0] * 90 * distScale,		       normal[1] * 90 * distScale,			   normal[2] * 90 * distScale,
			0,					                   0,				                       0,
			0.2f,					               0.2f,				                   0.2f,
			0,					                   0,				                       0,
			PT_SMOKE1 + (rand () & 2),
			0.8f,					               -0.5f,
			65 + (crand () * 9.5f),				   35 + (crand () * 9.5f),
			flags,
			0,
			frand () * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}

	// Outwards smoke
	for (i = 0; i < 19; i++)
	{
		// Calcs for outwards smoke -- Thanks paril!
		Vec3Copy (org, origin);
		origin[0] += crand () * 3;
		origin[1] += crand () * 3;

		Vec3Subtract (origin, org, sub);
		for (j = 0; j < 3; j++)
			velocity[j] = sub[j] * -speed;

		CL_SpawnParticle (
			org[0],			                       org[1],			                       org[2],
			0,					                   0,				                       0,
			velocity[0],					       velocity[1],				               velocity[2],
			0,					                   0,				                       5 + (crand () * 2),
			0.2f,					               0.2f,				                   0.2f,
			0,					                   0,				                       0,
			PT_SMOKE1 + (rand () & 3),
			0.8f,					               -0.5f,
			35 + (crand () * 9.5f),				   25 + (crand () * 9.5f),
			flags,
			0.01f,
			frand () * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}
}

/*
 ==================
 CL_BulletParticles
 ==================
*/
void CL_BulletParticles (const vec3_t org, const vec3_t dir)
{
	int			flags;
	int			i, count;

	if (!cl_particles->integer)
		return;

	count = 10 + (rand() % 5);

	// Check for water
	if (CL_PointContents (org, -1) & MASK_WATER)
	{
		// Found it, so do a bubble trail
		CL_BubbleParticles (org, count, 0);
		return;
	}

	// Sparks
	flags = PARTICLE_STRETCH;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++)
	{
		CL_SpawnParticle (
			org[0] + dir[0] * 1 + crand (),		   org[1] + dir[1] * 1 + crand (),         org[2] + dir[2] * 1 + crand (),
			0,					                   0,				                       0,
			dir[0] * 180 + crand () * 60,		   dir[1] * 180 + crand () * 60,		   dir[2] * 180 + crand () * 60,
			0,					                   0,				                       -120 + (60 * crand ()),
			1,					                   1,				                       1,
			0,					                   0,				                       0,
			PT_SPARK,
			1.0f ,			                       -8.0f,
			1.2f + (1.9f * crand () / count),	   0,
			flags,
			0.2f,
			0,
			11 + (4 * crand () * 4),               11 + (4 * crand () * 4),
			NULL,						           NULL,								   NULL);
	}
}

/*
 ==================
 CL_BulletParticles
 ==================
*/
void CL_SmokePuffParticles (const vec3_t org, float radius, int count)
{
	int     flags;
	int		i;

	flags = 0;

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	// Smoke puff
	for (i = 0; i < count; i++)
	{
		CL_SpawnParticle (
			org[0] + crand (),		               org[1] + crand (),                      org[2] + crand (),
			0,					                   0,				                       0,
			crand() * 5,		                   crand() * 5,		                       crand() * 5 + (25 + crand() * 5),
			0,					                   0,				                       0,
			0.4,					               0.4,				                       0.4,
			0.75,					               0.75,				                   0.75,
			PT_SMOKE1 + (rand () & 2),
			0.5f ,			                       -(0.4 + frand() * 0.2),
			radius + ((radius * 0.5) * crand()),   (radius * 2) + ((radius * 4) * crand()),
			flags,
			0,
			frand() * 360,
			1,                                     0,
			pSmokeThink,						   NULL,								   NULL);
	}
}

// ===========================================================================

/*
 =================
 CL_RailTrail
 =================
*/
void CL_RailTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	int			flags;
	vec3_t		move, vec;
	float		len, dist;
	int			i;
	vec3_t		right, up, dir;
	float		d, s, c;

	if (!cl_particles->integer)
		return;

	// Core
	flags = 0;

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 2;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.smokeParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 1.5;
		p->vel[1] = crand() * 1.5;
		p->vel[2] = crand() * 1.5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -0.5 / (1.0 + frand() * 0.2);
		p->size = 1.5 + (0.5 * crand());
		p->sizeVel = 3 + (1.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}

	// Spiral
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = i * 0.1;
		s = sin(d);
		c = cos(d);

		dir[0] = right[0]*c + up[0]*s;
		dir[1] = right[1]*c + up[1]*s;
		dir[2] = right[2]*c + up[2]*s;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + dir[0] * 2.5;
		p->org[1] = move[1] + dir[1] * 2.5;
		p->org[2] = move[2] + dir[2] * 2.5;
		p->vel[0] = dir[0] * 5;
		p->vel[1] = dir[1] * 5;
		p->vel[2] = dir[2] * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.09;
		p->color[1] = 0.32;
		p->color[2] = 0.43;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0;
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_BFGTrail
 =================
*/
void CL_BFGTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec, org;
	float		len, dist, d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integer)
		return;

	// Particles
	time = cl.time * 0.001;

	org[0] = start[0] + (end[0] - start[0]) * cl.lerpFrac;
	org[1] = start[1] + (end[1] - start[1]) * cl.lerpFrac;
	org[2] = start[2] + (end[2] - start[2]) * cl.lerpFrac;
	
	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.24;
		p->color[1] = 0.82;
		p->color[2] = 0.10;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0 - Distance(org, p->org) / 90.0;
		p->alphaVel = 0;
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}

	if (CL_PointContents(end, -1) & MASK_WATER){
		CL_BubbleTrail(start, end, 75, 10);
		return;
	}

	// Smoke
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 75;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.liteSmokeParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->size = 25 + (5 * crand());
		p->sizeVel = 25 + (5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_HeatBeamTrail
 =================
*/
void CL_HeatBeamTrail (const vec3_t start, const vec3_t forward){

	cparticle_t	*p;
	vec3_t		move, vec, end;
	float		len, dist, step;
	vec3_t		dir;
	float		rot, s, c;

	if (!cl_particles->integer)
		return;

	len = VectorNormalize2(forward, vec);
	VectorMA(start, len, vec, end);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	VectorNormalize(vec);

	dist = fmod(cl.time * 0.096, 32);
	len -= dist;
	VectorMA(move, dist, vec, move);

	dist = 32;
	VectorScale(vec, dist, vec);

	step = M_PI * 0.1;

	while (len > 0){
		len -= dist;

		for (rot = 0; rot < M_PI2; rot += step){
			p = CL_AllocParticle();
			if (!p)
				return;

			s = sin(rot) * 0.5;
			c = cos(rot) * 0.5;

			dir[0] = cl.refDef.viewAxis[1][0] * c + cl.refDef.viewAxis[2][0] * s;
			dir[1] = cl.refDef.viewAxis[1][1] * c + cl.refDef.viewAxis[2][1] * s;
			dir[2] = cl.refDef.viewAxis[1][2] * c + cl.refDef.viewAxis[2][2] * s;

			p->shader = cl.media.energyParticleShader;
			p->time = cl.time;
			p->flags = PARTICLE_INSTANT;

			p->org[0] = move[0] + dir[0] * 3;
			p->org[1] = move[1] + dir[1] * 3;
			p->org[2] = move[2] + dir[2] * 3;
			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = 0;
			p->accel[0] = 0;
			p->accel[1] = 0;
			p->accel[2] = 0;
			p->color[0] = 0.97;
			p->color[1] = 0.46;
			p->color[2] = 0.14;
			p->colorVel[0] = 0;
			p->colorVel[1] = 0;
			p->colorVel[2] = 0;
			p->alpha = 0.5;
			p->alphaVel = 0;
			p->size = 1.5;
			p->sizeVel = 0;
			p->length = 1;
			p->lengthVel = 0;
			p->rotation = 0;
		}

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_TrackerTrail
 =================
*/
void CL_TrackerTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec;
	vec3_t		angles, forward, up;
	float		len, dist, c;

	if (!cl_particles->integer)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorToAngles(vec, angles);
	AngleVectors(angles, forward, NULL, up);

	dist = 2.5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		c = 8 * cos(DotProduct(move, forward));

		p->shader = cl.media.trackerParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + up[0] * c;
		p->org[1] = move[1] + up[1] * c;
		p->org[2] = move[2] + up[2] * c;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -2.0;
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_TagTrail
 =================
*/
void CL_TagTrail (const vec3_t start, const vec3_t end){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integer)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 16;
		p->org[1] = move[1] + crand() * 16;
		p->org[2] = move[2] + crand() * 16;
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.00;
		p->color[1] = 1.00;
		p->color[2] = 0.15;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.8 + frand() * 0.2);
		p->size = 1.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_FlagTrail
 =================
*/
void CL_FlagTrail (const vec3_t start, const vec3_t end, float r, float g, float b){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integer)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.glowParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 16;
		p->org[1] = move[1] + crand() * 16;
		p->org[2] = move[2] + crand() * 16;
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.8 + frand() * 0.2);
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_BFGExplosionParticles
 =================
*/
void CL_BFGExplosionParticles (const vec3_t org){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	// Particles
	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + ((rand() % 32) - 16);
		p->org[1] = org[1] + ((rand() % 32) - 16);
		p->org[2] = org[2] + ((rand() % 32) - 16);
		p->vel[0] = (rand() % 384) - 192;
		p->vel[1] = (rand() % 384) - 192;
		p->vel[2] = (rand() % 384) - 192;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (10 * crand());
		p->color[0] = 0.24;
		p->color[1] = 0.82;
		p->color[2] = 0.10;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.8 / (0.5 + frand() * 0.3);
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;

		VectorCopy(p->org, p->lastOrg);
	}

	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	// Smoke
	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.liteSmokeParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 10;
		p->org[1] = org[1] + crand() * 10;
		p->org[2] = org[2] + crand() * 10;
		p->vel[0] = crand() * 10;
		p->vel[1] = crand() * 10;
		p->vel[2] = crand() * 10 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.1 + frand() * 0.1);
		p->size = 50 + (25 * crand());
		p->sizeVel = 15 + (7.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_TrackerExplosionParticles
 =================
*/
void CL_TrackerExplosionParticles (const vec3_t org){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < 384; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.trackerParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + ((rand() % 32) - 16);
		p->org[1] = org[1] + ((rand() % 32) - 16);
		p->org[2] = org[2] + ((rand() % 32) - 16);
		p->vel[0] = (rand() % 256) - 128;
		p->vel[1] = (rand() % 256) - 128;
		p->vel[2] = (rand() % 256) - 128;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (10 * crand());
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.4 / (0.6 + frand() * 0.2);
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;

		VectorCopy(p->org, p->lastOrg);
	}
}

/*
 =================
 CL_BubbleParticles
 =================
*/
void CL_BubbleParticles (const vec3_t org, int count, float magnitude){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.bubbleParticleShader;
		p->time = cl.time;
		p->flags = PARTICLE_UNDERWATER;

		p->org[0] = org[0] + (magnitude * crand());
		p->org[1] = org[1] + (magnitude * crand());
		p->org[2] = org[2] + (magnitude * crand());
		p->vel[0] = crand() * 5;
		p->vel[1] = crand() * 5;
		p->vel[2] = crand() * 5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1;
		p->color[1] = 1;
		p->color[2] = 1;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->size = 1 + (0.5 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_SparkParticles
 =================
*/
void CL_SparkParticles (const vec3_t org, const vec3_t dir, int count){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 0);
		return;
	}

	// Sparks
	flags = PARTICLE_STRETCH;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.sparkParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = dir[0] * 180 + crand() * 60;
		p->vel[1] = dir[1] * 180 + crand() * 60;
		p->vel[2] = dir[2] * 180 + crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (60 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.5;
		p->size = 0.4 + (0.2 * crand());
		p->sizeVel = 0;
		p->length = 8 + (4 * crand());
		p->lengthVel = 8 + (4 * crand());
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}

	// Smoke
	flags = 0;

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 3; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.smokeParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 5 + crand();
		p->org[1] = org[1] + dir[1] * 5 + crand();
		p->org[2] = org[2] + dir[2] * 5 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.4;
		p->color[1] = 0.4;
		p->color[2] = 0.4;
		p->colorVel[0] = 0.2;
		p->colorVel[1] = 0.2;
		p->colorVel[2] = 0.2;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->size = 3 + (1.5 * crand());
		p->sizeVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_DamageSparkParticles
 =================
*/
void CL_DamageSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i, index;

	if (!cl_particles->integer)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 2.5);
		return;
	}

	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		index = (color + (rand() & 7)) & 0xff;

		p->shader = cl.media.impactSparkParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = crand() * 60;
		p->vel[1] = crand() * 60;
		p->vel[2] = crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -180 + (30 * crand());
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -(0.75 + frand());
		p->size = 0.4 + (0.2 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.6;
	}
}

/*
 =================
 CL_LaserSparkParticles
 =================
*/
void CL_LaserSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER){
		CL_BubbleParticles(org, count, 2.5);
		return;
	}

	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	color &= 0xff;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.impactSparkParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = dir[0] * 180 + crand() * 60;
		p->vel[1] = dir[1] * 180 + crand() * 60;
		p->vel[2] = dir[2] * 180 + crand() * 60;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -120 + (60 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.5;
		p->size = 0.4 + (0.2 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.2;

		VectorCopy(p->org, p->lastOrg);
	}
}

/*
 =================
 CL_SplashParticles
 =================
*/
void CL_SplashParticles (const vec3_t org, const vec3_t dir, int count, float magnitude, float spread){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.dropletParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 3 + crand() * magnitude;
		p->org[1] = org[1] + dir[1] * 3 + crand() * magnitude;
		p->org[2] = org[2] + dir[2] * 3 + crand() * magnitude;
		p->vel[0] = dir[0] * spread + crand() * spread;
		p->vel[1] = dir[1] * spread + crand() * spread;
		p->vel[2] = dir[2] * spread + crand() * spread + 4 * spread;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -150 + (25 * crand());
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.25 + frand() * 0.25);
		p->size = 0.5 + (0.25 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_LavaSteamParticles
 =================
*/
void CL_LavaSteamParticles (const vec3_t org, const vec3_t dir, int count){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.steamParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + dir[0] * 2 + crand();
		p->org[1] = org[1] + dir[1] * 2 + crand();
		p->org[2] = org[2] + dir[2] * 2 + crand();
		p->vel[0] = crand() * 2.5;
		p->vel[1] = crand() * 2.5;
		p->vel[2] = crand() * 2.5 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.82;
		p->color[1] = 0.34;
		p->color[2] = 0.00;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 0.5;
		p->alphaVel = -(0.4 + frand() * 0.2);
		p->size = 1.5 + (0.75 * crand());
		p->sizeVel = 5 + (2.5 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_FlyParticles
 =================
*/
void CL_FlyParticles (const vec3_t org, int count){

	cparticle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integer)
		return;

	if (CL_PointContents(org, -1) & MASK_WATER)
		return;

	time = cl.time * 0.001;

	for (i = 0; i < count ; i += 2){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->shader = cl.media.flyParticleShader;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 1.0;
		p->color[1] = 1.0;
		p->color[2] = 1.0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = 0;
		p->size = 1.0;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TeleportParticles
 =================
*/
void CL_TeleportParticles (const vec3_t org){

	cparticle_t	*p;
	vec3_t		dir;
	float		vel, color;
	int			x, y, z;

	if (!cl_particles->integer)
		return;

	for (x = -16; x <= 16; x += 4){
		for (y = -16; y <= 16; y += 4){
			for (z = -16; z <= 32; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y*8, x*8, z*8);
				VectorNormalizeFast(dir);

				vel = 50 + (rand() & 63);

				color = 0.1 + (0.2 * frand());

				p->shader = cl.media.glowParticleShader;
				p->time = cl.time;
				p->flags = 0;

				p->org[0] = org[0] + x + (rand() & 3);
				p->org[1] = org[1] + y + (rand() & 3);
				p->org[2] = org[2] + z + (rand() & 3);
				p->vel[0] = dir[0] * vel;
				p->vel[1] = dir[1] * vel;
				p->vel[2] = dir[2] * vel;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = -40;
				p->color[0] = color;
				p->color[1] = color;
				p->color[2] = color;
				p->colorVel[0] = 0;
				p->colorVel[1] = 0;
				p->colorVel[2] = 0;
				p->alpha = 1.0;
				p->alphaVel = -1.0 / (0.3 + (rand() & 7) * 0.02);
				p->size = 2;
				p->sizeVel = 0;
				p->length = 1;
				p->lengthVel = 0;
				p->rotation = 0;
			}
		}
	}
}

/*
 =================
 CL_BigTeleportParticles
 =================
*/
void CL_BigTeleportParticles (const vec3_t org){

	cparticle_t	*p;
	float		d, angle, s, c, color;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < 4096; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		d = rand() & 31;

		angle = M_PI2 * (rand() & 1023) / 1023.0;
		s = sin(angle);
		c = cos(angle);

		color = 0.1 + (0.2 * frand());

		p->shader = cl.media.glowParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + c * d;
		p->org[1] = org[1] + s * d;
		p->org[2] = org[2] + 8 + (rand() % 90);
		p->vel[0] = c * (70 + (rand() & 63));
		p->vel[1] = s * (70 + (rand() & 63));
		p->vel[2] = -100 + (rand() & 31);
		p->accel[0] = -100 * c;
		p->accel[1] = -100 * s;
		p->accel[2] = 160;
		p->color[0] = color;
		p->color[1] = color;
		p->color[2] = color;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.3 / (0.5 + frand() * 0.3);
		p->size = 2;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TeleporterParticles
 =================
*/
void CL_TeleporterParticles (const vec3_t org){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < 8; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] - 16 + (rand() & 31);
		p->org[1] = org[1] - 16 + (rand() & 31);
		p->org[2] = org[2] - 8 + (rand() & 7);
		p->vel[0] = crand() * 14;
		p->vel[1] = crand() * 14;
		p->vel[2] = 80 + (rand() & 7);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40;
		p->color[0] = 0.97;
		p->color[1] = 0.46;
		p->color[2] = 0.14;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -0.5;
		p->size = 3 + (1.5 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TrapParticles
 =================
*/
void CL_TrapParticles (const vec3_t org){

	cparticle_t	*p;
	vec3_t		start, end, move, vec, dir;
	float		len, dist, vel;
	int			x, y, z;

	if (!cl_particles->integer)
		return;

	VectorSet(start, org[0], org[1], org[2] + 18);
	VectorSet(end, org[0], org[1], org[2] + 82);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 5;
	VectorScale(vec, dist, vec);

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand();
		p->org[1] = move[1] + crand();
		p->org[2] = move[2] + crand();
		p->vel[0] = 15 * crand();
		p->vel[1] = 15 * crand();
		p->vel[2] = 15 * crand();
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 40;
		p->color[0] = 0.97;
		p->color[1] = 0.46;
		p->color[2] = 0.14;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.3 + frand() * 0.2);
		p->size = 3 + (1.5 * crand());
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}

	for (x = -2; x <= 2; x += 4){
		for (y = -2; y <= 2; y += 4){
			for (z = -2; z <= 4; z += 4){
				p = CL_AllocParticle();
				if (!p)
					return;

				VectorSet(dir, y*8, x*8, z*8);
				VectorNormalizeFast(dir);

				vel = 50 + (rand() & 63);

				p->shader = cl.media.energyParticleShader;
				p->time = cl.time;
				p->flags = 0;

				p->org[0] = org[0] + x + ((rand() & 23) * crand());
				p->org[1] = org[1] + y + ((rand() & 23) * crand());
				p->org[2] = org[2] + z + ((rand() & 23) * crand()) + 32;
				p->vel[0] = dir[0] * vel;
				p->vel[1] = dir[1] * vel;
				p->vel[2] = dir[2] * vel;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = -40;
				p->color[0] = 0.96;
				p->color[1] = 0.46;
				p->color[2] = 0.14;
				p->colorVel[0] = 0;
				p->colorVel[1] = 0;
				p->colorVel[2] = 0;
				p->alpha = 1.0;
				p->alphaVel = -1.0 / (0.3 + (rand() & 7) * 0.02);
				p->size = 3 + (1.5 * crand());
				p->sizeVel = 0;
				p->length = 1;
				p->lengthVel = 0;
				p->rotation = 0;
			}
		}
	}
}

/*
 =================
 CL_LogParticles
 =================
*/
void CL_LogParticles (const vec3_t org, float r, float g, float b){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < 512; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.glowParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 20;
		p->org[1] = org[1] + crand() * 20;
		p->org[2] = org[2] + crand() * 20;
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = r;
		p->color[1] = g;
		p->color[2] = b;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (1.0 + frand() * 0.3);
		p->size = 2;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_ItemRespawnParticles
 =================
*/
void CL_ItemRespawnParticles (const vec3_t org){

	cparticle_t	*p;
	int			i;

	if (!cl_particles->integer)
		return;

	for (i = 0; i < 64; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.glowParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + crand() * 8;
		p->org[1] = org[1] + crand() * 8;
		p->org[2] = org[2] + crand() * 8;
		p->vel[0] = crand() * 8;
		p->vel[1] = crand() * 8;
		p->vel[2] = crand() * 8;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0.0;
		p->color[1] = 0.3;
		p->color[2] = 0.5;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (1.0 + frand() * 0.3);
		p->size = 2;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_TrackerShellParticles
 =================
*/
void CL_TrackerShellParticles (const vec3_t org){

	cparticle_t	*p;
	vec3_t		vec;
	float		d, time;
	float		angle, sy, cy, sp, cp;
	int			i;

	if (!cl_particles->integer)
		return;

	time = cl.time * 0.001;

	for (i = 0; i < NUM_VERTEX_NORMALS; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		angle = time * cl_particleVelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = time * cl_particleVelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);

		vec[0] = cp*cy;
		vec[1] = cp*sy;
		vec[2] = -sp;

		d = sin(time + i) * 64.0;

		p->shader = cl.media.trackerParticleShader;
		p->time = cl.time;
		p->flags = PARTICLE_INSTANT;

		p->org[0] = org[0] + byteDirs[i][0]*d + vec[0]*16;
		p->org[1] = org[1] + byteDirs[i][1]*d + vec[1]*16;
		p->org[2] = org[2] + byteDirs[i][2]*d + vec[2]*16;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = 0;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = 0;
		p->size = 2.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
	}
}

/*
 =================
 CL_NukeSmokeParticles
 =================
*/
void CL_NukeSmokeParticles (const vec3_t org){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	flags = 0;

	if (cl_particleVertexLight->integer)
		flags |= PARTICLE_VERTEXLIGHT;

	for (i = 0; i < 5; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.smokeParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + crand() * 20;
		p->org[1] = org[1] + crand() * 20;
		p->org[2] = org[2] + crand() * 20;
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20 + (25 + crand() * 5);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = 0;
		p->color[1] = 0;
		p->color[2] = 0;
		p->colorVel[0] = 0.75;
		p->colorVel[1] = 0.75;
		p->colorVel[2] = 0.75;
		p->alpha = 0.5;
		p->alphaVel = -(0.1 + frand() * 0.1);
		p->size = 60 + (30 * crand());
		p->sizeVel = 30 + (15 * crand());
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}

/*
 =================
 CL_WeldingSparkParticles
 =================
*/
void CL_WeldingSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	color &= 0xff;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 5 + (5 * crand());
		p->org[1] = org[1] + dir[1] * 5 + (5 * crand());
		p->org[2] = org[2] + dir[2] * 5 + (5 * crand());
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -40 + (5 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->size = 1.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;
	}
}

/*
 =================
 CL_TunnelSparkParticles
 =================
*/
void CL_TunnelSparkParticles (const vec3_t org, const vec3_t dir, int count, int color){

	cparticle_t	*p;
	int			flags;
	int			i;

	if (!cl_particles->integer)
		return;

	flags = 0;

	if (cl_particleBounce->integer)
		flags |= PARTICLE_BOUNCE;
	if (cl_particleFriction->integer)
		flags |= PARTICLE_FRICTION;

	color &= 0xff;

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = flags;

		p->org[0] = org[0] + dir[0] * 5 + (5 * crand());
		p->org[1] = org[1] + dir[1] * 5 + (5 * crand());
		p->org[2] = org[2] + dir[2] * 5 + (5 * crand());
		p->vel[0] = crand() * 20;
		p->vel[1] = crand() * 20;
		p->vel[2] = crand() * 20;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 40 + (5 * crand());
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->size = 1.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;
		p->bounceFactor = 0.7;
	}
}

/*
 =================
 CL_ForceWallParticles
 =================
*/
void CL_ForceWallParticles (const vec3_t start, const vec3_t end, int color){

	cparticle_t	*p;
	vec3_t		move, vec;
	float		len, dist;

	if (!cl_particles->integer)
		return;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dist = 4;
	VectorScale(vec, dist, vec);

	color &= 0xff;

	while (len > 0){
		len -= dist;

		p = CL_AllocParticle();
		if (!p)
			return;

		p->shader = cl.media.energyParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = move[0] + crand() * 3;
		p->org[1] = move[1] + crand() * 3;
		p->org[2] = move[2] + crand() * 3;
		p->vel[0] = 0;
		p->vel[1] = 0;
		p->vel[2] = -40 - (crand() * 10);
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = 0;
		p->color[0] = cl_particlePalette[color][0];
		p->color[1] = cl_particlePalette[color][1];
		p->color[2] = cl_particlePalette[color][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (3.0 + frand() * 0.5);
		p->size = 1.5;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = 0;

		VectorAdd(move, vec, move);
	}
}

/*
 =================
 CL_SteamParticles
 =================
*/
void CL_SteamParticles (const vec3_t org, const vec3_t dir, int count, int color, float magnitude){

	cparticle_t	*p;
	vec3_t		r, u;
	float		rd, ud;
	int			index;
	int			i;

	if (!cl_particles->integer)
		return;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++){
		p = CL_AllocParticle();
		if (!p)
			return;

		rd = crand() * magnitude / 3;
		ud = crand() * magnitude / 3;

		index = (color + (rand() & 7)) & 0xff;

		p->shader = cl.media.steamParticleShader;
		p->time = cl.time;
		p->flags = 0;

		p->org[0] = org[0] + magnitude * 0.1 * crand();
		p->org[1] = org[1] + magnitude * 0.1 * crand();
		p->org[2] = org[2] + magnitude * 0.1 * crand();
		p->vel[0] = dir[0] * magnitude + r[0] * rd + u[0] * ud;
		p->vel[1] = dir[1] * magnitude + r[1] * rd + u[1] * ud;
		p->vel[2] = dir[2] * magnitude + r[2] * rd + u[2] * ud;
		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -20;
		p->color[0] = cl_particlePalette[index][0];
		p->color[1] = cl_particlePalette[index][1];
		p->color[2] = cl_particlePalette[index][2];
		p->colorVel[0] = 0;
		p->colorVel[1] = 0;
		p->colorVel[2] = 0;
		p->alpha = 1.0;
		p->alphaVel = -1.0 / (0.5 + frand() * 0.3);
		p->size = 2;
		p->sizeVel = 0;
		p->length = 1;
		p->lengthVel = 0;
		p->rotation = rand() % 360;
	}
}
