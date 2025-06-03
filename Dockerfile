FROM ubuntu:22.04

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
        ca-certificates

# Build and install gRPC (and its dependencies) from source
RUN git clone --recurse-submodules -b v1.48.0 https://github.com/grpc/grpc.git /tmp/grpc && \
    cd /tmp/grpc && \
    mkdir -p build && cd build && \
    cmake -DgRPC_BUILD_TESTS=OFF -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ .. && \
    make -j$(nproc) && \
    make install && \
    ldconfig && \
    cd / && rm -rf /tmp/grpc

WORKDIR /app

COPY . .

# Generate C++ sources from proto using the in-container protoc
RUN protoc -I./include ./include/dht.proto \
    --cpp_out=./include \
    --grpc_out=./include \
    --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin

RUN mkdir -p build && cd build && \
    cmake -G Ninja .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ && \
    ninja

EXPOSE 50051-50060

CMD ["./build/main_exec"]