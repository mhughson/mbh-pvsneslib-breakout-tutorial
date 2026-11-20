export PVSNESLIB_HOME ?= $(abspath $(CURDIR)/../vendor/pvsneslib)

ifeq ($(wildcard $(PVSNESLIB_HOME)/devkitsnes/snes_rules),)
$(error PVSnesLib 4.6.0 not found at $(PVSNESLIB_HOME). Extract the SDK into vendor/pvsneslib or set PVSNESLIB_HOME)
endif

AUDIOFILES := res/effectssfx.it res/pollen8.it
export SOUNDBANK := res/soundbank

export ROMNAME := breakout
export ROMTITLE := BREAKOUT TUTORIAL
# ROM + save RAM + battery, with 2 KB of save RAM for the high score.
export CARTRIDGETYPE := 02
export SRAMSIZE := 01

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

SMCONVFLAGS := -s -o $(SOUNDBANK) -V -b 5 -f

.PHONY: all clean musics

all: musics $(ROMNAME).sfc

musics: $(SOUNDBANK).obj

# The sample music and effects are not redistributed in this repository;
# copy them from the SDK's own sound example.
res/%.it: $(PVSNESLIB_HOME)/snes-examples/audio/effectsandmusic/res/%.it
	mkdir -p res
	cp $< $@

paddle.pic: paddle.png
	$(GFXCONV) -s 16 -o 16 -u 16 -p -t png -i $<

ball.pic: ball.png
	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<

capsule.pic: capsule.png
	$(GFXCONV) -s 8 -R -o 16 -u 16 -p -t png -i $<

playfield.pic: playfield.png
	$(GFXCONV) -s 8 -o 16 -u 16 -e 2 -p -m -t png -i $<

backdrop.pic: backdrop.png
	$(GFXCONV) -s 8 -o 4 -u 4 -e 0 -p -m -t png -i $<

title-logo.pic: title-logo.png
	$(GFXCONV) -s 8 -o 64 -u 16 -e 3 -p -m -t png -i $<

src/data.obj: paddle.pic ball.pic capsule.pic playfield.pic backdrop.pic title-logo.pic
src/main.obj src/game.obj: src/game.h

clean: cleanBuildRes cleanRom cleanGfx cleanAudio
