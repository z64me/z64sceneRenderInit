#include <z64ovl/oot/debug.h>
#include <z64ovl/oot/helpers.h>
#include <z64ovl/z64_functions.h>

#include "types.h"

// TODO eliminate Z64GL_IS_NIGHT and Z64GL_SAVE_CONTEXT;
//      these should be derived from the z64_global_t

/* global variables contained within */
static struct
{
	/* last generated color key */
	struct colorkey Pcolorkey;
} g;

/* propagate ram segment with pointer to data */
static
inline
void
segment(z64_global_t *gl, int seg, void *data)
{
	z64_gfx_t *gfx = gl->common.gfx_ctxt;
	
	gSPSegment(gfx->poly_opa.p++, seg, data);
	gSPSegment(gfx->poly_xlu.p++, seg, data);
	// TODO is `overlay` also needed, or does
	//      the above already work for decals?
}

/* returns 1 = flag active; 0 = flag inactive */
static
int
flag(z64_global_t *gl, struct flag *f)
{
	int r = 0;
	
	switch (f->type)
	{
		case FLAG_TYPE_ROOMCLEAR:
			r = flag_get_roomclear(gl, f->flag);
			break;
		
		case FLAG_TYPE_TREASURE:
			r = flag_get_treasure(gl, f->flag);
			break;
		
		case FLAG_TYPE_USCENE:
			r = flag_get_uscene(gl, f->flag);
			break;
		
		case FLAG_TYPE_TEMP:
			r = temp_clear_flag_get(gl, f->flag);
			break;
		
		case FLAG_TYPE_SCENECOLLECT:
			r = flag_get_scenecollect(gl, f->flag);
			break;
		
		case FLAG_TYPE_SWITCH:
			r = flag_get_switch(gl, f->flag);
			break;
		
		case FLAG_TYPE_EVENTCHKINF:
			r = flag_get_event_chk_inf(f->flag);
			break;
		
		case FLAG_TYPE_INFTABLE:
			r = flag_get_inf_table(f->flag);
			break;
		
		case FLAG_TYPE_IS_NIGHT:
			r = (*(uint32_t*)(Z64GL_IS_NIGHT));
			break;
		
		case FLAG_TYPE_SAVE:
			r = !!(
				(*(uint32_t*)((uint8_t*)(Z64GL_SAVE_CONTEXT) + f->flag))
				& f->and
			);
			break;
		
		case FLAG_TYPE_GLOBAL:
			r = !!(
				(*(uint32_t*)((uint8_t*)(gl) + f->flag)) & f->and
			);
			break;
		
		case FLAG_TYPE_RAM:
			r = !!((*(uint32_t*)(f->flag)) & f->and);
			break;
	}
	
	return f->eq == r;
}

/* given a color list header and gameplay frame, *
 * get the keyframe within the color list        */
static
struct colorkey *
get_colorkey(struct colorlist *list, uint32_t *frame)
{
	struct colorkey *key;
	uint32_t along = 0;
	
	if (!list->dur)
		return list->key;
	
	/* loop */
	*frame %= list->dur;
	
	/* locate appropriate keyframe */
	for (key = list->key; key->next; ++key)
	{
		uint32_t next = along + key->next;
		
		/* keyframe range encompasses frame */
		if (*frame >= along && *frame <= next)
			return key;
		
		/* advance to next frame range */
		along = next;
	}
	
	/* found none; return first keyframe */
	return list->key;
}

/* get next key in list */
static
struct colorkey *
get_colorkey_next(struct colorlist *list, struct colorkey *key)
{
	/* next == 0 indicates end of list */
	if (!key->next)
		return key;
	
	/* advance to next key */
	key += 1;
	
	/* key->next == 0 indicates end of list, so loop to first key */
	if (!key->next)
		return list->key;
	
	/* return next key */
	return key;
}

static
inline
int
ease_int(int from, int to, float factor)
{
	return from + (to - from) * factor;
}

static
inline
int
ease_8(int from, int to, float factor)
{
	return ease_int(from, to, factor) & 0xFF;
}

