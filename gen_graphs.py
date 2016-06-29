#!/usr/bin/env python3

import io
import os
import sys
import re
import subprocess

#the list of file prefixes to use
prefix="/tmp/timing_data"
lst = sorted([ int(x.split(".")[0]) for x in os.listdir(prefix) if re.match(R"^[0-9]+\.dat$", x)])

highest = [0]*6

x_tick_interval= str( lst[-1]/ 5 )

#create the data file
with open("/tmp/data", "w") as data:
    for fnumber in lst:
        fname = prefix+"/"+str(fnumber)+".dat"
        with open(fname, "r") as f:
            for line in f:
                lst = line.split(",")
                #turn some lines into ops/ns
                lst[2] = str( float(lst[2]) * 10**9 / fnumber)
                lst[3] = str( float(lst[3]) * 10**9 / fnumber)
                lst[4] = str( float(lst[4]) / fnumber)
                lst[5] = str( float(lst[5]) / fnumber)
                highest[2] = max( highest[2], float(lst[2]))
                highest[3] = max( highest[3], float(lst[3]))
                highest[4] = max( highest[4], float(lst[4]))
                highest[5] = max( highest[5], float(lst[5]))

                line = ",".join(lst).strip()
                print(line, file=data)

plot_string="""
set term "{output_type}"
set output "{output_name}"
set ylabel ("{y_axis_label}")
set xlabel ("{x_axis_label}")
set datafile separator ","
set key font ",12"
set key left
set yrange [{y_range_low}:{y_range_high}]
set xtics axis nomirror {x_tick_interval}

plot '/tmp/data' every 5    using {x_col}:{y_col} title 'unordered map' , \\
     '/tmp/data' every 5::1 using {x_col}:{y_col} title 'ut hash', \\
     '/tmp/data' every 5::2 using {x_col}:{y_col} title 'glib', \\
     '/tmp/data' every 5::3 using {x_col}:{y_col} title 'khash', \\
     '/tmp/data' every 5::4 using {x_col}:{y_col} title 'hhash'
quit
""".strip()

insert_speed={
        "output_type":"png",
        "output_name":"insert.png",
        "y_axis_label":"ns per insertion",
        "x_axis_label":"insertions in test",
        "y_range_low":"0",
        "y_range_high":str(highest[2]),
        "x_col":7,
        "y_col":3,
        "plot_title":"Insertion Time",
        "x_tick_interval":x_tick_interval
        }

access_speed={
        "output_type":"png",
        "output_name":"access.png",
        "y_axis_label":"ns per access",
        "x_axis_label":"insertions in test",
        "y_range_low":"0",
        "y_range_high":str(highest[3]),
        "x_col":7,
        "y_col":4,
        "plot_title":"Access Time",
        "x_tick_interval":x_tick_interval
        }

minimum_memory={
        "output_type":"png",
        "output_name":"minimum_memory.png",
        "y_axis_label":"bytes per element",
        "x_axis_label":"insertions in test",
        "y_range_low":"0",
        "y_range_high":str(highest[4]),
        "x_col":7,
        "y_col":5,
        "plot_title":"Bytes returned by malloc",
        "x_tick_interval":x_tick_interval
        }

actual_memory={
        "output_type":"png",
        "output_name":"actual_memory.png",
        "y_axis_label":"bytes per element",
        "x_axis_label":"insertions in test",
        "y_range_low":"0",
        "y_range_high":str(highest[5]),
        "x_col":7,
        "y_col":6,
        "plot_title":"Bytes returned by malloc",
        "x_tick_interval":x_tick_interval
        }






for d in [insert_speed, access_speed, minimum_memory, actual_memory]:
    with open( "/tmp/plot","w") as f:
        f.write(plot_string.format(**d))

    pipe = subprocess.Popen(["gnuplot","/tmp/plot"])
    pipe.wait()

print()
