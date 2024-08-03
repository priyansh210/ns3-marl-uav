ARG BASE=defiance-full

FROM --platform=amd64 $BASE

COPY . contrib/defiance

# unset if you want to skip building
ARG BUILD_NS3=True

RUN if [ "$BUILD_NS3" = "True" ]; then ./ns3 build; fi