static
inline
uint32_t
ease_rgba(uint32_t from, uint32_t to, float factor)
{
	uint8_t rgba[4];
	uint8_t *from8 = (uint8_t*)&from;
	uint8_t *to8 = (uint8_t*)&to;
	
	rgba[0] = ease_int(from8[0], to8[0], factor);
	rgba[1] = ease_int(from8[1], to8[1], factor);
	rgba[2] = ease_int(from8[2], to8[2], factor);
	rgba[3] = ease_int(from8[3], to8[3], factor);
	
	return *(uint32_t*)rgba;
}

static
inline
void
colorkey_put(uint8_t which, Gfx **work, struct colorkey *key)
{
	if (which & COLORKEY_PRIM)
	{
		uint8_t *rgba = (uint8_t*)&key->prim;
		
		gDPSetPrimColor(
			(*work)++
			, key->mlevel
			, key->lfrac
			, rgba[0]
			, rgba[1]
			, rgba[2]
			, rgba[3]
		);
	}
	
	if (which & COLORKEY_ENV)
	{
		uint8_t *rgba = (uint8_t*)&key->env;
		
		gDPSetEnvColor(
			(*work)++
			, rgba[0]
			, rgba[1]
			, rgba[2]
			, rgba[3]
		);
	}
}

static
inline
void
colorkey_blend(
	enum8(colorkey_types) which
	, float factor
	, struct colorkey *from
	, struct colorkey *to
	, struct colorkey *result
)
{
	if (from == to)
	{
		*result = *from;
		return;
	}
	
	/* primcolor */
	if (which & COLORKEY_PRIM)
	{
		result->prim = ease_rgba(from->prim, to->prim, factor);
		
		if (which & COLORKEY_LODFRAC)
			result->lfrac = ease_8(from->lfrac, to->lfrac, factor);
		
		if (which & COLORKEY_MINLEVEL)
			result->mlevel = ease_8(from->mlevel, to->mlevel, factor);
	}
	
	/* envcolor */
	if (which & COLORKEY_ENV)
	{
		result->env = ease_rgba(from->env, to->env, factor);
	}
}

static
float
interp(uint32_t frame, uint32_t next, enum8(ease) ease)
{
	float factor;
	
	if (!next)
		return 0;
	
	factor = frame;
	
	/* normalize factor */
	factor /= next;
	
	// TODO transform factor with easing transformations
#define M_PI 3.14579
	if (ease != EASE_LINEAR)
	{
		switch (ease)
		{
			/* sinusoidal (in) */
			case EASE_SIN_IN:
				break;
			
			/* sinusoidal (out) */
			case EASE_SIN_OUT:
				break;
			
			/* unsupported: use linear */
			default:
				break;
		}
	}
	
	return factor;
}

static
inline
void
color_loop(z64_global_t *gl, Gfx **work, struct colorlist *list)
{
	struct colorkey *from;
	struct colorkey *to;
	uint32_t frame = gl->gameplay_frames;
	float factor;
	
	from = get_colorkey(list, &frame);
	to = get_colorkey_next(list, from);
	
	factor = interp(frame, from->next, list->ease);
	
	/* blend color keys (result goes into Pcolorkey) */
	colorkey_blend(list->which, factor, from, to, &g.Pcolorkey);
	
	colorkey_put(list->which, work, &g.Pcolorkey);
}

static
inline
void
color_loop_flag(z64_global_t *gl, Gfx **work, struct colorlist_flag *c)
{
	struct flag *f = &c->flag;
	struct colorlist *list = &c->list;
	
	struct colorkey *key;
	struct colorkey  Nkey;
	int active = flag(gl, f);
	int xfading = (f->frames || active) && f->frames <= f->xfade && f->xfade;
	
	enum8(colorkey_types) which = list->which;
	
	//if (flag(gl, &scroll->flag))
	//	scroll->flag.frames++;
	
	/* if not cross fading, write to Pcolorkey */
	if (!xfading)
		key = &g.Pcolorkey;
	else
		key = &Nkey;
	
	/* not across-fading, and flag is not active */
	if (!active && !f->frames && !f->freeze)
		return;
	
	/* if cross fading or flag is active, compute colors */
	if (active || xfading)
	{
		struct colorkey *from;
		struct colorkey *to;
		uint32_t frame = gl->gameplay_frames;
		float factor;
		
		from = get_colorkey(list, &frame);
		to = get_colorkey_next(list, from);
		
		factor = interp(frame, from->next, list->ease);
		
		/* blend color keys (result goes into key) */
		colorkey_blend(which, factor, from, to, key);
	}
	
	/* if cross fading, interpolate between old and new colors */
	if (xfading)
	{
		float factor;
		
		if (active)
			f->frames += f->frames < f->xfade;
		else
			f->frames -= f->frames > 0;
		
		factor = interp(f->frames, f->xfade, list->ease);
		
		/* blend color keys (result goes into Pcolorkey) */
		colorkey_blend(which, factor, &g.Pcolorkey, key, &g.Pcolorkey);
		key = &g.Pcolorkey;
	}
	
	colorkey_put(which, work, key);
}

