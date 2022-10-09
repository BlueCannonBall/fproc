#include "streampeerbuffer.hpp"
#include <array>
#include <string.h>
#include <string>
#include <vector>

#if __has_include(<endian.h>)
    #include <endian.h>
#elif __has_include(<machine/endian.h>)
    #include <machine/endian.h>
#endif

namespace spb {
    StreamPeerBuffer::StreamPeerBuffer(bool big_endian) {
#if __BYTE_ORDER == __BIG_ENDIAN
        this->swap_endian = !big_endian;
#else
        this->swap_endian = big_endian;
#endif
    }

    void StreamPeerBuffer::put_u8(uint8_t num) {
        data_array.insert(data_array.begin() + offset, num);
        offset++;
    }

    void StreamPeerBuffer::put_u16(uint16_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[2];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 2;
    }

    void StreamPeerBuffer::put_u32(uint32_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[4];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 4;
    }

    void StreamPeerBuffer::put_u64(uint64_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[8];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 8;
    }

    void StreamPeerBuffer::put_varuint(uint64_t num) {
        do {
            uint8_t byte = num & 0b01111111;
            num >>= 7;
            if (num)
                byte |= 0x80;
            put_u8(byte);
        } while (num);
    }

    void StreamPeerBuffer::put_i8(int8_t num) {
        data_array.insert(data_array.begin() + offset, num);
        offset++;
    }

    void StreamPeerBuffer::put_i16(int16_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[2];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 2;
    }

    void StreamPeerBuffer::put_i32(int32_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[4];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 4;
    }

    void StreamPeerBuffer::put_i64(int64_t num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[8];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 8;
    }

    void StreamPeerBuffer::put_varint(int64_t num) {
        int more = 1;
        bool negative = num < 0;
        int size = 64 - __builtin_clzl(num);

        while (more) {
            uint8_t byte = num & 0b01111111;
            num >>= 7;

            if (negative)
                num |= (~0 << (size - 7)); // sign extend

            // sign bit of byte is second high-order bit (0x40)
            if ((num == 0 && !(byte & 0x40)) || (num == -1 && (byte & 0x40))) {
                more = 0;
            } else {
                byte |= 0x80;
            }

            put_u8(byte);
        }
    }

    uint8_t StreamPeerBuffer::get_u8() {
        uint8_t num = from_bytes<uint8_t>(&data_array[offset]);
        offset++;
        return num;
    }

    uint16_t StreamPeerBuffer::get_u16() {
        uint16_t num = from_bytes<uint16_t>(&data_array[offset]);
        offset += 2;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    uint32_t StreamPeerBuffer::get_u32() {
        uint32_t num = from_bytes<uint32_t>(&data_array[offset]);
        offset += 4;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    uint64_t StreamPeerBuffer::get_u64() {
        uint64_t num = from_bytes<uint64_t>(&data_array[offset]);
        offset += 8;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    int StreamPeerBuffer::get_varuint(uint64_t& ret) {
        ret = 0;
        int shift = 0;
        while (true) {
            if (offset >= size()) {
                return 1;
            }

            uint8_t byte = get_u8();
            ret |= (byte & 0b01111111) << shift;
            if ((byte & 0x80) == 0)
                break;
            shift += 7;
        }
        return 0;
    }

    int8_t StreamPeerBuffer::get_i8() {
        int8_t num = from_bytes<int8_t>(&data_array[offset]);
        offset++;
        return num;
    }

    int16_t StreamPeerBuffer::get_i16() {
        int16_t num = from_bytes<int16_t>(&data_array[offset]);
        offset += 2;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    int32_t StreamPeerBuffer::get_i32() {
        int32_t num = from_bytes<int32_t>(&data_array[offset]);
        offset += 4;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    int64_t StreamPeerBuffer::get_i64() {
        int64_t num = from_bytes<int64_t>(&data_array[offset]);
        offset += 8;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    int StreamPeerBuffer::get_varint(int64_t& ret) {
        ret = 0;
        int shift = 0;
        int size = 64;

        uint8_t byte;
        do {
            if (offset >= this->size()) {
                return 1;
            }

            byte = get_u8();
            ret |= (byte & 0b01111111) << shift;
            shift += 7;
        } while (byte & 0x80);

        // sign bit of byte is second high-order bit
        if ((shift < size) && (byte & 0x40)) {
            ret |= (~0 << shift); // sign extend
        }

        return 0;
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
        str.assign(std::begin(bytes), std::end(bytes));
        return 0;
    }

    void StreamPeerBuffer::put_float(float num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[4];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 4;
    }

    float StreamPeerBuffer::get_float() {
        float num = from_bytes<float>(&data_array[offset]);
        offset += 4;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    void StreamPeerBuffer::put_double(double num) {
        if (swap_endian) {
            num = bswap(num);
        }
        char bytes[8];
        to_bytes(num, bytes);
        data_array.insert(data_array.begin() + offset, std::begin(bytes), std::end(bytes));
        offset += 8;
    }

    double StreamPeerBuffer::get_double() {
        double num = from_bytes<double>(&data_array[offset]);
        offset += 8;
        if (swap_endian) {
            num = bswap(num);
        }
        return num;
    }

    void StreamPeerBuffer::reset() {
        offset = 0;
        data_array.clear();
    }

    size_t StreamPeerBuffer::size() const {
        return data_array.size();
    }

    size_t StreamPeerBuffer::capacity() const {
        return data_array.capacity();
    }

    char* StreamPeerBuffer::data() {
        return data_array.data();
    }

    const char* StreamPeerBuffer::data() const {
        return data_array.data();
    }

    void StreamPeerBuffer::resize(size_t size) {
        data_array.resize(size);
    }

    void StreamPeerBuffer::reserve(size_t size) {
        data_array.reserve(size);
    }

    void StreamPeerBuffer::assign(size_t n, uint8_t val) {
        data_array.assign(n, val);
    }

    void StreamPeerBuffer::insert(iterator position, const_iterator first, const_iterator last) {
        data_array.insert(position, first, last);
    }

    void StreamPeerBuffer::insert(iterator position, size_t n, uint8_t val) {
        data_array.insert(position, n, val);
    }
} // namespace spb
