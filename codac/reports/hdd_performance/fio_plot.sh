#!/bin/sh
#
# This script is an almost total rewrite by Louwrentius
# of the original fio_generate_plots script provided as part of the FIO storage
# benchmark utiliy. I only retained how GNUplot is used to generate graphs, as
# that is something I know nothing about.
#
# The script uses the files generated by FIO to create nice graphs in the
# SVG format. This output format is supported by most modern browsers and
# allows resolution independent graphs to be generated.
#
# This script supports GNUPLOT 4.4 and higher.
#
# Version 1.0 @ 20121231
#
#
#

if [ -z "$1" ]; then
		echo "Usage: fio_generate_plots subtitle [xres yres]"
		exit 1
fi

GNUPLOT=$(which gnuplot)
if [ ! -x "$GNUPLOT" ]
then
		echo You need gnuplot installed to generate graphs
		exit 1
fi

TITLE="$1"

# set resolution
if [ ! -z "$2" ] && [ ! -z "$3" ]
then
		XRES="$2"
		YRES="$3"
else
		XRES=1280
		YRES=768
fi

if [ -z "$SAMPLE_DURATION" ]
then
	SAMPLE_DURATION="*"
fi

DEFAULT_GRID_LINE_TYPE=3
DEFAULT_LINE_WIDTH=2
DEFAULT_LINE_COLORS="
set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb\"#ffffff\" behind
set style line 1 lc rgb \"#E41A1C\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 2 lc rgb \"#377EB8\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 3 lc rgb \"#4DAF4A\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 4 lc rgb \"#984EA3\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 5 lc rgb \"#FF7F00\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 6 lc rgb \"#DADA33\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 7 lc rgb \"#A65628\" lw $DEFAULT_LINE_WIDTH lt 1;
set style line 20 lc rgb \"#000000\" lt $DEFAULT_GRID_LINE_TYPE lw $DEFAULT_LINE_WIDTH;
"

DEFAULT_TERMINAL="set terminal postscript eps enhanced color" # dashed size $XRES,$YRES"
DEFAULT_TITLE_FONT="\"Helvetica,28\""
DEFAULT_AXIS_FONT="\"Helvetica,14\""
DEFAULT_AXIS_LABEL_FONT="\"Helvetica,16\""
DEFAULT_XLABEL="set xlabel \"Time (sec)\" font $DEFAULT_AXIS_LABEL_FONT"
DEFAULT_XTIC="set xtics font $DEFAULT_AXIS_FONT"
DEFAULT_YTIC="set ytics font $DEFAULT_AXIS_FONT"
DEFAULT_MXTIC="set mxtics 1"
DEFAULT_MYTIC="set mytics 2"
DEFAULT_XRANGE="set xrange [0:$SAMPLE_DURATION]"
DEFAULT_YRANGE="set yrange [0:500]"
DEFAULT_GRID="set grid ls 20"
# DEFAULT_KEY="set key outside bottom center ; set key box enhanced spacing 2.0 samplen 3 horizontal width 4 height 1.2 "
# DEFAULT_SOURCE="set label 30 \"Data source: http://example.com\" font $DEFAULT_AXIS_FONT tc rgb \"#00000f\" at screen 0.976,0.175 right"

DEFAULT_OPTS="\
$DEFAULT_TERMINAL;
$DEFAULT_LINE_COLORS;
$DEFAULT_GRID_LINE;
$DEFAULT_GRID;
$DEFAULT_GRID_MINOR;
$DEFAULT_XLABEL;
$DEFAULT_XRANGE;
$DEFAULT_YRANGE;
$DEFAULT_XTIC;
$DEFAULT_YTIC;
$DEFAULT_MXTIC;
$DEFAULT_MYTIC;
"


plot () {

	if [ -z "$TITLE" ]
	then
		PLOT_TITLE=" set title \"$1\" font $DEFAULT_TITLE_FONT"
	fi
	FILETYPE="$2"
	YAXIS="set ylabel \"$3\" font $DEFAULT_AXIS_LABEL_FONT"
	SCALE=$4

	echo "Title: $PLOT_TITLE"
	echo "File type: $FILETYPE"
	echo "yaxis: $YAXIS"

	i=0

	for x in *_"$FILETYPE".log *_"$FILETYPE".*.log
	do
		if [ -e "$x" ]; then
			i=$((i+1))
			if [ ! -z "$PLOT_LINE" ]
			then
				PLOT_LINE=$PLOT_LINE", "
			fi
			NAME=$(echo $x | cut -d "_" -f 1)
			PLOT_LINE=$PLOT_LINE"'$x' using (\$1/1000):(\$2/($SCALE)) title \"$NAME\" with ${5-lines} ls $i"
		fi
	done

	if [ $i -eq 0 ]; then
	   echo "No log files found"
	   exit 1
	fi

	OUTPUT="set output \"$TITLE-$FILETYPE.eps\" "

	echo " $PLOT_TITLE ; $YAXIS ; $DEFAULT_OPTS ; show style lines ; $OUTPUT ; plot "  $PLOT_LINE  | $GNUPLOT -
#	echo " $PLOT_TITLE ; $YAXIS ; $DEFAULT_OPTS ; show style lines ; $OUTPUT ; plot "  $PLOT_LINE  > plot_$FILETYPE.gpl
	unset PLOT_LINE
}

#
# plot <sub title> <file name tag> <y axis label> <y axis scale>
#
plot "I/O Latency" lat "Time (msec)" 1000
#plot "I/O Operations Per Second" iops "IOPS" 1
#plot "I/O Submission Latency" slat "Time (μsec)" 1
#plot "I/O Completion Latency" clat "Time (msec)" 1000
plot "I/O Bandwidth" bw "Throughput (MB/s)" 1024 dots
