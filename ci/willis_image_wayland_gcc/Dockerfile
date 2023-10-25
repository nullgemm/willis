FROM alpine:edge

COPY ./scripts /scripts

RUN /scripts/install.sh

CMD /scripts/build_wayland.sh release wayland native
