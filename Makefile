CC=sdcc
RM=rm
MV=mv
PACKIHX=packihx
MAKEBIN=makebin
TARGET?=VTX_S
BUILD=RELEASE

SDCC_OPTS:=--model-large --stack-auto
CFLAGS=$(SDCC_OPTS) -D$(TARGET)
LDFLAGS=$(SDCC_OPTS)

SRCS= \
  src/mcu.c \
  src/camera.c \
  src/dm6300.c \
  src/global.c \
  src/hardware.c \
  src/i2c.c \
  src/i2c_device.c \
  src/isr.c \
  src/lifetime.c \
  src/msp_displayport.c \
  src/rom.c \
  src/sfr_ext.c \
  src/smartaudio_protocol.c \
  src/spi.c \
  src/uart.c

ifneq ($(BUILD), RELEASE)
  CFLAGS+=-D_DEBUG_MODE

  SRCS+= \
    src/monitor.c \
    src/print.c 
endif

ifeq ($(TARGET), VTX_WL)
  SRCS+=src/vtx_wl.c
else ifeq ($(TARGET), VTX_L) 
  SRCS+=src/vtx_l.c
else ifeq ($(TARGET), VTX_S) 
  SRCS+=src/vtx_s.c
else ifeq ($(TARGET), VTX_R) 
  SRCS+=src/vtx_r.c
endif

_MAKE_BUILD_DIR:=$(shell mkdir -p obj bin)
OBJ_PATHS=$(subst src/,obj/,$(SRCS))
OBJS=$(subst .c,.rel,$(OBJ_PATHS))

all: $(TARGET).bin

$(TARGET).ihx: $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET).ihx $(OBJS)

%.hex: %.ihx
	$(PACKIHX) $< > $@

%.bin: %.hex
	$(MAKEBIN) -s 65536 -p $< $@
	$(MV) $(TARGET).* bin/

$(OBJS): obj/%.rel : src/%.c
	$(CC) $(CFLAGS) -o obj/ -c $<

clean:
	$(RM) -rf obj $(TARGET).*

cleanall: clean
	$(RM) -rf bin
