import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import argparse


parser = argparse.ArgumentParser(
    description="Script to plot the current saved data")

parser.add_argument("-f", "--filename", dest="filename", default="../build/correlation", type=str,
    help="filenamme with the correlated data")

parser.add_argument("-w", "--window", dest="window", default=100, type=int,
    help="window size saved")



args = parser.parse_args()
window = args.window

fig, axes = plt.subplots(1,2)
lines = []
for a in axes.flatten():
    a.set_ylabel('dB')
    a.set_xlabel('MHz')
    a.set_ylim(100,180)
    a.set_xlim(0, window)
    a.grid()
    line, = a.plot([],[])
    lines.append(line)



def animate(i):
    filename = args.filename
    window = args.window
    f = open(filename, "rb")
    raw_data = f.read(); dat = np.frombuffer(raw_data, np.float32)
    f.close()
    dat= dat.reshape((-1, (window*4+2)))
    pow0 = dat[-1, 2:2+window]
    pow1 = dat[-1, 2+window:2+window*2]
    sample0= pow0
    sample1= pow1

    lines[0].set_data(np.arange(window), 10*np.log10(np.abs(sample0)))
    lines[1].set_data(np.arange(window), 10*np.log10(np.abs(sample1)))
    return lines

ani = FuncAnimation(fig, animate, blit=True, interval=1000)
plt.show()
