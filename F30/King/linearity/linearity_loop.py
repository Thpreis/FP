#!/usr/bin/python

import numpy as np
import os
import time

lower =0.1
upper =20
#timeStep =
Nsteps = 15

fileName = "flatVRange"
obsNames = "TobiasThimo"

#times = np.arange(lower, upper, timeStep)
times = np.linspace(lower,upper,Nsteps)
for timex in times:
    os.system('ccdread -C 0 -b 2 -g 5 -T -F {fileName} -e {expTime} -o {fileName}{expTime} -u {obsNames} -O -C 1'.format(fileName=fileName, expTime=str(timex), obsNames=obsNames))
    print "Preparing for next image - don't interrupt now!"

