#include <vector>
#include <array>
#include <memory>
#include <type_traits>
#include <iomanip>
#include <string>

#pragma once

namespace spb
{
    class StreamPeerBuffer
    {
        private:
          template <typename T>
          std::array<unsigned char, sizeof(T)> to_bytes(const T &object) {
            std::array<unsigned char, sizeof(T)> bytes;

            const unsigned char *begin =
                reinterpret_cast<const unsigned char *>(std::addressof(object));
            const unsigned char *end = begin + sizeof(T);
            std::copy(begin, end, std::begin(bytes));

            return bytes;
          }

          template <typename T>
          T &from_bytes(const std::array<unsigned char, sizeof(T)> &bytes,
                        T &object) {
            static_assert(std::is_trivially_copyable<T>::value,
                          "not a TriviallyCopyable type");

            unsigned char *begin_object =
                reinterpret_cast<unsigned char *>(std::addressof(object));
            std::copy(std::begin(bytes), std::end(bytes), begin_object);

            return object;
          }

            template <typename T>
            T bswap(T val)
            {
                T retVal;
                char *pVal = (char *) &val;
                char *pRetVal = (char *) &retVal;
                int size = sizeof(T);
                for (int i=0; i<size; i++) {
                    pRetVal[size-1-i] = pVal[i];
                }

                return retVal;
            }
        
        public:
          std::vector<unsigned char> data_array;
          unsigned int offset = 0;
          bool endian_swap = false;
          StreamPeerBuffer(bool);

          void put_u8(unsigned char);
          void put_u16(unsigned short int);
          void put_u32(unsigned int);

          void put_8(char);
          void put_16(short int);
          void put_32(int);

          unsigned char get_u8();
          unsigned short int get_u16();
          unsigned int get_u32();

          char get_8();
          short int get_16();
          int get_32();

          void put_utf8(std::string);
          std::string get_utf8();

          void put_float(float);
          float get_float();
    };
    StreamPeerBuffer::StreamPeerBuffer(bool is_opposite_endian)
    {
        endian_swap = is_opposite_endian;
    }
    void StreamPeerBuffer::put_u8(unsigned char number)
    {
        data_array.insert(data_array.begin() + offset, number);
        offset++;
    }
    void StreamPeerBuffer::put_u16(unsigned short int number)
    {
        if (endian_swap)
        {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    void StreamPeerBuffer::put_u32(unsigned int number)
    {
        if (endian_swap)
        {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    void StreamPeerBuffer::put_8(char number)
    {
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    void StreamPeerBuffer::put_16(short int number)
    {
        if (endian_swap)
        {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    void StreamPeerBuffer::put_32(int number)
    {
        if (endian_swap)
        {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    unsigned char StreamPeerBuffer::get_u8()
    {
        unsigned char number;
        from_bytes({data_array[offset]}, number);
        offset++;
        return number;
    }
    unsigned short int StreamPeerBuffer::get_u16()
    {
        unsigned short int number;
        from_bytes({data_array[offset], data_array[offset + 1]}, number);
        offset += 2;
        if (endian_swap)
        {
            number = bswap(number);
        }
        return number;
    }
    unsigned int StreamPeerBuffer::get_u32()
    {
        unsigned int number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (endian_swap)
        {
            number = bswap(number);
        }
        return number;
    }
    char StreamPeerBuffer::get_8()
    {
        char number;
        from_bytes({data_array[offset]}, number);
        offset++;
        return number;
    }
    short int StreamPeerBuffer::get_16()
    {
        short int number;
        from_bytes({data_array[offset], data_array[offset + 1]}, number);
        offset += 2;
        if (endian_swap)
        {
            number = bswap(number);
        }
        return number;
    }
    int StreamPeerBuffer::get_32()
    {
        int number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (endian_swap)
        {
            number = bswap(number);
        }
        return number;
    }
    void StreamPeerBuffer::put_utf8(std::string str) {
      std::vector<char> bytes(str.begin(), str.end());
      put_u16(str.length());
      for (char b : bytes) {
        put_8(b);
      }
    }
    std::string StreamPeerBuffer::get_utf8() {
      unsigned short int length = get_u16();
      std::vector<char> bytes;
      for (unsigned short int b = 0; b < length; b++) {
        bytes.push_back(get_8());
      }
      std::string str(bytes.begin(), bytes.end());
      return str;
    }
    void StreamPeerBuffer::put_float(float number)
    {
        if (endian_swap)
        {
            number = bswap(number);
        }
        const auto bytes = to_bytes(number);
        for (unsigned char b : bytes)
        {
            data_array.insert(data_array.begin() + offset, b);
            offset++;
        }
    }
    float StreamPeerBuffer::get_float()
    {
        float number;
        from_bytes({data_array[offset], data_array[offset + 1], data_array[offset + 2], data_array[offset + 3]}, number);
        offset += 4;
        if (endian_swap)
        {
            number = bswap(number);
        }
        return number;
    }
}
