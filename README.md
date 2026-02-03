# Sparrow: Lossless Floating Point Time Series Compression For Signals With Sparse Frequency Represenation

## Abstract

Domain specific data compression fills an increasingly crucial role as all-purpose algorithms struggle to achieve optimal compression ratios for specialized data types and formats. The more information about the data that is compressed is available, the more effectively these features can be exploited to achieve higher compression ratios. One such example is the state of the art algorithm "Gorilla", which was published by Meta in 2015. It uses the fact that, for most time series, consecutive values are close to each other or even equal. Their XOR-based approach teaches how to utilize this property to create long series of zeros and encode them in a way to reduce storage space. *Sparrow* too reduces the average absolute value stored, but the approach is different: it approximates a time series by identifying and storing the most dominant frequency components and then only saves the residual gained from an XOR operation for each data point. This approach works best for signals with a sparse frequency domain.

## Implementation

The implementation of the algorithms is done in C++. This includes *Sparrow*, Gorilla, *Sparrow* + *Elf* and *Gorilla* + Elf. To compile everything, the script `compile.bat` can be used. The file `compare.exe`/`compare.cpp` takes the id of an algorithm as parameter and uses that to compress the file at `data/signal_data.txt` into `data/code.bin` and compares the compression ratios. It also measures the runtime of both encoder and decoder, checks for any reconstruction error and writes the results to a `.json` file.

The evaluations are done by a separate group of python scripts. The `evaluation/evaluate.py` offers the base methods for evaluation and uses them to conduct a fundamental comparison. The `data_client.py` is responsible for fetching the real world data used for evaluation. API keys are stored in `evaluation/.env`, which is not visible on the repository for obvious reasons.

### Build Dependencies

#### C++

The implementation requires the following external libraries:

- **FFTW3** -- Fast Fourier Transform library for frequency analysis
- **zlib** -- General-purpose compression library
- **LZ4** -- Fast compression algorithm
- **Zstandard** -- High-ratio compression by Facebook

The code is compiled with GCC using C++17 standard.

#### Python

The following dependencies need to be installed:

- NumPy -- numerical computing
- Pandas -- data manipulation
- SciPy -- scientific computing
- Matplotlib -- visualization
- Requests -- HTTP requests
- Python-dotenv -- environment variables

Using pip this can be done with:

`pip install -r evaluation/requirements.txt`