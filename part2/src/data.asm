.include "hdr.asm"

.section ".breakout_assets" superfree

paddle_tiles:
.incbin "paddle.pic"

ball_tiles:
.incbin "ball.pic"

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
