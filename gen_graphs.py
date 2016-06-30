#!/usr/bin/env python3

import io
import os
import sys
import re
import subprocess

#the list of file prefixes to use
prefix="/tmp/timing_data"
lst = sorted([ int(x.split(".")[0]) for x in os.listdir(prefix) if re.match(R"^[0-9]+\.dat$", x)])

highest = [0]*20

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
                lst[4] = str( float(lst[4]) * 10**9 /fnumber)
                lst[5] = str( float(lst[5]) * 10**9 /fnumber)
                lst[7] = str( float(lst[7]) * 10**9 /fnumber)
                lst[8] = str( float(lst[8]) * 10**9 /fnumber)
                highest[2] = max( highest[2], float(lst[2]))
                highest[3] = max( highest[3], float(lst[3]))
                highest[4] = max( highest[4], float(lst[4]))
                highest[5] = max( highest[5], float(lst[5]))
                highest[7] = max( highest[7], float(lst[7]))
                highest[8] = max( highest[8], float(lst[8]))

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

plot '/tmp/data' every 4    using {x_col}:{y_col} title 'unordered map' , \\
     '/tmp/data' every 4::1 using {x_col}:{y_col} title 'ut hash', \\
     '/tmp/data' every 4::2 using {x_col}:{y_col} title 'glib', \\
     '/tmp/data' every 4::3 using {x_col}:{y_col} title 'khash'
quit
""".strip()

default={
        "output_type":"png",
        "y_range_low":"0",
        "x_col":7,
        "x_tick_interval":x_tick_interval
        }

insert_speed={
        "output_name":"insert.png",
        "y_axis_label":"ns per insertion",
        "x_axis_label":"insertions in test",
        "y_range_high":str(highest[2]),
        "y_col":3,
        "plot_title":"Insertion Time",
        }

access_speed={
        "output_name":"access.png",
        "y_axis_label":"ns per access",
        "x_axis_label":"insertions in test",
        "y_range_high":str(highest[3]),
        "y_col":4,
        "plot_title":"Access Time",
        }

minimum_memory={
        "output_name":"minimum_memory.png",
        "y_axis_label":"bytes per element",
        "x_axis_label":"insertions in test",
        "y_range_high":str(highest[4]),
        "y_col":5,
        "plot_title":"Bytes returned by malloc",
        }

actual_memory={
        "output_name":"actual_memory.png",
        "y_axis_label":"bytes per element",
        "x_axis_label":"insertions in test",
        "y_range_high":str(highest[5]),
        "y_col":6,
        "plot_title":"Bytes returned by malloc",
        }

partial_delete={
        "output_name":"partial_delete.png",
        "y_axis_label":"ns per deletion",
        "x_axis_label":"size of array",
        "y_range_high":str(highest[7]),
        "y_col":8,
        "plot_title":"Time to delete half the key/value pairs",
        }


after_delete={
        "output_name":"access_after_deletion.png",
        "y_axis_label":"ns per access",
        "x_axis_label":"size of array",
        "y_range_high":str(highest[8]),
        "y_col":9,
        "plot_title":"Time to access after partial deletion",
        }


def merge_dict(x,y):
    z = x.copy()
    z.update(y)
    return z




for d in [insert_speed, access_speed, minimum_memory, actual_memory, partial_delete, after_delete]:
    d = merge_dict(default, d)
    with open( "/tmp/plot","w") as f:
        f.write(plot_string.format(**d))

    pipe = subprocess.Popen(["gnuplot","/tmp/plot"])
    pipe.wait()

print()
