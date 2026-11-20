.include "hdr.asm"

.section ".breakout_assets" superfree

paddle_tiles:
.incbin "paddle.pic"

ball_tiles:
.incbin "ball.pic"

capsule_tiles:
.incbin "capsule.pic"

sprite_palette:
.incbin "paddle.pal"

backdrop_tiles:
.incbin "backdrop.pic"
backdrop_tiles_end:

backdrop_map:
.incbin "backdrop.map"

backdrop_palette:
.incbin "backdrop.pal"

playfield_tiles:
.incbin "playfield.pic"
playfield_tiles_end:

playfield_map:
.incbin "playfield.map"

playfield_palette:
.incbin "playfield.pal"

.ends

.section ".title_logo_assets" superfree

title_logo_tiles:
.incbin "title-logo.pic"
title_logo_tiles_end:

title_logo_map:
.incbin "title-logo.map"

title_logo_palette:
.incbin "title-logo.pal"

.ends
