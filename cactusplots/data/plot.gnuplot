#set term epslatex color rounded size 4.9,2.2
#set term postscript size 8,4
set term png rounded size 800,500 font Vera 10
#set term postscript eps color lw 1.5 "Helvetica" 29 size 7,3

set pointsize 1
set tics scale 2

#set ytics 4e+8
#set output "only-vs-congl.eps"
set xlabel "No. solved instances from SAT Comp'09"
set ylabel "Time (s)"
#unset key
set xtics 40
set ytics 200
#set key inside b
#set logscale x
#set xtics (10,40,160,640,2560,5000)
set style line 1 lt 1 lw 1 pt 4 ps 0.3 linecolor rgbcolor "red"
set style line 2 lt 2 lw 1 pt 8 ps 0.3 linecolor rgbcolor "orange"
set style line 3 lt 3 lw 1 pt 12 ps 0.3 linecolor rgbcolor "blue"

##########################################

#set key lmargin
set key left top

set output "lingeling-glucose-cryptoms.png"
plot [80:] "./crypto-graph" w lp title "CryptoMiniSat 3.0", "./ling-graph" w lp title "Lingeling b02aa1a-121013", "gluco-graph" w lp title "Glucose 2.2"

