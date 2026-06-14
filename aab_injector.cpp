#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstring>

class LocalAabModifier {
public:
    static bool OverwriteBundle(std::vector<char>& aabBuffer, const std::string& targetPath, const std::string& newJsCode) {
        auto it = std::search(aabBuffer.begin(), aabBuffer.end(), targetPath.begin(), targetPath.end());
        
        if (it == aabBuffer.end()) {
            std::cerr << "Target asset descriptor signature mismatch or file path missing inside AAB." << std::endl;
            return false;
        }

        size_t pathIndex = std::distance(aabBuffer.begin(), it);
        
        size_t headerStart = pathIndex;
        bool foundHeader = false;
        while (headerStart > 4) {
            if (static_cast<unsigned char>(aabBuffer[headerStart]) == 0x50 && 
                static_cast<unsigned char>(aabBuffer[headerStart+1]) == 0x4B && 
                static_cast<unsigned char>(aabBuffer[headerStart+2]) == 0x03 && 
                static_cast<unsigned char>(aabBuffer[headerStart+3]) == 0x04) {
                foundHeader = true;
                break;
            }
            headerStart--;
        }

        if (!foundHeader) {
            std::cerr << "Failed to locate local file header signature block boundary constraints." << std::endl;
            return false;
        }

        // FIX: Read standard little-endian ZIP metadata safely using safe copies to prevent alignment crashes
        uint32_t uncompressedSize = 0;
        std::memcpy(&uncompressedSize, &aabBuffer[headerStart + 22], 4);

        uint16_t fileNameLength = 0;
        std::memcpy(&fileNameLength, &aabBuffer[headerStart + 26], 2);

        uint16_t extraFieldLength = 0;
        std::memcpy(&extraFieldLength, &aabBuffer[headerStart + 28], 2);

        size_t dataStartOffset = headerStart + 30 + fileNameLength + extraFieldLength;

        if (newJsCode.size() > uncompressedSize) {
            std::cerr << "New payload size (" << newJsCode.size() 
                      << " bytes) exceeds master template allocation layout bounds (" 
                      << uncompressedSize << " bytes)." << std::endl;
            return false;
        }

        std::copy(newJsCode.begin(), newJsCode.end(), aabBuffer.begin() + dataStartOffset);

        size_t remainingBytes = uncompressedSize - newJsCode.size();
        std::fill_n(aabBuffer.begin() + dataStartOffset + newJsCode.size(), remainingBytes, ' ');

        // FIX: Safely update compressed size parameters back into the byte vector layout
        uint32_t compressedSizeUpdate = uncompressedSize;
        std::memcpy(&aabBuffer[headerStart + 18], &compressedSizeUpdate, 4);

        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./aab_injector <master.aab> <output.aab> <new_code_string>" << std::endl;
        return 1;
    }

    std::string masterPath = argv[1];
    std::string outputPath = argv[2];
    std::string newCode = argv[3];

    std::ifstream file(masterPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to load master template layout asset bundle file." << std::endl;
        return 1;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        std::cerr << "Error reading binary stream array footprint content structures." << std::endl;
        return 1;
    }
    file.close();

    std::string targetAsset = "base/assets/index.android.bundle";
    
    if (LocalAabModifier::OverwriteBundle(buffer, targetAsset, newCode)) {
        std::ofstream outFile(outputPath, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Error outputting target binary asset." << std::endl;
            return 1;
        }
        outFile.write(buffer.data(), buffer.size());
        outFile.close();
        std::cout << "SUCCESS_AAB_ESTABLISHED" << std::endl;
        return 0;
    }

    return 1;
}


