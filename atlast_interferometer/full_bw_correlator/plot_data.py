import numpy as np
import matplotlib.pyplot as plt
import datetime

filename = "build/correlation"
samp_wind = 30

f = open(filename, "rb")
raw_data = f.read(); dat = np.frombuffer(raw_data, np.float32)
f.close()

dat= dat.reshape((-1, (1024*4+2)))

pow0 = dat[:, 2:1026]
pow1 = dat[:, 1026:1026+1024]
corr = dat[:,2+1024*2:2+1024*3]+1j*dat[:,2+1024*3:2+1024*4]
stamp = dat[:,:2].view(np.int64)[:,0]

date = [datetime.datetime.fromtimestamp(x.view(np.int64)/1e6) for x in stamp]

plt.figure()
sample0= np.hstack([pow0[-1,512:], pow0[-1, :512]])
sample1= np.hstack([pow1[-1,512:], pow1[-1, :512]])
freq = np.linspace(-1,1, 1024)

plt.plot(freq, 10*np.log10(sample0), label='antenna1')
plt.plot(freq, 10*np.log10(sample1), label='antenna2')
plt.xlabel('MHz')
plt.ylabel('dB')
plt.grid()
plt.legend()


##just check for the maximum value
peak = np.argmax(pow0[-1,:])
phase = np.rad2deg(np.angle(np.sum(corr[:, peak-samp_wind//2:peak+samp_wind//2], axis=1)))
phase_unwrap = np.rad2deg(np.unwrap(np.angle(np.sum(corr[:, peak-samp_wind//2:peak+samp_wind//2], axis=1))))
pow_diff = 10*(np.log10(np.abs(np.sum(pow0[:,peak-samp_wind//2:peak+samp_wind//2], axis=1)))-np.log10(np.abs(np.sum(pow1[:, peak-samp_wind//2:peak+samp_wind//2], axis=1))))

fig, axes = plt.subplots(3,1, sharex=1)
axes[0].plot(date, pow_diff)
axes[1].plot(date, phase)
axes[2].plot(date, phase_unwrap)
axes[0].grid()
axes[1].grid()
axes[2].grid()

axes[0].set_ylabel('dB')
axes[1].set_ylabel('deg')
axes[2].set_ylabel('deg unwrapped')

plt.show()