static
inline
int
abs_int(int x)
{
	return (x < 0) ? -x : x;
}

/* TODO zh_seg2ram should do the same thing; test it later */
static
inline
void *
mkabs(z64_global_t *gl, void *ptr)
{
	unsigned char *scene = gl->scene_file;
	uint32_t ptr32 = (uint32_t)ptr;
	
	if (!ptr)
		return 0;
	
	scene += ptr32 & 0xFFFFFF;
	
	return scene;
}

/* change pointer based on flag */
static
void
pointer_flag(z64_global_t *gl, Gfx **work, struct pointer_flag *ptr)
{
	// TODO don't forget to uncomment this
	*work = (void*)zh_seg2ram(ptr->ptr[flag(gl, &ptr->flag)]);
	// testing:
	//*work = (void*)zh_seg2ram(ptr->ptr[0]);
}

/* change pointer as time progresses (each pointer has its own time) */
static
void
pointer_timeloop(z64_global_t *gl, Gfx **work, struct pointer_timeloop *ptr)
{
	int item;
	int num = ptr->num;
	uint32_t *list = (void*)(ptr->each + num + !(num & 1));
	
	/* walk list */
	for (item = ptr->prev; item < num; ++item)
		if (ptr->time >= ptr->each[item])
			break;
	
	/* reached end of animation; roll back to beginning */
	if (item >= num - 1)
		item = ptr->prev = ptr->time = 0;
	
	*work = (void*)zh_seg2ram(list[item]);
	
	/* increment time elapsed */
	ptr->time += 1;
	if (ptr->time == ptr->each[item+1])
		ptr->prev += 1;
}

/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static
int
pointer_timeloop_flag(z64_global_t *gl, Gfx **work, struct pointer_timeloop_flag *_ptr)
{
	struct pointer_timeloop *ptr = &_ptr->list;
	if (!flag(gl, &_ptr->flag))
		return 0;
	
	pointer_timeloop(gl, work, ptr);
	return 1;
}

/* change pointer as time progresses */
static
void
pointer_loop(z64_global_t *gl, Gfx **work, struct pointer_loop *ptr)
{
	int item;
	
	/* overflow test */
	if (ptr->time >= ptr->dur)
		ptr->time = 0;
	
	item = ptr->time / ptr->each;
	*work = (void*)zh_seg2ram(ptr->ptr[item]);
	
	/* increment time elapsed */
	ptr->time += 1;
}

/* displays matrix hex values onto the screen */
static void print_matrix(z64_global_t *gl, void *_mtx)
{
	uint32_t *mtx = _mtx;
	int i;
	
	zh_text_init(gl, 0xFFFFFFFF, 1, 2);
	
	mtx += 8;
	
	for (i=0; i < 8; i+=2)
	{
		zh_text_draw("%08X %08X", mtx[i], mtx[i+1]);
	}
	
	zh_text_done();
}

/* change pointer as time progresses */
/* skipped if flag is undesirable */
static
int
pointer_loop_flag(z64_global_t *gl, Gfx **work, struct pointer_loop_flag *_ptr)
{
	struct pointer_loop *ptr = &_ptr->list;
	u8 flagstate = flag(gl, &_ptr->flag);
	struct flag *f = &_ptr->flag;

	if (!flagstate && f->freeze == 0)
		return 0;
	
	pointer_loop(gl, work, ptr);


	// if freeze mode is set, time doesnt advance when flag is not set
	if (!flagstate && f->freeze == 1)
		ptr->time -= 1;
	

	return 1;
}

