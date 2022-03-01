#ifndef _STREAMPEERBUFFER_HPP
#define _STREAMPEERBUFFER_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace spb {
    template <typename T>
    std::array<uint8_t, sizeof(T)> to_bytes(const T& object) {
        std::array<uint8_t, sizeof(T)> bytes;
        union {
            T object;
            uint8_t bytes[sizeof(T)];
        } converter;
        converter.object = object;
        std::copy(std::begin(converter.bytes), std::end(converter.bytes), std::begin(bytes));
        return bytes;
    }

    template <typename T>
    void from_bytes(const std::array<uint8_t, sizeof(T)>& bytes,
        T& object) {
        union {
            T object;
            uint8_t bytes[sizeof(T)];
        } converter;
        memcpy(converter.bytes, bytes.data(), sizeof(T));
        object = converter.object;
    }

    template <typename T>
    T bswap(T val) {
        T ret;
        auto pointer_val = (int8_t*) &val;
        auto pointer_ret = (int8_t*) &ret;
        size_t size = sizeof(T);

        for (size_t i = 0; i < size; i++) {
            pointer_ret[size - 1 - i] = pointer_val[i];
        }

        return ret;
    }

    class StreamPeerBuffer {
    public:
        std::vector<uint8_t> data_array;
        size_t offset = 0;
        bool big_endian = false;

        StreamPeerBuffer(bool big_endian = true);

        void put_u8(uint8_t);
        void put_u16(uint16_t);
        void put_u32(uint32_t);
        void put_u64(uint64_t);

        void put_i8(int8_t);
        void put_i16(int16_t);
        void put_i32(int32_t);
        void put_i64(int64_t);

        uint8_t get_u8();
        uint16_t get_u16();
        uint32_t get_u32();
        uint64_t get_u64();

        int8_t get_i8();
        int16_t get_i16();
        int32_t get_i32();
        int64_t get_i64();

        void put_string(const std::string&);
        int get_string(std::string& str);

        void put_float(float);
        float get_float();
        void put_double(double);
        double get_double();

        void reset();
        size_t size();
        uint8_t* data();
        void resize(size_t);
    };
} // namespace spb

#endif