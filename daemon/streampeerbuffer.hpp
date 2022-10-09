#ifndef _STREAMPEERBUFFER_HPP
#define _STREAMPEERBUFFER_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace spb {
    template <typename T>
    void to_bytes(const T& object, char* ret) {
        memcpy(ret, &object, sizeof(object));
    }

    template <typename T>
    T from_bytes(char* bytes) {
        T ret;
        memcpy(&ret, bytes, sizeof(T));
        return ret;
    }

    template <typename T>
    T bswap(T object) {
        union {
            T object;
            char bytes[sizeof(T)];
        } ret;
        char* original_bytes = (char*) &object;
        for (size_t i = 0; i < sizeof(T); i++) {
            ret.bytes[sizeof(T) - 1 - i] = original_bytes[i];
        }
        return ret.object;
    }

    class StreamPeerBuffer {
    public:
        std::vector<char> data_array;
        size_t offset = 0;
        bool swap_endian;

        StreamPeerBuffer(bool big_endian = true);

        void put_u8(uint8_t);
        void put_u16(uint16_t);
        void put_u32(uint32_t);
        void put_u64(uint64_t);
        void put_varuint(uint64_t);

        void put_i8(int8_t);
        void put_i16(int16_t);
        void put_i32(int32_t);
        void put_i64(int64_t);
        void put_varint(int64_t);

        uint8_t get_u8();
        uint16_t get_u16();
        uint32_t get_u32();
        uint64_t get_u64();
        int get_varuint(uint64_t&);

        int8_t get_i8();
        int16_t get_i16();
        int32_t get_i32();
        int64_t get_i64();
        int get_varint(int64_t&);

        void put_string(const std::string&);
        int get_string(std::string& str);

        void put_float(float);
        float get_float();
        void put_double(double);
        double get_double();

        typedef decltype(data_array)::iterator iterator;
        typedef decltype(data_array)::const_iterator const_iterator;

        void reset();
        size_t size() const;
        size_t capacity() const;
        char* data();
        const char* data() const;
        void resize(size_t);
        void reserve(size_t);
        void assign(size_t n, uint8_t val);
        void insert(iterator, const_iterator, const_iterator);
        void insert(iterator, size_t, uint8_t);

        template <typename InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            data_array.assign(first, last);
        }

        inline iterator begin() {
            return data_array.begin();
        }
        inline const_iterator begin() const {
            return data_array.begin();
        }

        inline iterator end() {
            return data_array.end();
        }
        inline const_iterator end() const {
            return data_array.end();
        }
    };
} // namespace spb

#endif