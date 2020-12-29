/* OoT debug */
ADDRESS_START = 0x8009E0B8; /* where code goes in ram */

/* important offsets
00B15258      ROMOFS         where code is injected into rom
00BA1554      ROMPTR         location of pointer to update in rom
8009E0B8      RAMADDR        entry point

NOTE: read the makefile for a full explanation
      of what these BBMTX variables are for
00B362D8      BBMTX_ASM_OFS
00B362DA      BBMTX_HI
00B362DE      BBMTX_LO
3C02000024420000  BBMTX_ASM_BIN
*/

/* extra compilation flags (one per line)
-DMAJORA=0    DEFINE
*/

/* note: how many bytes can safely be overwritten?
[???]         MAXBYTES */

INCLUDE src/ld/n64.ld