/* scroll one tile layer */
// TODO does MM really scroll only one layer using this?
static
inline
void
scroll(z64_global_t *gl, Gfx **work, struct scroll *sc)
{
	uint32_t frame = gl->gameplay_frames;
	
	gDPSetTileSize(
		(*work)++
		, 0,  sc->u * frame,  sc->v * frame,  sc->w,  sc->h
	);
}

/* scroll two tile layers */
static
inline
void
scroll_two(z64_global_t *gl, Gfx **work, struct scroll *sc)
{
	struct scroll *sc1 = sc + 1;
	uint32_t frame = gl->gameplay_frames;
	
	gDPSetTileSize(
		(*work)++
		, 0,  sc->u * frame,  sc->v * frame,  sc->w,  sc->h
	);
	
	gDPSetTileSize(
		(*work)++
		, 1, sc1->u * frame, sc1->v * frame, sc1->w, sc1->h
	);
}

/* scroll tiles based on flag */
static
inline
void
scroll_flag(z64_global_t *gl, Gfx **work, struct scroll_flag *scroll)
{
	struct scroll *sc = scroll->sc;
	struct scroll *sc1 = sc + 1;
	uint16_t frame = scroll->flag.frames;
	
	if (flag(gl, &scroll->flag))
		scroll->flag.frames++;
	
	gDPSetTileSize(
		(*work)++
		, 0,  sc->u * frame,  sc->v * frame,  sc->w,  sc->h
	);
	
	gDPSetTileSize(
		(*work)++
		, 1, sc1->u * frame, sc1->v * frame, sc1->w, sc1->h
	);
}


extern void external_func_800AA76C(void* view, f32 arg1, f32 arg2, f32 arg3);
		asm("external_func_800AA76C = 0x800AA76C");

extern void external_func_800AA78C(void* view, f32 arg1, f32 arg2, f32 arg3);
		asm("external_func_800AA78C = 0x800AA78C");

extern void external_func_800AA7AC(void* view, f32 arg1);
		asm("external_func_800AA7AC = 0x800AA7AC");

extern s32 FrameAdvance_IsEnabled(z64_global_t *gl);
		asm("FrameAdvance_IsEnabled = 0x800C0D28");

extern void external_func_8009BEEC(z64_global_t *gl);
		asm("external_func_8009BEEC = 0x8009BEEC");


/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static
int
cameraeffect(z64_global_t *gl, Gfx **work, struct cameraeffect *cam) //TODO
{
	u8 cameratype = cam->cameratype;
	if (!flag(gl, &cam->flag))
		return 0;

	if (cameratype == 0)
	{
		external_func_8009BEEC(gl); //camera shake
	}
	else //jabu jabu
	{
		static s16 D_8012A39C = 538;
    	static s16 D_8012A3A0 = 4272;

    	f32 temp;


    	if (FrameAdvance_IsEnabled(gl) != true) {
        D_8012A39C += 1820;
        D_8012A3A0 += 1820;

        temp = 0.020000001f;
        external_func_800AA76C(&gl->view, ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_Coss(D_8012A39C),
                      ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_Sins(D_8012A39C),
                      ((360.00018f / 65535.0f) * (M_PI / 180.0f)) * temp * Math_Sins(D_8012A3A0));
        external_func_800AA78C(&gl->view, 1.f + (0.79999995f * temp * Math_Sins(D_8012A3A0)),
                      1.f + (0.39999998f * temp * Math_Coss(D_8012A3A0)), 1.f + (1 * temp * Math_Coss(D_8012A39C)));
        external_func_800AA7AC(&gl->view, 0.95f); 



     
    }


	}
	

	return 1;
}

/* change pointer as time progresses (each pointer has its own time) */
/* skipped if flag is undesirable */
static
int
conditionaldraw(z64_global_t *gl, Gfx **work, struct conditionaldraw *_ptr, int seg)
{
	Matrix_Push();
	if (!flag(gl, &_ptr->flag))
	{
		Matrix_Scale(0.0f, 0.0f, 0.0f, MTXMOD_NEW);
	}
	else
	{
		Matrix_Scale(1.0f, 1.0f, 1.0f, MTXMOD_NEW);
	}

	gSPSegment((*work)++, seg, Matrix_NewMtx(gl->common.gfx_ctxt, 0));

	matrix_pop();



	return 1;
}



