import numpy as np
import matplotlib.pyplot as plt
import datetime
import argparse

parser = argparse.ArgumentParser(
    description="Script to plot the current saved data")

parser.add_argument("-f", "--filename", dest="filename", default="../build/correlation", type=str,
    help="filenamme with the correlated data")

parser.add_argument("-w", "--window", dest="window", default=100, type=int,
    help="window size saved")



args = parser.parse_args()
filename = args.filename#"correlation"

f = open(filename, "rb")
raw_data = f.read(); dat = np.frombuffer(raw_data, np.float32)
f.close()

window = args.window

wind_plot = 20


dat= dat.reshape((-1, (window*4+2)))

pow0 = dat[:, 2:2+window]
pow1 = dat[:, 2+window:2+window*2]
corr = dat[:, 2+window*2:2+window*3]+1j*dat[:,2+window*3:2+window*4]
stamp = dat[:,:2].view(np.int64)[:,0]

date = [datetime.datetime.fromtimestamp(x.view(np.int64)/1e6) for x in stamp]

plt.figure()
###this was only when you have the FFT unwrapped
#sample0= np.hstack([pow0[-1,512:], pow0[-1, :512]])
#sample1= np.hstack([pow1[-1,512:], pow1[-1, :512]])
#freq = np.linspace(-1,1, 1024)

sample0= pow0[-1,:]
sample1= pow1[-1,:]
freq = np.linspace(0,1, 1024)[:window]

plt.plot(freq, 10*np.log10(sample0), label='antenna1')
plt.plot(freq, 10*np.log10(sample1), label='antenna2')
plt.xlabel('MHz')
plt.ylabel('dB')
plt.grid()
plt.legend()


##just check for the maximum value
peak = 20#np.argmax(pow0[0,:])
phase = np.unwrap(np.rad2deg(np.angle(np.sum(corr[:, peak-wind_plot//2:peak+wind_plot//2], axis=1))))
pow_diff = 10*(np.log10(np.abs(np.sum(pow0[:,peak-wind_plot//2:peak+wind_plot//2], axis=1)))-np.log10(np.abs(np.sum(pow1[:, peak-wind_plot//2:peak+wind_plot//2], axis=1))))

fig, axes = plt.subplots(2,1, sharex=1)
axes[0].plot(date, pow_diff)
axes[1].plot(date, phase)
axes[0].grid()
axes[1].grid()

axes[0].set_ylabel('dB')
axes[1].set_ylabel('deg')

plt.show()


