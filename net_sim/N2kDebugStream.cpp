#include <cstdio>
#include "N2kDebugStream.h"

int N2kDebugStream::read() {
    return 0;
}

int N2kDebugStream::peek() {
    return 0;
}

size_t N2kDebugStream::write(const uint8_t *data, size_t size) {
    printf("%.*s", int(size), data);
    return size;
}
