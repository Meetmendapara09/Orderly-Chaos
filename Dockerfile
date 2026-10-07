# Orderly Chaos runtime image.
#
# Builds the orderly-chaos Python package from source (compiling the C++
# extension) and ships it with the runnable examples. The default command
# runs the quickstart demo.
#
#   docker build -t orderly-chaos .
#   docker run --rm orderly-chaos
#   docker run --rm orderly-chaos python examples/python/replay_events.py
#
# Run the test suite inside the build:
#
#   docker build --target test -t orderly-chaos-test .

# syntax=docker/dockerfile:1

ARG PYTHON_VERSION=3.12

FROM python:${PYTHON_VERSION}-slim AS builder
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY pyproject.toml setup.py MANIFEST.in README.md CHANGELOG.md LICENSE THIRD_PARTY_NOTICES.md ./
COPY include/ include/
COPY src/ src/
COPY third_party/ third_party/
COPY python/ python/
RUN pip install --no-cache-dir --prefix=/install .
ENV PYTHONPATH=/install/lib/python3.12/site-packages

FROM builder AS test
COPY tests/ tests/
COPY examples/ examples/
RUN pip install --no-cache-dir pytest \
    && python -m pytest tests/python -q

FROM python:${PYTHON_VERSION}-slim AS runtime
LABEL org.opencontainers.image.title="Orderly Chaos"
LABEL org.opencontainers.image.description="A fast price-time priority limit order book with C++, C, and Python APIs."
LABEL org.opencontainers.image.authors="Meet Mendapara"
LABEL org.opencontainers.image.licenses="MIT"
LABEL org.opencontainers.image.source="https://github.com/Meetmendapara09/Orderly-Chaos"
COPY --from=builder /install /usr/local
WORKDIR /app
COPY examples/ examples/
CMD ["python", "examples/python/quickstart.py"]
