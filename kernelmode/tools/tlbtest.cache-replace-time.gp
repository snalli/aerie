# Calibrator v0.9e
# (by Stefan.Manegold@cwi.nl, http://www.cwi.nl/~manegold/)
 set term postscript portrait enhanced
 set output 'tlbtest.cache-replace-time.ps'
#set term gif transparent interlace small size 500, 707 # xFFFFFF x333333 x333333 x0055FF x005522 x660000 xFF0000 x00FF00 x0000FF
#set output 'tlbtest.cache-replace-time.gif'
set data style linespoints
set key below
set title 'tlbtest.cache-replace-time'
set xlabel 'memory range [bytes]'
set x2label ''
set ylabel 'nanosecs per iteration'
set y2label 'cycles per iteration'
set logscale x 2
set logscale x2 2
set logscale y 10
set logscale y2 10
set format x '%1.0f'
set format x2 '%1.0f'
set format y '%1.0f'
set format y2 ''
set xrange[0.750000:10240.000000]
#set x2range[0.750000:10240.000000]
set yrange[1.000000:1000.000000]
#set y2range[1.000000:1000.000000]
set grid x2tics
set xtics mirror ('1k' 1, '' 2, '4k' 4, '' 8, '16k' 16, '' 32, '64k' 64, '' 128, '256k' 256, '' 512, '1M' 1024, '' 2048, '4M' 4096, '' 8192)
set x2tics mirror ('[32k]' 32, '[3M]' 3072)
set y2tics ('(8)' 3.220000, '(21)' 8.320000, '(199)' 79.750000, '2.5' 1, '25' 10, '250' 100, '2.5e+03' 1000)
set label 1 '(3.2)  ' at 0.750000,3.201281 right
set arrow 1 from 0.750000,3.201281 to 10240.000000,3.201281 nohead lt 0
set label 2 '(8.4)  ' at 0.750000,8.403361 right
set arrow 2 from 0.750000,8.403361 to 10240.000000,8.403361 nohead lt 0
set label 3 '(79.6)  ' at 0.750000,79.631851 right
set arrow 3 from 0.750000,79.631851 to 10240.000000,79.631851 nohead lt 0
 set label 4 '^{ Calibrator v0.9e (Stefan.Manegold\@cwi.nl, www.cwi.nl/~manegold) }' at graph 0.5,graph 0.02 center
#set label 4    'Calibrator v0.9e (Stefan.Manegold@cwi.nl, www.cwi.nl/~manegold)'    at graph 0.5,graph 0.03 center
plot \
0.1 title 'stride:' with points pt 0 ps 0 , \
'tlbtest.cache-replace-time.data' using 1:($7-0.000000) title '256' with linespoints lt 1 pt 3 , \
'tlbtest.cache-replace-time.data' using 1:($13-0.000000) title '\{128\}' with linespoints lt 2 pt 4 , \
'tlbtest.cache-replace-time.data' using 1:($19-0.000000) title '\{64\}' with linespoints lt 3 pt 5 , \
'tlbtest.cache-replace-time.data' using 1:($25-0.000000) title '32' with linespoints lt 4 pt 6 , \
'tlbtest.cache-replace-time.data' using 1:($31-0.000000) title '16' with linespoints lt 5 pt 7 , \
'tlbtest.cache-replace-time.data' using 1:($37-0.000000) title '8' with linespoints lt 6 pt 8
set nolabel
set noarrow
