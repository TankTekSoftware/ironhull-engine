#include <IronHull/io/FileSystem.hpp>
#include <IronHull/io/SaveFile.hpp>

namespace IronHull
{
    namespace
    {
        // Arbitrary fixed seed for the xorshift32 keystream below. The goal is to scramble the
        // save file's bytes, not to guard a secret, so there's no need to hide or vary this.
        constexpr unsigned int KEYSTREAM_SEED = 0x4C696E74u; // 'Lint'
    }

    std::vector<unsigned char> SaveFile::apply_keystream(std::vector<unsigned char> data)
    {
        // xorshift32: a cheap, dependency-free PRNG. Its output is far less patterned than a
        // repeating XOR key would be, which is enough to defeat a casual look at the raw bytes.
        unsigned int state = KEYSTREAM_SEED;
        for (unsigned char& byte : data) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            byte ^= static_cast<unsigned char>(state & 0xFF);
        }
        return data;
    }

    void SaveFile::write(const std::string& uri, const std::vector<unsigned char>& data)
    {
        std::vector<unsigned char> encrypted = SaveFile::apply_keystream(data);
        FileSystem::write_bytes(uri, encrypted.data(), encrypted.size());
    }

    std::vector<unsigned char> SaveFile::read(const std::string& uri)
    {
        std::vector<unsigned char> encrypted = FileSystem::read_bytes(uri);
        return SaveFile::apply_keystream(encrypted);
    }
}
