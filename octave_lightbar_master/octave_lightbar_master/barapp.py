# Change sys path to properly import Realtime_PyAudio_FFT submodules from src dir.
import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "submodules/Realtime_PyAudio_FFT")))
from submodules.Realtime_PyAudio_FFT.src.stream_analyzer import Stream_Analyzer

import argparse
import time

import struct
import construct
import numpy as np
import serial
import crc16
from src.RS485Manager import RS485_bus

# ========= DEBUG ONLY ==========
import matplotlib.pyplot as plt 
from matplotlib.animation import FuncAnimation

# ======= CONFIG. PARAMS ========
# How often FFT features + Slave Boards are commanded
k_freq = 45 # Hz
k_sample_rate = 1 / k_freq # Sec
# Compose Configuration Parameters for Slave Boards
all_connected_slave_addresses = [0x05, 0x0a]      # List consisting of Slave Board addresses connected to RS485 BUS
k_pixel_brightness = 40 # uint8t [0-255]
k_config_vec = [k_pixel_brightness]


# ====== TEMPORARY JERRY-RIG ======
k_max_binned_value = 22
k_byte_max = 255


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--device', type=int, default=None, dest='device',
                        help='pyaudio (portaudio) device index')
    parser.add_argument('--height', type=int, default=450, dest='height',
                        help='height, in pixels, of the visualizer window')
    parser.add_argument('--n_frequency_bins', type=int, default=400, dest='frequency_bins',
                        help='The FFT features are grouped in bins')
    parser.add_argument('--verbose', action='store_true')
    parser.add_argument('--window_ratio', default='24/9', dest='window_ratio',
                        help='float ratio of the visualizer window. e.g. 24/9')
    parser.add_argument('--sleep_between_frames', dest='sleep_between_frames', action='store_true',
                        help='when true process sleeps between frames to reduce CPU usage (recommended for low update rates)')
    return parser.parse_args()

def convert_window_ratio(window_ratio):
    if '/' in window_ratio:
        dividend, divisor = window_ratio.split('/')
        try:
            float_ratio = float(dividend) / float(divisor)
        except:
            raise ValueError('window_ratio should be in the format: float/float')
        return float_ratio
    raise ValueError('window_ratio should be in the format: float/float')

def run_lightbar():
    args = parse_args()
    window_ratio = convert_window_ratio(args.window_ratio)

    # Launch Communications Bus
    main_comms_bus = RS485_bus('/dev/ttyUSB0', all_connected_slave_addresses, slave_config_params = k_config_vec)

    # Launch Stream Analyzer
    ear = Stream_Analyzer(
                    device = args.device,        # Pyaudio (portaudio) device index, defaults to first mic input
                    rate   = None,               # Audio samplerate, None uses the default source settings
                    FFT_window_size_ms  = 60,    # Window size used for the FFT transform
                    updates_per_second  = 500,   # How often to read the audio stream for new data
                    smoothing_length_ms = 50,    # Apply some temporal smoothing to reduce noisy features
                    n_frequency_bins = 8,        # The FFT features are grouped in bins
                    visualize = 0,               # Visualize the FFT features with PyGame
                    verbose   = args.verbose,    # Print running statistics (latency, fps, ...)
                    height    = args.height,     # Height, in pixels, of the visualizer window,
                    window_ratio = window_ratio  # Float ratio of the visualizer window. e.g. 24/9
                    )

    last_update = time.time()
    print("All ready, starting audio measurements now...")
    fft_samples = 0

    # DEBUG PLOTTING
    # x = np.random.rand(1203)
    # y = np.random.rand(1203)
    # fig, ax = plt.subplots()
    # line, = ax.plot(x,y)
    # plt.show()

    max_binned_fft = 0

    while True:
        if (time.time() - last_update) > (1./k_freq):
            last_update = time.time()
            raw_fftx, raw_fft, binned_fftx, binned_fft = ear.get_audio_features()
            fft_samples += 1

            # Rescale, round to int & clip binned_fft vals
            rescaled_bins_fft = np.clip(np.round(binned_fft / k_max_binned_value * k_byte_max).astype(int), 0, 255)

            for slave_id in [10,5]:
                # Build command vectors
                if slave_id == 10:
                    cmd_vec = rescaled_bins_fft[:4].copy() 
                if slave_id == 5:
                    cmd_vec = rescaled_bins_fft[4:].copy() 
                # SEND command to slave address
                main_comms_bus.send_cmd(slave_id,cmd_vec)


            # # PRINT PROPERTIES
            # if fft_samples == 1:
            #     print(f"RAW_FFTX {raw_fftx};")
            #     print(f"BINS {binned_fftx};")

            # # Find maximum value
            # if max_binned_fft < np.max(binned_fft):
            #     max_binned_fft = np.max(binned_fft)

            # # DISPLAY PARAMS 
            # if fft_samples % 10 == 0:
            #    print(f"Got binned_fft_scaled {rescaled_bins_fft}")
            #    print(f"Max binned_fft is {max_binned_fft}")

            
        elif args.sleep_between_frames:
            time.sleep(((1./k_freq)-(time.time()-last_update)) * 0.99)

if __name__ == '__main__':
    run_lightbar()