/* returns pointer to raw scene header data */
static
void *
zh_get_current_scene_header(z64_global_t *global)
{
	z64_save_context_t *saveCtx = (void*)Z64GL_SAVE_CONTEXT;
	uint32_t *header = global->scene_file;
	uint32_t setup = saveCtx->scene_setup_index;
	
	if (setup && *header == 0x18000000)
	{
		uint32_t *list = (void*)zh_seg2ram(header[1]);
		
		/* the correct way */
		for (int i = setup - 1; i >= 0; --i)
			if (list[i])
				return (void*)zh_seg2ram(list[i]);
		
		/* the wrong but faster way */
//		if (list[i])
//			header = zh_seg2ram(list[i]);
	}
	
	return header;
}

static
inline
struct anim *
get_0x1A(void *_scene)
{
	unsigned char *scene = _scene;
	unsigned char *header = scene;
	
	if (!scene)
		return 0;
	
	/* while current header command is not end command */
	while (*header != 0x14)
	{
		/* animated texture list */
		if (*header == 0x1A)
			return (struct anim*)(
				scene + (*((uint32_t*)(header + 4)) & 0xFFFFFF)
			);
		
		/* advance to next header command */
		header += 8;
	}
	
	/* failed to locate 0x1A command */
	return 0;
}

/* fill existing display list with zeroes */
static
inline
void
unused_dl(Gfx **work)
{
	gDPNoOp((*work)++);
	gSPEndDisplayList((*work)++);
}

/* fill a segment with zeroes */
static
inline
void
unused_segment(z64_global_t *gl, int seg)
{
	z64_gfx_t *gfx = gl->common.gfx_ctxt;
	Gfx *buf;
	Gfx *Obuf;
	
	buf = Obuf = graph_alloc(gfx, 16);
	
	unused_dl(&buf);
	
	segment(gl, seg, Obuf);
}



