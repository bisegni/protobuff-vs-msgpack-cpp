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
namespace fs = std::filesystem;
using clk = std::chrono::system_clock;
using sec = std::chrono::duration<double>;
// First create an instance of an engine.
std::random_device rnd_device;
// Specify the engine and distribution.
std::mt19937 mersenne_engine{rnd_device()}; // Generates random integers
std::uniform_int_distribution<int32_t> dist{0, 254};

std::unique_ptr<char[]> randomMemory(int32_t buffer_size)
{
    std::fstream random_dev;
    random_dev.open("/dev/urandom",
                    std::ios::in |         // output file stream
                        std::ios::binary); // set file cursor at the end
    std::unique_ptr<char[]> buffer(new char[buffer_size]);

    random_dev.read(buffer.get(), buffer_size);
    random_dev.close();
    return buffer;
}

void test_protobuf(std::ofstream &ofs, int32_t buffer_size)
{
    std::unique_ptr<google::protobuf::io::OstreamOutputStream> fileOutput = std::make_unique<google::protobuf::io::OstreamOutputStream>(&ofs);
    std::unique_ptr<google::protobuf::io::CodedOutputStream> codedOutput = std::make_unique<google::protobuf::io::CodedOutputStream>(fileOutput.get());
    for (int idx = 0; idx < 1024; idx++)
    {
        codedOutput->WriteString("counter");
        codedOutput->WriteLittleEndian32(idx);
        codedOutput->WriteString("channel_name");
        codedOutput->WriteString("string data");
        std::unique_ptr<char[]> buffer = randomMemory(buffer_size);
        codedOutput->WriteRaw(buffer.get(), buffer_size);

        std::cout << "Index " << idx << "\t\r" << std::flush;
    }
    codedOutput.reset();
    fileOutput.reset();
}

void test_msgpack(std::ofstream &ofs, int32_t buffer_size)
{
    for (int idx = 0; idx < 1024; idx++)
    {
        msgpack::pack(ofs, "counter");
        msgpack::pack(ofs, idx);
        msgpack::pack(ofs, "channel_name");
        msgpack::pack(ofs, "string data");

        std::unique_ptr<char[]> buffer = randomMemory(buffer_size);
        msgpack::pack(ofs, msgpack::type::raw_ref(buffer.get(), buffer_size));

        std::cout << "Index " << idx << "\t\r" << std::flush;
    }
}

int main()
{

    int32_t buffer_size = 1024 * 1024;
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
    const sec duration_msgpack = clk::now() - before_msgpack;

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
    const sec duration_protobuf = clk::now() - before_protobuf;
    std::cout << "Generate " << buffer_size << " bytes each iteration" << std::endl;
    std::cout << "Msgpack time " << duration_msgpack.count() << "s" << std::endl;
    std::cout << "Msgpack file size:" << fs::file_size("random.msgpack") << std::endl;
    std::cout << "Protobuf time " << duration_protobuf.count() << "s" << std::endl;
    std::cout << "Protobuf file size:" << fs::file_size("random.protobuf") << std::endl;
    return 0;
}