#!/usr/bin/gnuplot
#
# Plotting the data of file plotting_data1.dat
#
# AUTHOR: Hagen Wierstorf

reset

# wxt
#set terminal wxt size 350,262 enhanced font 'Verdana,10' persist
# png
set terminal pngcairo size 350,262 enhanced font 'Verdana,10'
set output 'mem_access.png'
# svg
#set terminal svg size 350,262 fname 'Verdana, Helvetica, Arial, sans-serif' \
#fsize '10'
#set output 'plotting_data1.svg'

# color definitions
set border linewidth 1.5
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 ps 1.5 # --- blue

unset key

#set format y "0x%g"
#set ytics 1
set tics scale 0.75

#set xrange [0:5]
#set yrange [0:4]

plot 'mem_access.dat' using 1:2 with linespoints ls 1

