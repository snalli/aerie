# syntax=docker/dockerfile:1
# ---------------------------------------------------------------------------
# Aerie – usermode NVM filesystem
# Platform: linux/amd64 only (code uses rdtsc / SSE movnti / cpu_set_t)
# ---------------------------------------------------------------------------

# ---- stage 1: build --------------------------------------------------------
FROM --platform=linux/amd64 ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libconfig++-dev \
        libboost-dev \
        libsparsehash-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /aerie

# Copy only what the build needs (see .dockerignore for exclusions)
COPY libfs/ libfs/

RUN cmake -S libfs -B libfs/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DRPC=fast \
        -DSCMPOOL=kernel \
    && cmake --build libfs/build --parallel "$(nproc)"


# ---- stage 2: runtime ------------------------------------------------------
FROM --platform=linux/amd64 ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        libconfig++9v5 \
        libboost-system1.74.0 \
    && rm -rf /var/lib/apt/lists/*

# Binaries
COPY --from=builder /aerie/libfs/build/src/scm/pool_tool            /usr/local/bin/pool_tool
COPY --from=builder /aerie/libfs/build/src/pxfs/pxfs_server          /usr/local/bin/pxfs_server
COPY --from=builder /aerie/libfs/build/src/pxfs/pxfs_mkfs            /usr/local/bin/pxfs_mkfs
COPY --from=builder /aerie/libfs/build/src/pxfs/pxfs_client          /usr/local/bin/pxfs_client
COPY --from=builder /aerie/libfs/build/src/kvfs/kvfs_server          /usr/local/bin/kvfs_server
COPY --from=builder /aerie/libfs/build/src/kvfs/kvfs_mkfs            /usr/local/bin/kvfs_mkfs
COPY --from=builder /aerie/libfs/build/src/cfs/cfs_server            /usr/local/bin/cfs_server
COPY --from=builder /aerie/libfs/build/src/cfs/cfs_mkfs              /usr/local/bin/cfs_mkfs
COPY --from=builder /aerie/libfs/build/bench/ubench/ubench_pxfs      /usr/local/bin/ubench_pxfs
COPY --from=builder /aerie/libfs/build/bench/ubench/ubench_vfs       /usr/local/bin/ubench_vfs

# Shared libraries
COPY --from=builder /aerie/libfs/build/ /aerie/build/
RUN find /aerie/build -name "*.so" -exec cp {} /usr/local/lib/ \; && ldconfig

# Runtime config
COPY libfs/libfs.ini /etc/libfs.ini
ENV LIBFS_CONF=/etc/libfs.ini

# Pool lives on a named volume so it survives container restarts
VOLUME ["/data"]

COPY libfs/scripts/ /usr/local/bin/
RUN chmod +x /usr/local/bin/aerie-*.sh

EXPOSE 10000

ENTRYPOINT ["/usr/local/bin/aerie-server.sh"]