void
main(z64_global_t *gl)
{
	/* persistent storage */
	static struct anim *list = 0;
	static uint16_t last_scene = -1;
	
	/* temporary variables */
	struct anim *item;
	z64_disp_buf_t *buf;
	z64_gfx_t *gfx_ctxt;
	Gfx todo;
	uint16_t scene;
	int prev_seg = 0;
	Gfx *work = 0;
	Gfx *Owork = 0; // = 0 is not necessary
	int has_written_pointer = 0;  /* used to test if pointer written */
	
	scene = gl->scene_index;	
	gfx_ctxt = (gl->common).gfx_ctxt;
	
	//if (list != NULL && list->pad != 0xDE)
	//	last_scene = -1;
	
	
	/* re-parse scene header on change */
	if (scene != last_scene)
	{
		list = get_0x1A(zh_get_current_scene_header(gl));
		last_scene = scene;
		
		/* preprocess the list, converting to faster format */
		if (list)
		{
		//	list->pad = 0xDE;
			for (item = list; ; ++item)
			{
				int8_t Oseg = item->seg;
				
				/* this math is now only necessary at load time */
				item->seg = abs_int(item->seg) + 7;
				item->data = mkabs(gl, item->data);
				
				/* negative segment value indicates list end */
				if (Oseg <= 0)
				{
					item->seg |= 0x80;
					break;
				}
			}
		}
	}
	
	/* scene render init functions always start with this */
	z_debug_graph_alloc(&todo, gfx_ctxt, "wow", __LINE__);
	
	/* default environment color (opa) */
	buf = &(gfx_ctxt->poly_opa);
	gDPPipeSync(buf->p++);
	gDPSetEnvColor(buf->p++, 128, 128, 128, 128);
	
	/* default environment color (xlu) */
	buf = &(gfx_ctxt->poly_xlu);
	gDPPipeSync(buf->p++);
	gDPSetEnvColor(buf->p++, 128, 128, 128, 128);
	
	// testing
	//scroll_two(gl, 8, 0);
	
	/* no animation list */
	if (!list)
	{
		int i;
		
		/* propagate ram segments with defaults so scene *
		 * headers lacking a 0x1A command do not crash   */
		for (i = 0x08; i <= 0x0F; ++i)
			unused_segment(gl, i);
		
		goto cleanup;
	}
	
	/* parse every item in animation list */
	for (item = list; ; ++item)
	{
		int seg = item->seg & 0x7F;
		void *data = item->data;
		
		/* begin work on new dlist */
		if (seg != prev_seg || !work)
		{
			/* flush generated dlist's contents to ram segment */
			if (work)
			{
				/* is display list */
				if (work > Owork)
					gSPEndDisplayList(work++);
				
				segment(gl, prev_seg, Owork);
			}
			
			/* request space for 8 opcodes in graphics memory */
			Owork = work = graph_alloc(gl->common.gfx_ctxt, 8 * 8);
			prev_seg = seg;
			
			/* zero-initialize any variables needing it */
			has_written_pointer = 0;
		}
		
		switch (item->type)
		{
			/* scroll one layer */
			case 0x0000:
				scroll(gl, &work, data);
				break;
			
			/* scroll two layers */
			case 0x0001:
				scroll_two(gl, &work, data);
				break;
			
			/* cycle through color list (advance one per frame) */
			case 0x0002:
				unused_dl(&work);
				break;
			
			/* unused in MM */
			case 0x0003:
				unused_dl(&work);
				break;
			
			/* color easing with keyframe support */
			case 0x0004:
				unused_dl(&work);
				break;
			
			/* flag-based texture scrolling */
			/* used only in Sakon's Hideout in MM */
			case 0x0005:
				unused_dl(&work);
				break;
			
			/* nothing; data and seg are always 0 when this is used */
			case 0x0006:
				unused_dl(&work);
				break;
			
			/* extended functionality */
			
			/* pointer changes based on flag */
			case 0x0007:
				if (has_written_pointer)
					break;
				has_written_pointer = 1;
				pointer_flag(gl, &work, data);
				Owork = work;
				break;
			
			/* scroll tiles based on flag */
			case 0x0008:
				scroll_flag(gl, &work, data);
				break;
			
			/* loop through color list */
			case 0x0009:
				color_loop(gl, &work, data);
				break;
			
			/* loop through color list, with flag */
			case 0x000A:
				color_loop_flag(gl, &work, data);
				break;
			
			/* loop through pointer list */
			case 0x000B:
				if (has_written_pointer)
					break;
				has_written_pointer = 1;
				pointer_loop(gl, &work, data);
				Owork = work;
				break;
			
			/* loop through pointer list */
			/* skipped if flag is undesirable */
			case 0x000C:
				if (has_written_pointer)
					break;
				if (pointer_loop_flag(gl, &work, data))
					has_written_pointer = 1;
				Owork = work;
				break;
			
			/* loop through pointer list (each frame has its own time) */
			case 0x000D:
				if (has_written_pointer)
					break;
				has_written_pointer = 1;
				pointer_timeloop(gl, &work, data);
				Owork = work;
				break;
			
			/* loop through pointer list (each frame has its own time) */
			/* skipped if flag is undesirable */
			case 0x000E:
				if (has_written_pointer)
					break;
				if (pointer_timeloop_flag(gl, &work, data))
					has_written_pointer = 1;
				Owork = work;
				break;

			/* camera effect if flag is set */
			case 0x000F:
				cameraeffect(gl, &work, data);
				break;

			/* draw if flag is set */
			case 0x0010:
				conditionaldraw(gl, &work, data, seg);
				break;
			
			default:
				unused_dl(&work);
				break;
		}
		
		/* this bit means this is the last item in the list */
		if (item->seg & 0x80)
		{
			/* flush generated dlist's contents to ram segment */
			if (work)
			{
				/* is display list */
				if (work > Owork)
					gSPEndDisplayList(work++);
				
				segment(gl, prev_seg, Owork);
			}
			break;
		}
	}
#if 0
	/* day, night textures test */
	uint32_t texture[] = { 0x020081E0, 0x0200FBE0 };
	
	buf = &(gfx_ctxt->poly_opa);
	
	gSPSegment(
		buf->p++
		, seg
		, zh_seg2ram(texture[*(uint32_t*)Z64GL_IS_NIGHT])
	);
#endif
cleanup:
//	triangle_test(gl);
	/* scene render init functions always end with this */
	z_debug_graph_write(&todo, gfx_ctxt, "wow", __LINE__);
	
//	cameramod_shake(gl);
}
