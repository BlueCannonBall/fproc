#include <vector>
#include <array>
#include <string>

#pragma once

namespace spb
{
    class StreamPeerBuffer
    {
        private:
            template <typename T>
            std::array<uint8_t, sizeof(T)> to_bytes(const T& object);

            template <typename T>
            void from_bytes(const std::array<uint8_t, sizeof(T)>& bytes,
                T& object);

            template <typename T>
            T bswap(T val);
        
        public:
            std::vector<uint8_t> data_array;
            size_t offset = 0;
            bool big_endian = false;
            
            StreamPeerBuffer(bool big_endian=true);

            void put_u8(uint8_t);
            void put_u16(uint16_t);
            void put_u32(uint32_t);
            void put_u64(uint64_t);

            void put_8(int8_t);
            void put_16(int16_t);
            void put_32(int32_t);
            void put_64(int64_t);

            uint8_t get_u8();
            uint16_t get_u16();
            uint32_t get_u32();
            uint64_t get_u64();

            int8_t get_8();
            int16_t get_16();
            int32_t get_32();
            int64_t get_64();

            void put_string(const std::string&);
            std::string get_string();

            void put_float(float);
            float get_float();
            void put_double(double);
            double get_double();

            void reset();
            size_t size();
    };
}
