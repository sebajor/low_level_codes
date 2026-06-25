import subprocess
import numpy as np
import matplotlib.pyplot as plt
import time
import os


###
###
fstart = 1.947
fstop = 1.957
step = 0.0001

sleep_time = 4

cmd = "build/phase_correlator -gain 40 -batch_integration 20 -flo " #1952100000.0
output_file = "correlation"


###
###

os.makedirs('images', exist_ok=True)

points = int((fstop-fstart)/step)+1
freqs = (np.linspace(fstart, fstop, points)*1e9).astype(int)*1.0

plot_freq = np.linspace(-1,1, 1024)
output = np.zeros((points, 1024))

for i in range(len(freqs)):
	cmd_freq = cmd+str(freqs[i])
	proc = subprocess.Popen(cmd_freq.split(' '))
	time.sleep(sleep_time)
	proc.terminate()
	f = open(output_file, 'rb')
	raw_data = f.read(); dat = np.frombuffer(raw_data, np.float32)
	f.close()
	dat= dat.reshape((-1, (1024*4+2)))
	pow0 = dat[-1, 2:1026]
	output[i,:] = pow0
	time.sleep(sleep_time)
	plt.plot(plot_freq, 10*np.log10(np.hstack([pow0[512:], pow0[:512]])))
	plt.ylim(100,160)
	plt.grid()
	plt.title('LO:'+str(freqs[i]))
	plt.savefig('images/{:05d}.png'.format(i), dpi=100)
	plt.close()





