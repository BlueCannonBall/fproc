#include <cmath>
#include <vector>
#include <array>
#include <string.h>
#include <memory>
#include <type_traits>
#include <iomanip>
#include <string>
#include "streampeerbuffer.hpp"

#ifdef __linux__
#include <endian.h>
#endif

namespace spb {
    StreamPeerBuffer::StreamPeerBuffer(bool big_endian) {
#ifdef __linux__
#if __BYTE_ORDER == __BIG_ENDIAN
        big_endian = !big_endian;
#endif
#endif
        this->big_endian = big_endian;
    }
    
    template <typename T>
    std::array<uint8_t, sizeof(T)> StreamPeerBuffer::to_bytes(const T& object) {
        std::array<uint8_t, sizeof(T)> bytes;
        union {T object; uint8_t bytes[sizeof(T)];} converter;
        converter.object = object;
        std::copy(std::begin(converter.bytes), std::end(converter.bytes),
            std::begin(bytes));
        return bytes;
    }
    
    template <typename T>
    void StreamPeerBuffer::from_bytes(const std::array<uint8_t, sizeof(T)>& bytes,
        T& object) {
        union {T object; uint8_t bytes[sizeof(T)];} converter;
        memcpy(converter.bytes, bytes.data(), sizeof(T));
        object = converter.object;
    }
    
    template <typename T>
    T StreamPeerBuffer::bswap(T val) {
        T retVal;
        int8_t *pVal = (int8_t*) &val;
        int8_t *pRetVal = (int8_t*) &retVal;
        size_t size = sizeof(T);
        for (size_t i = 0; i<size; i++) {
            pRetVal[size-1-i] = pVal[i];
        }

        return retVal;
    }
    
    void StreamPeerBuffer::put_u8(uint8_t number) {
        data_array.insert(data_array.begin() + offset, number);
        offset++;
    }
    
    void StreamPeerBuffer::put_u16(uint16_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_u32(uint32_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_u64(uint64_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_8(int8_t number) {
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_16(int16_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_32(int32_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    void StreamPeerBuffer::put_64(int64_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    
    uint8_t StreamPeerBuffer::get_u8() {
        uint8_t number;
        from_bytes({data_array[offset]}, number);
        offset++;
        return number;
    }
    
    uint16_t StreamPeerBuffer::get_u16() {
        uint16_t number;
        from_bytes({data_array[offset], data_array[offset + 1]}, number);
        offset += 2;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }
    
    uint32_t StreamPeerBuffer::get_u32() {
        uint32_t number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }
    
    uint64_t StreamPeerBuffer::get_u64() {
        uint64_t number;
        from_bytes({
            data_array[offset],
            data_array[offset + 1],
            data_array[offset + 2],
            data_array[offset + 3],
            data_array[offset + 4],
            data_array[offset + 5],
            data_array[offset + 6],
            data_array[offset + 7],
        }, number);
        offset += 8;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }
    
    int8_t StreamPeerBuffer::get_8() {
        int8_t number;
        from_bytes({data_array[offset]}, number);
        offset++;
        return number;
    }
    
    int16_t StreamPeerBuffer::get_16() {
        int16_t number;
        from_bytes({data_array[offset], data_array[offset + 1]}, number);
        offset += 2;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    int32_t StreamPeerBuffer::get_32() {
        int32_t number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    int64_t StreamPeerBuffer::get_64() {
        int64_t number;
        from_bytes({
            data_array[offset],
            data_array[offset + 1],
            data_array[offset + 2],
            data_array[offset + 3],
            data_array[offset + 4],
            data_array[offset + 5],
            data_array[offset + 6],
            data_array[offset + 7],
        }, number);
        offset += 8;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    void StreamPeerBuffer::put_string(const std::string& str) {
        std::vector<int8_t> bytes(str.begin(), str.end());
        put_u16(str.length());
        for (auto b : bytes)
            put_8(b);
    }

    std::string StreamPeerBuffer::get_string() {
        uint16_t length = get_u16();
        std::vector<int8_t> bytes;
        bytes.resize(length);
        for (uint16_t b = 0; b < length; b++)
            bytes.push_back(get_8());
        std::string str(bytes.begin(), bytes.end());
        return str;
    }

    void StreamPeerBuffer::put_float(float number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }

    float StreamPeerBuffer::get_float() {
        float number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    void StreamPeerBuffer::put_double(double number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (uint8_t b : bytes) {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }

    double StreamPeerBuffer::get_double() {
        double number;
        from_bytes({
            data_array[offset],
            data_array[offset + 1],
            data_array[offset + 2],
            data_array[offset + 3],
            data_array[offset + 4],
            data_array[offset + 5],
            data_array[offset + 6],
            data_array[offset + 7],
        }, number);
        offset += 8;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    void StreamPeerBuffer::reset() {
        offset = 0;
        data_array.clear();
    }

    size_t StreamPeerBuffer::size() {
        return data_array.size();
    }
}
