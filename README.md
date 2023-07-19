# A Quic Measurement

Simple C programs to make a Quic connection using the msquic library.
https://github.com/microsoft/msquic

## Requiremnts

The following are required to build the project:

- cmake
- msquic library

To install msquic:

```
git clone https://github.com/microsoft/msquic
```
```
cd msquic 
```
```
git submodule update --init --recursive
```
```
mkdir build && cd build
```
```
cmake ..
```
```
make
```

## Build

To build the project

```
mkdir build && cd build
cmake ..
```

(Note: for any changes in the programs, there is no need of removing the build file, just run "cmake .." in the build folder)

## Usage

In progress
