// AudioSync is the audio synchronization feature made for spotivids.
//
// The objective of the module is to obtain the delay between two audio files.
// In its real usage, one of them will be the YouTube downloaded video and
// another the recorded song.
//
// The math behind it is a circular cross-correlation by using Fast Fourier
// Transforms. The output should be somewhat similar to Numpy's
// correlate(data1, data2, "full").
//
// This module is in C mainly because of speed. FFTW is the fastest Fourier
// Transformation library, and Python threading can be tricky because of the
// GIL.

#include <iostream>
#include <vector>
#include <math.h>
#include <mutex>
#include <thread>
#include <fftw3.h>
#include "../include/AudioFile.h"

#ifdef PLOT
// Plotting output in debug mode.
#include "../include/matplotlibcpp.h"
namespace plt = matplotlibcpp;
#endif


// fftw_complex indexes for clarification.
#define REAL 0
#define IMAG 1


// Global mutex used in multithreading.
std::mutex GLOBAL_MUTEX;
// Sample rate used. It has to be 48000 because most YouTube videos can only
// be downloaded at that rate, and both audio files must have the same one.
#define SAMPLE_RATE 48000
// Conversion from the used sample rate to milliseconds: 48000 / 1000
#define SAMPLES_TO_MS 48

// Calculating the magnitude of a complex number.
inline double getMagnitude(double r, double i) {
    return sqrt(r*r + i*i);
}


// Concurrent implementation of the Fast Fourier Transform using FFTW.
void fft(std::vector<double> &in, fftw_complex *out) {
    fftw_plan p;
    { std::unique_lock<std::mutex> lock(GLOBAL_MUTEX);
        p = fftw_plan_dft_r2c_1d(in.size(), &in[0], out, FFTW_ESTIMATE);
    }

    fftw_execute(p);

    { std::unique_lock<std::mutex> lock(GLOBAL_MUTEX);
        fftw_destroy_plan(p);
    }
}

// The Inverse Fast Fourier Transform using FFTW. This doesn't need to be
// concurrent because it's calculated at the end.
inline void ifft(fftw_complex *in, double out[], size_t length) {
    fftw_plan p = fftw_plan_dft_c2r_1d(length, in, out, FFTW_ESTIMATE);
    fftw_execute(p);
    fftw_destroy_plan(p);
}


// Calculating the cross-correlation between two signals `a` and `b`:
//     xcross = ifft(fft(a) * magn(fft(b)))
// And where magn() is the magnitude of the complex numbers returned by the
// fft. Instead of magn(), a reverse() function could be used. But the
// magnitude seems easier to calculate for now.
//
// Both datasets should have the same size. They should be zero-padded to
// length 2N-1. This is needed to calculate the circular cross-correlation
// rather than the regular cross-correlation.
//
// TODO: Check NULL mallocs and etc.
// TODO: Get confidence level: https://dsp.stackexchange.com/questions/9797/cross-correlation-peak
//
// Returns the delay in milliseconds the second data set has over the first
// one, with a confidence between 0 and 1.
double crossCorrelation(std::vector<double> &data1, std::vector<double> &data2, size_t length, double &confidence) {
    // Getting the complex results from both FFT. The output length for the
    // complex numbers is n/2+1.
    fftw_complex *out1 = fftw_alloc_complex((length / 2) + 1);
    fftw_complex *out2 = fftw_alloc_complex((length / 2) + 1);

    std::thread fft1(&fft, std::ref(data1), std::ref(out1));
    std::thread fft2(&fft, std::ref(data2), std::ref(out2));
    fft1.join();
    fft2.join();

    // Product of fft1 * mag(fft2) where fft1 is complex and mag(fft2) is real.
    fftw_complex *in = fftw_alloc_complex(length);
    double magnitude;
    for (size_t i = 0; i < (length/2)+1; ++i) {
        magnitude = getMagnitude(out2[i][REAL], out2[i][IMAG]);
        in[i][REAL] = out1[i][REAL] * magnitude;
        in[i][IMAG] = out1[i][IMAG] * magnitude;
    }
    double *results = fftw_alloc_real(length);
    ifft(in, results, length);
    confidence = results[0];
    int delay = 0;
    for (size_t i = 1; i < length; ++i) {
        if (abs(results[i]) > confidence) {
            confidence = results[i];
            // std::cout << results[i] << " ";
            delay = i;
        }
    }

    std::cout << "Finished with confidence " << confidence << " and delay " << delay << std::endl;

#ifdef PLOT
    plt::plot(std::vector<double>(results, results + length));
    plt::show();
#endif
#ifdef DEBUG
    std::cout << delay / SAMPLES_TO_MS << "ms of delay" << std::endl;
#endif

    fftw_free(out1);
    fftw_free(out2);
    fftw_free(in);
    fftw_free(results);

    // Conversion to milliseconds with 48000KHz
    return delay / SAMPLES_TO_MS;
}


// Function to concurrently open and process both files. Takes the name of
// the file to process, and returns a vector with the normalized maximum
// frequency throughout the track.
void runProcessing(std::string name, std::vector<double> &out) {
    // Loading the WAV file
    AudioFile<double> audio;
    audio.load(name);

    // Checking that the file was read correctly
    if (audio.getNumSamplesPerChannel() == 0) {
        throw std::runtime_error("Could not read " + name);
    }

    // Only using 5 seconds (audio is 48000KHz)
    // Half the vector has to be of size 2N-1, the rest being filled with zeroes.
    out = audio.samples[0];
    int n = SAMPLE_RATE * 5 * 2;
    out.resize(n);
    for (int i = n/2; i < n; ++i)
        out.push_back(0);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file1>.wav <file2>.wav\n";
        exit(1);
    }

#ifdef DEBUG
    // Timers in debug mode
    std::clock_t start = std::clock();
#endif

    // Processing both files with multi-threading.
    std::vector<double> out1;
    std::vector<double> out2;
    std::thread t1(&runProcessing, argv[1], std::ref(out1));
    std::thread t2(&runProcessing, argv[2], std::ref(out2));
    t1.join();
    t2.join();

#ifdef DEBUG
    double duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    std::cout << "Loading WAVs took " + std::to_string(duration) + "s \n";

    // Benchmarking the matching function
    start = std::clock();
#endif

    double confidence;
    double delay = crossCorrelation(out1, out2, out1.size(), confidence);

#ifdef DEBUG
    duration = (std::clock() - start) / (double) CLOCKS_PER_SEC;
    std::cout << "Matching took " + std::to_string(duration) + "s \n";;
#endif
#ifdef PLOT
    // Plotting the output
    double samplesDelay = delay * SAMPLES_TO_MS;
    if (samplesDelay < 0) {
        out1.erase(out1.begin(), out1.size() > samplesDelay ?  out1.begin() + samplesDelay : out1.end());
    } else {
        out2.erase(out2.begin(), out2.size() > samplesDelay ?  out2.begin() + samplesDelay : out2.end());
    }
    plt::plot(out1);
    plt::plot(out2);
    plt::show();
#endif

    return 0;
}
