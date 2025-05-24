CC=gcc
AR=ar rcs

FS_INC=-I/opt/freeswitch/include/freeswitch
#FS_LIB=-L/opt/freeswitch/lib -lfreeswitch.so
#LIBS=${FS_LIB}

#CFLAGS=-O2 -s -fPIC -DWITH_FREESWITCH -I./sources -I./sources/signal_processing ${FS_INC} 
CFLAGS=-g -O2 -fPIC -DSIVR_VAD_DEBUG -DWITH_FREESWITCH -I./sources -I./sources/signal_processing ${FS_INC} 

FVAD_SRC= sources/signal_processing/energy.c \
	sources/signal_processing/division_operations.c \
	sources/signal_processing/get_scaling_square.c \
	sources/signal_processing/resample_48khz.c \
	sources/signal_processing/resample_by_2_internal.c \
	sources/signal_processing/resample_fractional.c \
	sources/signal_processing/spl_inl.c \
	sources/vad/vad_core.c \
	sources/vad/vad_filterbank.c \
	sources/vad/vad_gmm.c \
	sources/vad/vad_sp.c \
	sources/fvad.c

ST_LIB=libsivrvad.a
LIB_SRC=sources/sivr-vad.c ${FVAD_SRC}
LIB_OBJ=$(LIB_SRC:.c=.o)

all: $(LIB_SRC) $(ST_LIB)

$(ST_LIB): $(LIB_OBJ)
	$(AR) $@ $(LIB_OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

install:
	exit 0

clean:
	@rm -f sources/*.o sources/vad/*.o sources/signal_processing/*.o
	@rm -f *.a core.*
