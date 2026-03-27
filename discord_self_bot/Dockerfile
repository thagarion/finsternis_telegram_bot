FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential \
    git \
    pkg-config \
    libssl-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh \
    && chmod +x cmake-3.28.3-linux-x86_64.sh \
    && ./cmake-3.28.3-linux-x86_64.sh --skip-license --prefix=/usr/local \
    && rm cmake-3.28.3-linux-x86_64.sh

COPY . .

RUN cmake -B build -S . && cmake --build build -j

FROM ubuntu:22.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    libssl3 \
    libboost-system1.74.0 \
    libboost-log1.74.0 \
    libyaml-cpp0.7 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /app/build/discord-self-bot .

CMD ["./discord-self-bot"]
