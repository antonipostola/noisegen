# Noise Generator
This is a simple audio noise generator which uses leaky integration of white noise. The program allows for specifying the duration, order of integration, and leak factor of the noise file, which is stored in WAV format.

## Compilation
The program has no external dependencies and relies only on the C standard library. Therefore, I have not included a build system. To compile, simply add `noisegen.c` as a source, `noisegen` as the executable, and link to libm.

On UNIX based systems:
```
cc -o noisegen noisegen.c -lm
```

## Usage
```
Usage: noisegen [OPTION]... FILE
Generates audio noise using leaky integration and stores the result in WAV format.
Example: noisegen --duration 30.0 --order 5 --leak 0.98 output.wav

Options:
  -h, --help               Display this help dialog and exit

  -t, --duration SECONDS   The duration of the output in seconds (float, default: 10.0)

  -n, --order N            The order of integration of the noise (non-negative integer, default: 1)
                           E.g. white noise = 0, brown noise = 1

  -l, --leak FACTOR        The leak factor of the leaky integrator (float between 0.0 and 1.0, default: 0.99)
```
