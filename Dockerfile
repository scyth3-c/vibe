FROM debian:bookworm

RUN mkdir /app
COPY . /app
WORKDIR /app

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN cmake . && \
    cmake --build . && \
    make install

CMD ["bash"]
