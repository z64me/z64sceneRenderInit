#ifndef Z64SCENE_TYPES_H_INCLUDED
#define Z64SCENE_TYPES_H_INCLUDED
	
#define enum8(x)   uint8_t
#define enum16(x)  uint16_t

enum flag_type
{	                          /* logic involved         */
	FLAG_TYPE_ROOMCLEAR = 0   /* flag_get_roomclear     */
	, FLAG_TYPE_TREASURE      /* flag_get_treasure      */
	, FLAG_TYPE_USCENE        /* flag_get_uscene        */
	, FLAG_TYPE_TEMP          /* temp_clear_flag_get    */
	, FLAG_TYPE_SCENECOLLECT  /* flag_get_scenecollect  */
	, FLAG_TYPE_SWITCH        /* flag_get_switch        */
	, FLAG_TYPE_EVENTCHKINF   /* flag_get_event_chk_inf */
	, FLAG_TYPE_INFTABLE      /* flag_get_inf_table     */
	, FLAG_TYPE_IS_NIGHT      /* flag on if night time  */
	
	/* NOTE: for the following types, flag must be 4-byte-aligned */
	
	, FLAG_TYPE_SAVE          /* flag = word(  save_ctx[flag]) & and */
	, FLAG_TYPE_GLOBAL        /* flag = word(global_ctx[flag]) & and */
	, FLAG_TYPE_RAM           /* flag = word(flag) & and */
};

/* substructure used to describe flag logic */
struct flag
{
	uint32_t          flag;   /* flag or offset    */
	uint32_t          and;    /* bit selection     */
	enum8(flag_type)  type;   /* flag type         */
	uint8_t           eq;     /* if (flag() == eq) */
	uint16_t          xfade;  /* crossfade (color) */
	uint32_t          frames; /* frames flag is on */
};

/* data processed by pointer_flag functions */
struct pointer_flag
{
	uint32_t          ptr[2]; /* pointer pointers  */
	struct flag       flag;   /* flag structure    */
};

/* data processed by scroll functions */
struct scroll
{
	int8_t            u;      /* u speed           */
	int8_t            v;      /* v speed           */
	uint8_t           w;      /* texture w         */
	uint8_t           h;      /* texture h         */
};

/* data processed by scroll_flag functions */
struct scroll_flag
{
	struct scroll     sc[2];  /* scroll contents   */
	struct flag       flag;   /* flag structure    */
};

enum colorkey_types
{
	COLORKEY_PRIM       = 1 << 0
	, COLORKEY_ENV      = 1 << 1
	, COLORKEY_LODFRAC  = 1 << 2
	, COLORKEY_MINLEVEL = 1 << 3
};

enum ease
{                            /* behavior          */
	EASE_LINEAR = 0           /* linear            */
	, EASE_SIN_IN             /* sinusoidal (in)   */
	, EASE_SIN_OUT            /* sinusoidal (out)  */
};

/* substructure used to describe color keyframe */
struct colorkey
{
	uint32_t          prim;   /* primcolor  (rgba) */
	uint32_t          env;    /* envcolor   (rgba) */
	uint8_t           lfrac;  /* lodfrac    (prim) */
	uint8_t           mlevel; /* minlevel   (prim) */
	uint16_t          next;   /* frames til next; 0 = last frame */
};

/* data processed by color functions */
struct colorlist
{
	enum8(colorkey)   which;  /* units to compute  */
	enum8(ease)       ease;   /* ease function     */
	uint16_t          dur;    /* duration          */
	struct colorkey   key[1]; /* keyframe storage  */
};

struct colorlist_flag
{
	struct flag       flag;   /* flag structure    */
	struct colorlist  list;   /* color structure   */
};

/* pointer loop */
struct pointer_loop
{
	uint16_t          dur;    /* duration of full cycle (frames) */
	uint16_t          time;   /* frames elapsed (internal use)   */
	uint16_t          each;   /* frames to display each item     */
	uint16_t          pad;    /* unused; padding                 */
	uint32_t          ptr[1]; /* list: dur/each elements long    */
};

/* pointer loop (flag) */
struct pointer_loop_flag
{
	struct flag         flag; /* flag structure */
	struct pointer_loop list; /* list structure */
};

/* pointer loop; each frame can have its own time */
struct pointer_timeloop
{
	uint16_t          prev;   /* item selected (previous frame)  */
	uint16_t          time;   /* frames elapsed (internal use)   */
	uint16_t          num;    /* number of pointers in list      */
	uint16_t          each[1];/* first frame of each pointer     */
//	uint32_t          ptr[1]; /* intentionally disabled, see note*/
	
	/* NOTE: each[1] can be any length; now imagine immediately  *
	 *       after it, there is a ptr[1], containing one pointer *
	 *       for each item; ptr[1] must be aligned by 4 bytes;   *
	 *       so to get a pointer to it, do the following:        *
	 *       uint32_t *ptr = (void*)(each + num + !(num & 1));   *
	 *       see the functions that process this structure if    *
	 *       you're having trouble                               */
	
	/* NOTE: each[] contains num items (the last indicating the  *
	 *       end frame), and ptr[] contains num-1 items          */
};

/* pointer loop (flag) */
struct pointer_timeloop_flag
{
	struct flag              flag; /* flag structure */
	struct pointer_timeloop  list; /* list structure */
};

/* animation settings; pointed to by 0x1A scene header command */
struct anim
{
	int8_t            seg;    /* ram segment       */
	uint8_t           pad;    /* unused            */
	uint16_t          type;   /* function          */
	void             *data;   /* data              */
};

#endif /* Z64SCENE_TYPES_H_INCLUDED */
