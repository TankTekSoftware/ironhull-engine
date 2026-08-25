#pragma once

#include <string>
#include <vector>

namespace IronHull
{
    // Persists arbitrary byte buffers through FileSystem's user:// storage, XOR'd against a
    // keystream so the bytes on disk aren't plainly readable. This keeps a save file from being
    // opened in a text/hex editor and read or hand-edited at a glance - it is NOT cryptographically
    // secure (the key is fixed and lives in the shipped binary) and won't stop a determined
    // attacker with the source, only casual poking around.
    class SaveFile
    {
        public:
            static void write(const std::string& uri, const std::vector<unsigned char>& data);
            static std::vector<unsigned char> read(const std::string& uri);

        private:
            // XOR is its own inverse, so regenerating the same keystream from the same seed
            // both encrypts (write) and decrypts (read) - one function serves both directions.
            static std::vector<unsigned char> apply_keystream(std::vector<unsigned char> data);
    };
}
