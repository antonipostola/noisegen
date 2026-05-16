# Noise Generator
This is a simple audio noise generator which uses leaky integration of white noise. The program allows for specifying the duration, order of integration, and leak factor of the noise file, which is stored in WAV format.

## Compilation
The program has no external dependencies and relies only on the C standard library. Therefore, I have not included a build system. To compile, simply add `noisegen.c` as a source, `noisegen` as the executable, and link to libm.

On UNIX based systems:
```
cc -o noisegen noisegen.c -lm
```

On Windows:
```
# using gcc
gcc -o noisegen.exe noisegen.c -lm

# using msvc
cl /Fe:noisegen.exe noisegen.c
```

## Usage
```
Usage: noisegen [OPTION]... FILE
Generates audio noise using leaky integration and stores the result in WAV format.
Example: noisegen --duration 30.0 --order 5 --leak 0.98 output.wav

Options:
  -h, --help               Display this help dialog and exit

  -t, --duration SECONDS   The duration of the output in seconds (float between 0 and 20,000, default: 10.0)

  -n, --order N            The order of integration of the noise (non-negative integer, default: 1)
                           E.g. white noise = 0, brown noise = 1

  -l, --leak FACTOR        The leak factor of the leaky integrator (float between 0.0 and 1.0, default: 0.99)
```

## Additional Notes
The sound produced by this program is peak normalised to about -6dB. This normalisation is operated on segments of 5 seconds and smoothened between them. A quirk of this is that a large sweeping modulation effect may be present in the noise depending on the parameters set. Whether that is a positive thing is up to you to decide.

Also, the program was made assuming a little-endian system, therefore running it on a big-endian system will produce a corrupt WAV file.
