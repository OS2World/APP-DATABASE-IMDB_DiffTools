#########################################################################
#                                                                       #
# Makefile for DiffTools                                                #
#                                                                       #
# (c)1996-2001 by Andre Bernhardt <ab@imdb.com>                         #
#                                                                       #
# last changes : 20.11.2001                                             #
#                                                                       #
#########################################################################

# Packer-options Please change as required
#PACK_NAME      e.g. /usr/bin/gzip
#PACK_EXT       file extension e.g. ".gz"
#PACK_COMPRESS  -1 (fast, ineffecient) ... -9 (slow, efficient)

#PACK_NAME       = "\"gzip\""
#PACK_EXT        = "\".gz\""
#PACK_COMPRESS   = "\"-9\""
#PACK_UNCOMPRESS = "\"-d\""
#USE_PACKER = -DIMDB_GZIP -DIMDBV_FILE_PACKER_NAME=$(PACK_NAME) -DIMDBV_FILE_PACKER_EXT=$(PACK_EXT) -DIMDBV_FILE_PACKER_PACK=$(PACK_COMPRESS) -DIMDBV_FILE_PACKER_UNPACK=$(PACK_UNCOMPRESS)

#### GCC - LINUX  ####

CC         = gcc
CFLAGS     = -DSYS_UNIX -O2 -c

LD         = gcc
LIBS       =
LDFLAGS    = -s -Zexe

DELETE     = rm

#### CC - NEXT ####
#
#CC         = cc
#CFLAGS     = -DSYS_UNIX -DNEXT -O2 -c
#
#LD         = cc
#LIBS       =
#LDFLAGS    = -s
#
#DELETE     = rm

#### AMIGA - Dice ####
#
#CC         = dcc
#CFLAGS     = -DSYS_AMIGA -proto
#
#LD         = dcc
#LIBS       =
#LDFLAGS    =
#
#DELETE     = delete


#########################################################################

EXE = ApplyDiffs CheckCRC

SRC = ApplyDiffs.c CheckCRC.c IMDB_Resources.c

OBJ = ApplyDiffs.o CheckCRC.o IMDB_Resources.o

all: $(EXE)


ApplyDiffs.o : ApplyDiffs.c IMDB.h
	$(CC) $(CFLAGS) $(USE_PACKER) -o ApplyDiffs.o -c ApplyDiffs.c

IMDB_Resources.o : IMDB_Resources.c IMDB.h
	$(CC) $(CFLAGS) -o IMDB_Resources.o -c IMDB_Resources.c

CheckCRC.o : CheckCRC.c IMDB.h
	$(CC) $(CFLAGS) -o CheckCRC.o -c CheckCRC.c


clean:
	$(DELETE) $(OBJ) $(EXE)


ApplyDiffs: ApplyDiffs.o IMDB_Resources.o
	$(LD) $(LDFLAGS) -o ApplyDiffs ApplyDiffs.o IMDB_Resources.o $(LIBS)

CheckCRC: CheckCRC.o
	$(LD) $(LDFLAGS) -o CheckCRC CheckCRC.o IMDB_Resources.o $(LIBS)

