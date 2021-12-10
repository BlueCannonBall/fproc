#include "streampeerbuffer.hpp"
#include <array>
#include <string.h>
#include <string>
#include <vector>

#if __has_include(<endian.h>)
#include <endian.h>
#endif

namespace spb {
    StreamPeerBuffer::StreamPeerBuffer(bool big_endian) {
#if __has_include(<endian.h>)
#if __BYTE_ORDER == __BIG_ENDIAN
        big_endian = !big_endian;
#endif
#endif
        this->big_endian = big_endian;
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
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 2;
    }

    void StreamPeerBuffer::put_u32(uint32_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 4;
    }

    void StreamPeerBuffer::put_u64(uint64_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 8;
    }

    void StreamPeerBuffer::put_i8(int8_t number) {
        data_array.insert(data_array.begin() + offset, number);
        offset++;
    }

    void StreamPeerBuffer::put_i16(int16_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 2;
    }

    void StreamPeerBuffer::put_i32(int32_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 4;
    }

    void StreamPeerBuffer::put_i64(int64_t number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 8;
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
                   },
            number);
        offset += 8;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    int8_t StreamPeerBuffer::get_i8() {
        int8_t number;
        from_bytes({data_array[offset]}, number);
        offset++;
        return number;
    }

    int16_t StreamPeerBuffer::get_i16() {
        int16_t number;
        from_bytes({data_array[offset], data_array[offset + 1]}, number);
        offset += 2;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    int32_t StreamPeerBuffer::get_i32() {
        int32_t number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    int64_t StreamPeerBuffer::get_i64() {
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
                   },
            number);
        offset += 8;
        if (big_endian) {
            number = bswap(number);
        }
        return number;
    }

    void StreamPeerBuffer::put_string(const std::string& str) {
        put_u16(str.length());
        data_array.insert(data_array.begin() + offset, str.begin(), str.end());
        offset += str.length();
    }

    int StreamPeerBuffer::get_string(std::string& str) {
        uint16_t length = get_u16();
        if (length > size() - offset)
            return 1;
        std::vector<int8_t> bytes;
        for (uint16_t b = 0; b < length; b++)
            bytes.push_back(get_i8());
        str.resize(length);
        std::copy(bytes.begin(), bytes.end(), str.begin());
        return 0;
    }

    void StreamPeerBuffer::put_float(float number) {
        if (big_endian) {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 4;
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
        data_array.insert(data_array.begin() + offset, bytes.begin(), bytes.end());
        offset += 8;
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
                   },
            number);
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

    uint8_t* StreamPeerBuffer::data() {
        return data_array.data();
    }

    void StreamPeerBuffer::resize(size_t size) {
        data_array.resize(size);
    }
} // namespace spb
