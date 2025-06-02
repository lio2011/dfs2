# Use an official Ubuntu base image
FROM ubuntu:24.04

# Install dependencies
RUN apt-get update && \
    apt-get install -y \
        build-essential \
        clang \
        cmake \
        ninja-build \
        libssl-dev \
        git \
        wget \
        pkg-config \
        protobuf-compiler \
        libprotobuf-dev \
        libprotoc-dev \
        libgrpc++-dev \
        grpc-proto \
        libgrpc-dev \
        python3-pip

# Install grpcio-tools for proto generation (optional, if you want to generate from .proto in container)
RUN pip3 install grpcio-tools

# Set workdir
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN mkdir -p build && cd build && \
    cmake -G Ninja .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ && \
    ninja

# Expose a range of ports for nodes (adjust as needed)
EXPOSE 50051-50060

# Default command: run the CLI
CMD ["./build/main_exec"]