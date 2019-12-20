// AudioSync is the audio synchronization feature made for spotivids. It is
// inspired by Allison Deal's VideoSync, but much faster.
// (https://github.com/allisonnicoledeal/VideoSync)
//
// This module is in C++ mainly because of speed. FFTW is the fastest Fourier
// Transformation library. Python threading can also be not real concurrency
// because of the GIL.
//
// The objective of the module is to obtain the delay between two audio files.
// In its usage, one of them will be the YouTube downloaded video and another
// the recorded song.

#include <iostream>
#include <math.h>
#include <mutex>
#include <thread>
#include <fftw3.h>
#include "../include/AudioFile.h"

#ifdef DEBUG
    #include "../include/matplotlibcpp.h"
    namespace plt = matplotlibcpp;
#endif

std::mutex GLOBAL_MUTEX;
static constexpr size_t BIN_SIZE = 1024;


// Its objective is to be as fast as possible by using FFTW and by narrowing
// down the calculation into input real values, rather than complex.
// 
// This function is also intended to be used in multi-threading, so everything
// inside it should be safe to run concurrently. This is why the returned
// data is a parameter, too. The only thread-safe function in FFTW is
// execute(), so the rest of the library usage is wrapped inside a lock.
double getMaxFreq(std::vector<double> &sample, size_t start, size_t end) {
    size_t length = end - start;
    fftw_complex *out;
    fftw_plan p;

    // fftw_complex is equivalent to double[2] where double[0] is the real
    // value and double[1] is the imaginary one.
    { std::unique_lock<std::mutex> lock(GLOBAL_MUTEX);
        out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * length);
        p = fftw_plan_dft_r2c_1d(length, &sample[start], out, FFTW_ESTIMATE);
    }

    // Actual calculation
    fftw_execute(p);

    // Getting the maximum frequency (magnitude) obtained in this interval.
    auto getMagnitude = [](double r, double i) {
        return sqrt(r*r + i*i);
    };
    double max = getMagnitude(out[0][0], out[1][0]);
    double magnitude;
    for (size_t i = 1; i < length; ++i) {
        magnitude = getMagnitude(out[0][i], out[1][i]);
        if (magnitude > max) {
            max = magnitude;
        }
    }

    // Freeing the used resources
    { std::unique_lock<std::mutex> lock(GLOBAL_MUTEX);
        fftw_destroy_plan(p);
        fftw_free(out);
    }

    return max;
}


// Getting the bins used to match both WAV files. The One-Dimensional Discrete
// Fourier Transform is calculated for each period of time, and what's
// actually returned is their peak frequency.
std::vector<double> getBins(std::vector<double> &sample) {
    // Vector where the bins are going to be stored
    std::vector<double> bins;
    // According to the docs, n/2+1 complex numbers are returned by this call.
    size_t numBins = sample.size() / 2 + 1;

    // The bin size is so small compared to the audio data that it doesn't
    // matter that the last bin is lost when it's smaller than BIN_SIZE.
    // The values have to be normalized between 0 and 1 so that they can be
    // compared later.
    size_t lastSplit = numBins - BIN_SIZE;
    double max = getMaxFreq(sample, 0, BIN_SIZE);
    double freq;
    for (size_t i = BIN_SIZE; i < lastSplit; i += BIN_SIZE) {
        freq = getMaxFreq(sample, i, i + BIN_SIZE);
        bins.push_back(freq);
        if (freq > max) {
            max = freq;
        }
    }

    if (max == 0) {
        throw std::runtime_error("The file is empty or corrupted, its maximum"
                                 " frequency is zero.");
    }

    // Normalizing the output
    for (double &el : bins) {
        el = el / max;
    }

    return bins;
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

#ifdef DEBUG
    // Printing a summary for debug
    audio.printSummary();
#endif

    // Actually doing the processing
    out = getBins(audio.samples[0]);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <file1>.wav <file2>.wav\n";
        exit(1);
    }

    // Processing both files with multi-threading.
    std::vector<double> out1;
    std::vector<double> out2;
    std::thread t1(&runProcessing, argv[1], std::ref(out1));
    std::thread t2(&runProcessing, argv[2], std::ref(out2));
    t1.join();
    t2.join();

#ifdef DEBUG
    plt::plot(out1);
    plt::plot(out2);
    plt::show();
#endif

    return 0;
}
