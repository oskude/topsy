#!/usr/bin/env bash
# record x window to gif

declare -i X=0  # window x position
declare -i Y=0  # window y position
declare -i W=0  # window width
declare -i H=0  # window height
declare -i F=4  # frames per second
declare -i T=5  # seconds to record
declare outfile="wincap.gif"
declare tmpfile="/tmp/wincap.mkv"
declare ffmpeg="ffmpeg -v 24 -y"

echo "Select a window to record, by clicking it ..."
while read -r l
do
	c=($l)
	[[ ${c[0]} == "Width:" ]] && W=${c[1]} && continue
	[[ ${c[0]} == "Height:" ]] && H=${c[1]} && continue
	if [[ ${c[0]} == "Absolute" ]]
	then
		[[ ${c[2]} == "X:" ]] && X=${c[3]} && continue
		[[ ${c[2]} == "Y:" ]] && Y=${c[3]} && continue
	fi
done < <(xwininfo -stats)

echo "Recording ${T} seconds to ${outfile} ..."
# FTW: https://github.com/cyburgee/ffmpeg-guide
$ffmpeg -video_size ${W}x${H} -framerate ${F} -f x11grab -t ${T} -i ${DISPLAY}+${X},${Y} -c:v libx264 -crf 0 -preset ultrafast ${tmpfile}
$ffmpeg -i ${tmpfile} -filter_complex "[0:v] split [a][b];[a] palettegen [p];[b][p] paletteuse" ${outfile}
# following is oneliner without a tmpfile, but the output stutters :(
#ffmpeg -v 24 -y -video_size ${W}x${H} -framerate ${F} -f x11grab -t ${T} -i ${DISPLAY}+${X},${Y} -filter_complex "[0:v] split [a][b];[a] palettegen [p];[b][p] paletteuse" ${outfile}
