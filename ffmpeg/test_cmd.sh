/Users/sergei/github/sailvue/cmake-build-debug/sailvue.app/Contents/MacOS/ffmpeg -progress - -nostats -y  \
 -framerate 9.822618 -i /private/tmp/race0/chapter_000_Start/overlay_%05d.png \
 -filter_complex   \
"[0:v][0] overlay=0:0 [out0]; " \
 -map [out0]  \
 -vcodec png \
-pix_fmt yuva420p \
 "/private/tmp/race0/chapter_000_Start/clip.mp4"