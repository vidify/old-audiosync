import matplotlib.pyplot as plt
import numpy as np
import scipy.io.wavfile
import scipy.signal


# Opening the files
file2 = "audio/youtube.wav"
file1 = "audio/recorded.wav"
sample_rate = 48000
rate1, audio1 = scipy.io.wavfile.read(file1)
rate2, audio2 = scipy.io.wavfile.read(file2)
#  plt.plot(data1)
#  plt.plot(data2)
#  plt.savefig("data.png")

# Zero padding
n = 10 * sample_rate  # Seconds of audio
data1 = audio1[:n]
data1 = np.pad(data1, (0,n), 'constant')
data2 = audio2[:n]
data2 = np.pad(data2, (0,n), 'constant')

# Circular cross-correlation with FFT
fft1 = np.fft.rfft(data1)
fft2 = np.fft.rfft(data2)
products = []
for i in range(fft1.size):
    mag = abs(fft2[i])
    products.append(complex(fft1[i].real * mag, fft1[i].imag * mag)) 
result = np.fft.irfft(products)
# Getting the peak
confidence = result[0]
delay = 0
for i in range(result.size):
    if abs(result[i]) > confidence:
        confidence = result[i]
        delay = i

print(delay / (sample_rate / 1000), "milliseconds")
delay = delay if delay < n else -delay
print(delay, n)
print(confidence, "confidence")


# Plotting the results
plt.plot(result)
plt.savefig("testinverse.png")
