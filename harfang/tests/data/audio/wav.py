import math
import wave
import struct
import argparse

if __name__ == '__main__':
        default_duration = 1.0
        default_tone = 440
        default_framerate = 44100
        default_channels = 2
        default_bitdepth = 16
        default_volume = 80.0

        parser = argparse.ArgumentParser()
        parser.add_argument('out', help='Output filename')
        parser.add_argument('--duration', required=False, help='Sample duration in seconds (default: %f)' % default_duration, default=default_duration)
        parser.add_argument('--frequency', required=False, help='Tone frequency Hz (default: %d)' % default_tone, default=default_tone)
        parser.add_argument('--framerate', required=False, help='Sampling rate in Hz (default: %d)' % default_framerate, default=default_framerate)
        parser.add_argument('--channels', required=False, help='Channel count (default: %d)' % default_channels, default=default_channels)
        parser.add_argument('--bitdepth', required=False, help='Bit depth (default: %d)' % default_bitdepth, default=default_bitdepth)
        parser.add_argument('--volume', required=False, help='Volume in dB (default: %f)' % default_volume, default=default_volume)
        
        args = vars(parser.parse_args())

        amplitude = math.pow(10.0, float(args['volume'])/20.0)
        frequency = float(args['frequency'])
        framerate = float(args['framerate'])
        channels = int(args['channels'])
        resolution = int(args['bitdepth'] / 8)
        duration = float(args['duration'])
        length = int(framerate * duration)
        
        div = length / channels

        out = wave.open(args['out'], "wb")
        out.setparams((channels, resolution, framerate, length, "NONE", "not compressed"))
        for i in range(length):
                sample = amplitude / 2.0 * math.sin(2.0 * math.pi * frequency * i / framerate)
                for j in range(channels):
                        j0 = div * j
                        j1 = j0 + div
                        data = struct.pack('h', int(sample) if (i >= j0) and (i < j1) else 0)
                        out.writeframes(data)
        out.close()
