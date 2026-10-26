export PVSNESLIB_HOME ?= $(CURDIR)/vendor/pvsneslib

ifeq ($(wildcard $(PVSNESLIB_HOME)/devkitsnes/snes_rules),)
$(error PVSnesLib 4.6.0 not found at $(PVSNESLIB_HOME). Extract the SDK into vendor/pvsneslib or set PVSNESLIB_HOME)
endif

export ROMNAME := breakout
export ROMTITLE := BREAKOUT TUTORIAL

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

.PHONY: all clean

all: $(ROMNAME).sfc

sprites.pic: sprites.png
	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<

src/data.obj: sprites.pic
src/main.obj src/game.obj: src/game.h

clean: cleanBuildRes cleanRom cleanGfx
