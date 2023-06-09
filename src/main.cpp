#include "test.pb.h"
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <fstream>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <msgpack.hpp>
#include <filesystem>
#include <chrono>
#include <array>
#include <vector>
#include <tuple>
namespace fs = std::filesystem;
using clk = std::chrono::system_clock;
using sec = std::chrono::duration<double>;
using ms = std::chrono::milliseconds;
// First create an instance of an engine.
std::random_device rnd_device;
// Specify the engine and distribution.
std::mt19937 mersenne_engine{rnd_device()}; // Generates random integers
std::uniform_int_distribution<int32_t> dist{0, 254};

std::fstream random_dev;
std::unique_ptr<char[]> randomMemory(int32_t buffer_size)
{
    std::unique_ptr<char[]> buffer(new char[buffer_size]);
    random_dev.read(buffer.get(), buffer_size);
    return buffer;
}
std::vector<char> randomMemoryVec(int32_t buffer_size)
{
    std::vector<char> buffer;
    buffer.resize(buffer_size);
    random_dev.read(&buffer[0], buffer_size);
    return buffer;
}

void test_protobuf(std::ofstream &ofs, int32_t buffer_size)
{
    std::unique_ptr<google::protobuf::io::OstreamOutputStream> fileOutput = std::make_unique<google::protobuf::io::OstreamOutputStream>(&ofs);
    std::unique_ptr<google::protobuf::io::CodedOutputStream> codedOutput = std::make_unique<google::protobuf::io::CodedOutputStream>(fileOutput.get());
    for (int idx = 0; idx < 1024; idx++)
    {
        codedOutput->WriteLittleEndian32(idx);
        codedOutput->WriteString("string data");
        std::unique_ptr<char[]> buffer = randomMemory(buffer_size);
        codedOutput->WriteRaw(buffer.get(), buffer_size);
        std::cout << "Index " << idx << "\t\r" << std::flush;
    }
    codedOutput.reset();
    fileOutput.reset();
}

struct DoubleVecType
{
    int32_t counter;
    std::string channel_name;
    std::vector<char> channel_value;
    MSGPACK_DEFINE(counter, channel_name, channel_value);
};

void test_msgpack(std::ofstream &ofs, int32_t buffer_size)
{
    std::string cn = "channel_name";
    for (int idx = 0; idx < 1024; idx++)
    {
        std::unique_ptr<char[]> buffer = randomMemory(buffer_size);
        std::tuple<int, std::string_view, msgpack::type::raw_ref> data(idx, "example", msgpack::type::raw_ref(buffer.get(), buffer_size));
        msgpack::pack(ofs, data);
        std::cout << "Index " << idx << "\t\r" << std::flush;
    }
}

int main()
{
    // reate array syze
    int cur_size = 2;
    int32_t max_buffer_size = 1024 * 1024;
    std::vector<int> test_array_size;
    while (cur_size < max_buffer_size)
    {
        test_array_size.push_back(cur_size <<= 1);
    }
    std::cout << "min size=" << test_array_size.front() << std::endl;
    std::cout << "max size=" << test_array_size.back() << std::endl;
    std::ofstream report;
    report.open("report.csv");

    random_dev.open("/dev/urandom",
                    std::ios::in |         // output file stream
                        std::ios::binary); // set file cursor at the end
    for (int idx = 0; idx < test_array_size.size(); idx++)
    {
        int32_t buffer_size = test_array_size[idx];
        std::cout << "test with size:" << buffer_size << std::endl;
        std::ofstream ofs_msgpack;
        ofs_msgpack.open("random.msgpack",
                         std::ios::out |        // output file stream
                             std::ios::binary); // set file cursor at the end
        std::cout << "Write msgpack" << std::endl;
        const auto before_msgpack = clk::now();
        if (ofs_msgpack)
        {
            test_msgpack(ofs_msgpack, buffer_size);
            ofs_msgpack.close();
        }
        const ms duration_msgpack_ms = std::chrono::duration_cast<ms>(clk::now() - before_msgpack);

        std::ofstream ofs_protobuf; // output file stream
        ofs_protobuf.open("random.protobuf",
                          std::ios::out |        // output file stream
                              std::ios::binary); // set file cursor at the end
        std::cout << "Write protobuf" << std::endl;
        const auto before_protobuf = clk::now();
        if (ofs_protobuf)
        {
            test_protobuf(ofs_protobuf, buffer_size);
            ofs_protobuf.close();
        }
        const ms duration_protobuf_ms = std::chrono::duration_cast<ms>(clk::now() - before_protobuf);
        std::cout << "Generate " << buffer_size << " bytes each iteration" << std::endl;
        std::cout << "Msgpack time " << duration_msgpack_ms.count() << " ms" << std::endl;
        std::cout << "Msgpack file size:" << fs::file_size("random.msgpack") << std::endl;
        std::cout << "Protobuf time " << duration_protobuf_ms.count() << " ms" << std::endl;
        std::cout << "Protobuf file size:" << fs::file_size("random.protobuf") << std::endl;

        report << buffer_size << "," << duration_msgpack_ms.count() << "," << fs::file_size("random.msgpack") << "," << duration_protobuf_ms.count() << "," << fs::file_size("random.protobuf") << std::endl;
    }
    random_dev.close();
    report.close();
    return 0;
}