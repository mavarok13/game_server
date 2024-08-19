FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Release

COPY ./src /app/src
COPY ./tests /app/tests

COPY CMakeLists.txt /app/
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release ..

RUN cd /app/build && cmake --build . -t game_server

FROM ubuntu:22.04 as run

RUN mkdir /app/ && mkdir /app/game_server_saves
RUN chmod 777 /app/game_server_saves

#RUN mkdir /tmp/volume && chmod 777 /tmp/volume

RUN groupadd -r www && useradd -r -g www www
USER www 

COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

ENTRYPOINT ["/app/game_server", "-c", "/app/data/config.json", "-w", "/app/static", "-t", "50", "--randomize-spawn-points", "--state-file", "/app/game_server_saves", "--save-state-period", 5000]
