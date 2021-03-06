/*
  Names:  Ang Li  Matthew Braun
  IDs:    1550746 1497171
  CMPUT 275, Winter 2019

  Project: audio visualizer
*/
#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>
#include "audio_file.h"

int main(int argc, char *argv[]) {
    // invalid usage
    if (argc != 2) {
        cout << "usage: " << argv[0] << " input_audio" << endl;
        exit(1);
    }

    audio_file input_file(argv[1], 44100);

    // array holding the transformed data
    double* data;
    // size of the data array
    unsigned long size;
    // decode & re-sample the audio file into time domain
    input_file.decode(&data, &size);

    double sample_rate = 44100;

    double timeIndex, freqIndex;

    // transform the data to frequency domain
    vector<vector<double>> f_bins_collection = input_file.f_domain(&data, &size);

    // investgated frequencies
    vector<double> freqVec = {
        27.50,29.14,30.87,
        32.7,34.65,36.71,38.89,41.2,43.65,46.25,49,51.91,55,58.27,61.74,
        65.41,69.3,73.42,77.78,82.41,87.31,92.5,98,103.83,110,116.54,123.47,
        130.81,138.59,146.83,155.56,164.81,174.61,185,196,207.65,220,233.08,246.94,
        261.63,277.18,293.66,311.13,329.63,349.23,369.99,392,415.3,440,466.16,493.88,
        523.25,554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880,932.33,987.77,
        1046.50,1108.73,1174.66,1244.51,1396.91,1479.98,1567.98,1661.22,1760,1864.66,1975.53
    };

    // extract the behavior of certain frequencies
    vector<vector<double>> new_f_bins_collection = input_file.filter(f_bins_collection, freqVec);
    vector<double>::iterator thisFreq;

    // write the data into output.txt
    ofstream output;
    output.open("output.txt");
    output << new_f_bins_collection.size() << "\n" << new_f_bins_collection[0].size() << "\n";
    for (timeIndex = 0; timeIndex < new_f_bins_collection.size(); timeIndex += 1)
    {
        output << "NEXT_TIME_INTERVAL\n" << (timeIndex * 2048 / sample_rate) << "\n";
        freqIndex = 0;
        thisFreq = freqVec.begin();
        for (auto iter : new_f_bins_collection[timeIndex]) {
            output << *thisFreq << ":" << iter << "\n";
            thisFreq++;
            freqIndex += 1;
        }
    }

    return 0;
}
