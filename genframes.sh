#!/bin/sh

run()
{
    format=$1
    gst-launch-1.0 videotestsrc num-buffers=1 ! \
	video/x-raw, format=$format, width=320, height=240  ! \
	filesink location="$format.rgb"
}

formats="RGBx BGRx xRGB xBGR RGBA BGRA ARGB ABGR RGB BGR RGB16 BGR16 RGB15 BGR15 GBR GBR_10LE GBR_10BE"

for i in $formats; do
    run $i
done

