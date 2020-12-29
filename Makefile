FILENAME = z64scene
FILE   = src/$(FILENAME)
OBJ    = src/$(FILENAME).o
BIN    = bin/$(FILENAME)
PUT    = bin/util/put        # this utility generates the cloudpatch

# N64 toolchain
CC      = mips64-gcc
LD      = mips64-ld
OBJCOPY = mips64-objcopy
OBJDUMP = mips64-objdump

# default compilation flags
CFLAGS = -DNDEBUG -Wall -Wno-main -mno-gpopt -fomit-frame-pointer -G 0 -Os --std=gnu99 -mtune=vr4300 -mabi=32 -mips3 -mno-check-zero-division -mno-explicit-relocs -mno-memcpy -Wno-unused-function -Wno-strict-aliasing
LDFLAGS = -L$(Z64OVL_LD) -T z64ovl.ld --emit-relocs

# display compilation options
default:
	@echo "type: make z64scene GAME=game MODE=mode"
	@echo "  GAME options"
	@echo "    oot-debug      build for OoT debug"
	@echo "    oot-ntsc-10    build for OoT NTSC 1.0"
	@echo "  MODE options"
	@echo "    ALL            support all features"
	@echo "    NO_MINIMAP     exclude mini-map features"

# every GAME option should have a matching .ld of the same name
LDFILE = src/ld/$(GAME).ld

# ROMOFS is the rom offset where we inject the code
ROMOFS = $(shell cat $(LDFILE) | grep ROMOFS | head -c 8)

# ROMPTR is the rom offset of the pointer to the original
#        function; this gets overwritten with one that will call
#        the new function
ROMPTR = $(shell cat $(LDFILE) | grep ROMPTR | head -c 8)

# RAMADDR is the entry point we use when compiling
RAMADDR = $(shell cat $(LDFILE) | grep RAMADDR | head -c 8)

######################
# BILLBOARDING NOTES #
######################

# this is hacky; use https://github.com/z64me/rank_pointlights instead

# the following is for overriding the game's original billboarding;
# originally, OoT allocates a single 0x40 byte block using graph_alloc,
# and generates a cylindrical billboarding matrix within;
# these offsets and assembly cause the game to use a global variable
# defined in the source file instead, so that the offset of the matrix
# is not only conveniently available in the source, , but followed by
# an additional 0x40 byte block, so that the cylindrical billboarding
# can be copied there and turned into a spherical billboard

# offset of original billboarding assembly
BBMTX_ASM_OFS = $(shell cat $(LDFILE) | grep BBMTX_ASM_OFS | head -c 8)

# new assembly to write there; it is this:
#  lui    v0, $0000
#  addiu  v0, v0, $0000
BBMTX_ASM_BIN = $(shell cat $(LDFILE) | grep BBMTX_ASM_BIN | head -c 16)

# offset of $0000 in lui above
BBMTX_HI = $(shell cat $(LDFILE) | grep BBMTX_HI | head -c 8)

# offset of $0000 in addiu above
BBMTX_LO = $(shell cat $(LDFILE) | grep BBMTX_LO | head -c 8)

#
# putting these billboarding variables together
#
# the raw assembly bytes (BBMTX_ASM_BIN) get written at BBMTX_ASM_OFS;
# BBMTX_HI and BBMTX_LO, which are simply $0000 in the above assembly,
# are then updated with the address of C variable new_billboards__

#############################
# END OF BILLBOARDING STUFF #
#############################

# MAXBYTES is the number of bytes that have been deemed safe to
#          overwrite with custom code
MAXBYTES = $(shell cat $(LDFILE) | grep MAXBYTES | head -n1)

# TARGET is the cloudpatch that will be created, of the form
#        "patch/codec/game_z64scene_codec.txt"
#        this may seem verbose, but consider how cryptic mm-u.txt
#        would be in a messy download folder
# NOTE   you can change this to a rom to target a rom directly; if
#        you do that, you will want to add something to this makefile
#        to produce a clean rom each run
TARGET = $(shell printf 'patch/' && printf $(GAME) && printf '_z64scene_' && printf $(GAME) && printf '.txt')

#TARGET = "private/oot-debug.z64"

# add the linker file for the chosen ga me to the compilation flags
LDFLAGS += $(LDFILE)

# fetch DEFINEs from the linker file
CFLAGS += $(shell cat $(LDFILE) | grep DEFINE | sed -e 's/\s.*$$//')

z64scene: all

all: clean build rompatch

build: $(OBJ)
	@echo -n "ENTRY_POINT = 0x" > entry.ld
	@echo -n $(RAMADDR) >> entry.ld
	@echo -n ";" >> entry.ld
	@mkdir -p patch
	@mkdir -p bin/util
	@$(LD) -o $(FILE).elf $(OBJ) $(LDFLAGS)
	@$(OBJCOPY) -R .MIPS.abiflags -O binary $(FILE).elf $(FILE).bin
	@printf "$(FILE).bin: "
	@stat --printf="%s" $(FILE).bin
	@printf " out of "
	@printf $(MAXBYTES)
	@printf " bytes used\n"
	@gcc -o bin/util/put src/util/put.c
	@gcc -o bin/util/n64crc src/util/n64crc.c
	@mv src/*.bin src/*.elf src/*.o bin

rompatch:
	@$(PUT) $(TARGET) --file $(ROMOFS) $(BIN).bin
# hacky billboarding hook; use the new version instead https://github.com/z64me/rank_pointlights
#	@$(PUT) $(TARGET) --bytes $(BBMTX_ASM_OFS) $(BBMTX_ASM_BIN)
#	@$(PUT) $(TARGET) --hilo $(BBMTX_HI) $(BBMTX_LO) $(shell $(OBJDUMP) -t $(BIN).elf | grep new_billboards__ | head -c 8)
# update function pointer
# TODO this updates ONLY kokiri forest's pointer for now
	@$(PUT) $(TARGET) --bytes $(ROMPTR) $(shell $(OBJDUMP) -t $(BIN).elf | grep .text.startup | head -c 8)
# this overrides interface compass drawing (when that was being tested)
#	@$(PUT) $(TARGET) --jump 0xAF83E0 $(shell $(OBJDUMP) -t $(BIN).elf | grep interface_draw_compass | head -c 8)

clean:
	@echo "do nothing"
