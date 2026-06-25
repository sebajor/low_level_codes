import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


def animate(i):
    filename = 'build/correlation'
    f = open(filename, "rb")
    raw_data = f.read(); dat = np.frombuffer(raw_data, np.float32)
    f.close()

    dat= dat.reshape((-1, (1024*4+2)))

    pow0 = dat[-1, 2:1026]
    pow1 = dat[-1, 1026:1026+1024]
    freq = np.linspace(-1,1,1024)
    
    sample0= np.hstack([pow0[512:], pow0[:512]])
    sample1= np.hstack([pow1[512:], pow1[:512]])

    lines[0].set_data(freq, 10*np.log10(np.abs(sample0)))
    lines[1].set_data(freq, 10*np.log10(np.abs(sample1)))
    return lines



fig, axes = plt.subplots(1,2)
lines = []
for a in axes.flatten():
    a.set_ylabel('dB')
    a.set_xlabel('MHz')
    a.set_ylim(100,180)
    a.set_xlim(-1,1)
    a.grid()
    line, = a.plot([],[])
    lines.append(line)


ani = FuncAnimation(fig, animate, blit=True, interval=1000)
plt.show()
