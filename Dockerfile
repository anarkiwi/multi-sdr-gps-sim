FROM ubuntu:24.04 AS builder

RUN apt-get -yq update && apt-get install -yq build-essential bison flex libcurl4-gnutls-dev libncurses-dev libxml2 libxml2-dev libcdk5-dev libusb-1.0-0-dev libuhd-dev graphviz
COPY . /src
WORKDIR /src
RUN make all 

FROM ubuntu:24.04
RUN apt-get -yq update && apt-get install -yq libcurl3t64-gnutls libncurses6 uhd-host
COPY --from=builder /src/gps-sim /usr/local/bin
RUN /usr/local/bin/gps-sim --help
