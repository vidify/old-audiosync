import time
import matplotlib.pyplot as plt
import numpy as np
import scipy.io.wavfile
import scipy.signal
from scipy.stats import pearsonr, spearmanr


# Opening the files. The actual lag is ~3648 ms.
file1 = "audio/youtube.wav"
file2 = "audio/recorded.wav"
sample_rate = 48000
rate1, audio1 = scipy.io.wavfile.read(file1)
rate2, audio2 = scipy.io.wavfile.read(file2)

# Zero padding
n = 30 * sample_rate  # Seconds of audio
data1 = audio1[:n]
data1 = np.pad(data1, (0,n), 'constant')
data2 = audio2[:n]
data2 = np.pad(data2, (0,n), 'constant')
#  plt.plot(data1)
#  plt.plot(data2)
#  plt.show()


# METHOD 1: cross-correlation method from numpy that calculates a
# time-dependent result with the cross-correlation coefficients (slower).
# print("Executing np correlate")
# out = np.correlate(data1, data2, "full")
# plt.plot(out)
# plt.show()

# METHOD 2: using the formula with FFT. We can either use FFT and IFFT, or
# save half the computation time by using RFFT and IRFFT, since the wave only
# has real values.
print("Executing with the formula")
start_time = time.time()

fft1 = np.fft.rfft(data1)
fft2 = np.fft.rfft(data2)
products = fft1 * np.conjugate(fft2)
result = np.fft.irfft(products)
# Getting the peak
lag = np.argmax(np.abs(result))
print(lag / (sample_rate / 1000), "milliseconds")

# Matching the files
result1 = data1[:data1.size//2]
result2 = np.roll(data2[:data2.size//2], lag)

# Calculating the Pearson coefficient
corr, pvalue = pearsonr(result1, result2)
print("Result obtained in", time.time() - start_time, "secs")

# Plotting the results
#  plt.plot(result1)
#  plt.plot(result2)
#  plt.show()
