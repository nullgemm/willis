FROM alpine:edge

COPY ./scripts /scripts

RUN /scripts/install.sh

CMD /scripts/build_win.sh release win native
