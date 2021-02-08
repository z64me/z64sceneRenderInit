# z64scene format specification

As you work through this format specification, please refer
to `types.h` and `z64scene.c`, which have been commented
thoroughly. An example scene file I crafted in a hex editor
has also been included in this package. It utilizes most
of the features available, and you can learn a lot about how
it works by stepping through the code while looking at the
file in a hex editor.

```

	In the scene header, a single 1A command points to a list of
	ram segments to initialize. Scene headers contain only one
	1A command. Alternate headers should be supported as well.
	
	All formats in this document are described in hexadecimal
	notation unless otherwise specified.
	
	1A000000 xxxxxxxx
	
		x = ram segment initialization list
	
	The ram segment initialization list follows this format:
	
	xx 00 wwww zzzzzzzz
	
	x (signed 8-bit value)
	
		ram segment  = abs(x) + 7
		
		last in list = x <= 0
		
		example:
			
			x = 0xFF
			
			x = -1 (0xFF is -1)
			
			abs(-1) + 7 = 8 (propagate ram segment 08)
			
			-1 <= 0, therefore, this is the last ram segment in the list
		
		* Majora's Mask note:
		  when x == 0, the following w value is always 0006, and z always
		  points to 0 (no data); therefore, we can make the assumption that
		  this combination means the scene contains no animated textures
		
		* abs(x) means "the absolute value of x".
		
		(Please don't hate me... Majora's Mask does it the same way...)
	

	w (16-bit value)
		
	/* vanilla Majora's Mask features (some are supported) */
		
		0000: scroll one texture layer
			
			uu vv ww hh
			
				u = speed along x axis
				
				v = speed along y axis
				
				w = texture width
				
				h = texture height

		0001: scroll two texture layers
		
			* The following structure repeats: once for texture layer 0, then 1
			
			uu vv ww hh
			
				u = speed along x axis
				
				v = speed along y axis
				
				w = texture width
				
				h = texture height

		/* not implemented; not enough is known yet */
		0002: cycle color through color list (one color per frame)
			
			cccc ???? pppppppp ???????? ????????
			                   ff = crash
			
				c = number of colors in color list
				
				p = pointer to color list
				
				? = always 0 wherever it's used in MM...
			
			TODO: This may actually do both primcolor and envcolor.
			
			TODO: This may actually be structured similarly to type 0004.
			
			TODO: Colors seem to follow RR GG BB AA format. Please confirm.
			
			* Used only in Goron Village Shop and Unfrozen Mountain Village.
			
			* Upon entering the Goron Village Shop, there are pointy gems
			  to your right. Behind those, cuboid gems. The cuboid gems have
			  a noticeable flicker on them if you look closely enough.
			
			* The ram segment initialized for Unfrozen Mountain Village
			  appears to go unused.
		
		/* not implemented; not enough is known yet */
		0003: unused in MM
		
			TODO: Despite being unused, it may still do something.

		/* not implemented; not enough is known yet */
		0004: color easing with keyframe support
			
			xxxx yyyy pppppppp ???????? kkkkkkkk
			
				x = number of frames before looping
				
				y = number of keyframes?
				
				p = pointer to list of primitive colors
				
				? = pointer to list of environment colors?
				
				k = pointer to keyframes
			
			* Note on colors:
			
				r g b a = first color in list
				a r g b = second
				b a r g = third
				a r g b = ...
				
				TODO: Inspect that more closely. It could be completely wrong.

		/* not implemented; not enough is known yet */
		0005: probably flag-based texture scrolling
		
			TODO: It's used only in Sakon's Hideout. Chances are it's
			      texture scrolling that can be toggled via a (local?) flag.
		
		/* not implemented; not enough is known yet */
		0006: nothing; data and seg are always 0 when this is used
		
	/* extended functionality */
		
		0007: pointer changes based on flag
			
			xxxxxxxx yyyyyyyy ffffffff ffffffff ffffffff ffffffff
			
				x = pointer used if flag not set
				
				y = pointer used if flag is set
				
				f = flag (see flag section)
		
		0008: scroll two layers if flag is set
			
			u0 v0 w0 h0 u1 v1 w1 h1 ffffffff ffffffff ffffffff ffffffff
			
				u0 = speed along x axis (layer 0)
				
				v0 = speed along y axis
				
				w0 = texture width
				
				h0 = texture height
			
				u1 = speed along x axis (layer 1)
				
				v1 = speed along y axis
				
				w1 = texture width
				
				h1 = texture height
			
				x = pointer used if flag not set
				
				y = pointer used if flag is set
				
				f = flag (see flag section)
		
		0009: loop through color list
			
			ww ee dddd [kkkkkkkk kkkkkkkk kkkkkkkk]
			
				w = which (bitfield describing values to calculate) (see colorkey_types)
				
				e = easing function (see easing functions)
				
				d = number of game frames to cycle through list once
				
				k = colorkey array (see colorkey section)
		
		000A: loop through color list if flag set
			
			ffffffff ffffffff ffffffff ffffffff
			ww ee dddd [kkkkkkkk kkkkkkkk kkkkkkkk]
				
				f = flag (see flag section)
			
				w = which (bitfield describing values to calculate) (see colorkey_types)
				
				e = easing function (see easing functions)
				
				d = number of game frames to cycle through list once
				
				k = colorkey array (see colorkey section)
		
		000B: loop through pointer list
			
			dddd tttt eeee 0000 [pppppppp]
			
				d = duration of full cycle (frames)
				
				t = frames elapsed (will always be 0 inside the zscene)
				
				e = number of frames to display each pointer
				
				0 = unused padding for later
				
				p = array of pointers to cycle through
		
		000C: loop through pointer list if flag is active
			
			ffffffff ffffffff ffffffff ffffffff
			dddd tttt eeee 0000 [pppppppp]
				
				f = flag (see flag section)
			
				d = duration of full cycle (frames)
				
				t = frames elapsed (0 inside the zscene)
				
				e = number of frames to display each pointer
				
				0 = unused padding for later
				
				p = array of pointers to cycle through
		
		000D: loop through pointer list (each frame has its own time)
			
			iiii tttt nnnn [dddd] [0000] [pppppppp]
				
				i = item selected on previous frame (0 in zscene)
				
				t = frames elapsed (0 in zscene)
				
				n = number of pointers in list
				
				d = duration to display each pointer, in order (array)
				
				0 = this padding will be present if n is an odd number
				    (it is used to ensure p is 4-byte-aligned)
				
				p = array of pointers to cycle through
		
		000E: loop through pointer list (each frame has its own time) if flag set
			
			ffffffff ffffffff ffffffff ffffffff
			iiii tttt nnnn [dddd] [0000] [pppppppp]
				
				f = flag (see flag section)
				
				i = item selected on previous frame (0 in zscene)
				
				t = frames elapsed (0 in zscene)
				
				n = number of pointers in list
				
				d = duration to display each pointer, in order (array)
				
				0 = this padding will be present if n is an odd number
				    (it is used to ensure p is 4-byte-aligned)
				
				p = array of pointers to cycle through
		
	
	z (unsigned 32-bit pointer)
	
		points to data of the structure described by w


	flags (flag section)
		
		a flag is structured like so
		
		ffffffff aaaaaaaa tt ee xxxx dddddddd
		
			f = flag or (or four-byte-aligned address)
			
			a = and (bitwise value test) (used only for save/global/ram types)
			
			t = type
				
				types                      test involved
				00 FLAG_TYPE_ROOMCLEAR     e == flag_get_roomclear(f)
				01 FLAG_TYPE_TREASURE      e == flag_get_treasure(f)
				02 FLAG_TYPE_USCENE        e == flag_get_uscene(f)
				03 FLAG_TYPE_TEMP          e == temp_clear_flag_get(f)
				04 FLAG_TYPE_SCENECOLLECT  e == flag_get_scenecollect (f)
				05 FLAG_TYPE_SWITCH        e == flag_get_switch(f) 
				06 FLAG_TYPE_EVENTCHKINF   e == flag_get_event_chk_inf(f)
				07 FLAG_TYPE_INFTABLE      e == flag_get_inf_table(f)
				08 FLAG_TYPE_IS_NIGHT      e == flag on if night time(f)
				
				09 FLAG_TYPE_SAVE          e == !!(u32(  save_ctx[flag]) & and)
				0A FLAG_TYPE_GLOBAL        e == !!(u32(global_ctx[flag]) & and)
				0B FLAG_TYPE_RAM           e == !!(u32(flag) & and)
			
			e = equals (0 tests if flag is off, 1 tests if flag is on)
			
			x = crossfade (for interpolating colors) (0 in zscene)
			
			d = frames flag has been active (0 in zscene)
	
	
	colorkey_types
	
		a bitfield used to describe what color key values to calculate
		and include in the generated F3DZEX2 microcode
		
			0000 mlep  (8-bit binary representation)
				
				p = interpolate primcolor
				
				e = interpolate envcolor
				
				l = interpolate lodfrac
				
				m = interpolate minlevel
	
	
	colorkey (colorkey section)
		
		a substructure used to describe a color keyframe
			
			pppppppp eeeeeeee ll mm nnnn
				
				p = primcolor
				
				e = envcolor
				
				l = lodfrac
				
				m = minlevel
				
				n = number of frames until next keyframe, or 0 on last keyframe
	
	
	easing functions
		
		only linear easing is currently supported, but the code
		for implementing other easing functions is in place; just
		add them to the `enum ease` array in `types.h` as you go,
		and only add them to the end of the list so we remain
		backwards-compatible with older maps
		
		enum ease
		{                            /* behavior          */
			EASE_LINEAR = 0           /* linear            */
			, EASE_SIN_IN             /* sinusoidal (in)   */
			, EASE_SIN_OUT            /* sinusoidal (out)  */
		};
		
		EASE_SIN_IN is automatically 1
		
		EASE_SIN_OUT is automatically 2
		
		etc
	
	
	a note on ram segments
		
		unlike Majora's Mask, you can specify the same ram segment
		multiple times; this allows you to generate a ram segment
		combining both texture scrolling and color lists, and even
		specify different behaviors to invoke for different flags

```
