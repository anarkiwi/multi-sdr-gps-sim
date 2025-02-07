FROM ubuntu:24.04 AS builder

RUN apt-get -yq update && apt-get install -yq build-essential bison flex libcurl4-gnutls-dev libncurses-dev libxml2 libxml2-dev libcdk5-dev libusb-1.0-0-dev libuhd-dev graphviz
COPY . /src
WORKDIR /src
RUN make all 

FROM ubuntu:24.04
RUN apt-get -yq update && apt-get install -yq libcurl3t64-gnutls libncurses6 uhd-host
ENV UHD_IMAGES_DIR=/usr/share/uhd/images
ENV PATH="${PATH}:/root/.local/bin"
RUN /usr/bin/uhd_images_downloader -t "b2|usb"
COPY --from=builder /src/gps-sim /usr/local/bin
RUN /usr/local/bin/gps-sim --help

# docker build -f Dockerfile . -t gps-sim
# docker run --privileged -v /dev/bus/usb:/dev/bus/usb -ti gps-sim /usr/local/bin/gps-sim -r uhd -3 -e /path/to/rnx_file.rnx --disable-almanac  --verbose
