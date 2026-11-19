# syntax=docker/dockerfile:1.7
#
# Commit image for DevilutionX (savantenvs/DevilutionX, branch `mayhem`).
# Builds upstream's own CMake project with the fuzz harnesses under mayhem/fuzz/ hooked in,
# plus the upstream standalone gtest suite used by mayhem/test.sh.
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# System dependencies upstream's CMake finds via find_package (SDL2, SDL2_image, libsodium,
# bzip2, zlib, Lua 5.4, googletest, google-benchmark) come from apt.
USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
        libsdl2-dev libsdl2-image-dev libsodium-dev libbz2-dev zlib1g-dev liblua5.4-dev \
        libgtest-dev libgmock-dev libbenchmark-dev \
 && rm -rf /var/lib/apt/lists/*

# Dependencies upstream's CMake downloads with FetchContent (3rdParty/<name>/CMakeLists.txt):
# pre-fetched from the same pinned URLs (sha256 checked; upstream's MD5s match too) into
# /opt/toolchains/deps/<name>-<rev>/src. build.sh points FETCHCONTENT_SOURCE_DIR_<NAME> at them
# and sets FETCHCONTENT_FULLY_DISCONNECTED=ON, so the build never touches the network.
RUN set -eu; fetch() { \
      d="/opt/toolchains/deps/$1/src"; mkdir -p "$d"; \
      curl -fsSL -o /tmp/dep.tgz "$2"; echo "$3  /tmp/dep.tgz" | sha256sum -c -; \
      tar -xzf /tmp/dep.tgz --strip-components=1 -C "$d"; rm -f /tmp/dep.tgz; }; \
    fetch asio-112f011 https://github.com/diasurgical/asio/archive/112f011842f4ae713b158a0fc57009f95c761a21.tar.gz 2f56a72c0ba18205b0b876f0918af5c918e69e0dd803b8c325bb2a5463bf66c6; \
    fetch magic_enum-0.9.7 https://github.com/Neargye/magic_enum/archive/refs/tags/v0.9.7.tar.gz b403d3dad4ef542fdc3024fa37d3a6cedb4ad33c72e31b6d9bab89dcaf69edf7; \
    fetch mpqfs-1dc2407 https://github.com/diasurgical/mpqfs/archive/1dc2407a9cd97eb7aaf8a0c184c70436871b7e5a.tar.gz bdeb4631c3cc3f936701b2368ee45d47b2e46ac67066eeea6a32637749be824d; \
    fetch sol2-832ac77 https://github.com/diasurgical/sol2/archive/832ac772c2cd3d9620d447e9e77897f7b5e806e3.tar.gz c91d48ad1259b57d4ff1a70eee90c113eafb749384f07d95f5bc10b5e8916699; \
    fetch SheenBidi-2.9.0 https://github.com/Tehreer/SheenBidi/archive/refs/tags/v2.9.0.tar.gz e90ae142c6fc8b94366f3526f84b349a2c10137f87093db402fe51f6eace6d13; \
    fetch unordered_dense-4.4.0 https://github.com/martinus/unordered_dense/archive/refs/tags/v4.4.0.tar.gz 3976399793e8cb4db1409ce15610fbd9e5e406ced4745f262d393a9311efbd88; \
    fetch libsmackerdec-0aaaf8c https://github.com/diasurgical/libsmackerdec/archive/0aaaf8c94a097b009d844db0d44dd7cd0ff81922.tar.gz c3a7fbc91531eee8989b1d281415adc0209d84ffc0437c6b0b4f677ae7fd3b41; \
    chmod -R a+rX,go-w /opt/toolchains/deps

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
