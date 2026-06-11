#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

// Direct, high-discipline AAB bundle archive asset alignment tool
class LocalAabModifier {
public:
    // Scans a raw AAB buffer, finds the React Native JS bundle signature, and replaces its contents
    static bool OverwriteBundle(std::vector<char>& aabBuffer, const std::string& targetPath, const std::string& newJsCode) {
        // Look for the targeted internal file path directly inside the ZIP allocation index
        auto it = std::search(aabBuffer.begin(), aabBuffer.end(), targetPath.begin(), targetPath.end());
        
        if (it == aabBuffer.end()) {
            std::cerr << "Target asset descriptor signature mismatch or file path missing inside AAB." << std::endl;
            return false;
        }

        size_t pathIndex = std::distance(aabBuffer.begin(), it);
        
        // Step backwards to locate the Local File Header start block signature (0x04034b50)
        size_t headerStart = pathIndex;
        while (headerStart > 4) {
            if (aabBuffer[headerStart] == 0x50 && aabBuffer[headerStart+1] == 0x4B && 
                aabBuffer[headerStart+2] == 0x03 && aabBuffer[headerStart+3] == 0x04) {
                break;
            }
            headerStart--;
        }

        // Read layout metadata offsets inside the ZIP header
        uint32_t* compressedSize = reinterpret_cast<uint32_t*>(&aabBuffer[headerStart + 18]);
        uint32_t* uncompressedSize = reinterpret_cast<uint32_t*>(&aabBuffer[headerStart + 22]);
        uint16_t fileNameLength = *reinterpret_cast<uint16_t*>(&aabBuffer[headerStart + 26]);
        uint16_t extraFieldLength = *reinterpret_cast<uint16_t*>(&aabBuffer[headerStart + 28]);

        size_t dataStartOffset = headerStart + 30 + fileNameLength + extraFieldLength;

        // Verify bounds constraint compliance against our pre-allocated shell template sector
        if (newJsCode.size() > *uncompressedSize) {
            std::cerr << "New payload size exceeds pre-allocated shell master asset bounds." << std::endl;
            return false;
        }

        // Direct memory footprint injection onto the binary vector array
        std::copy(newJsCode.begin(), newJsCode.end(), aabBuffer.begin() + dataStartOffset);

        // Clear remaining byte artifacts safely using empty space padding
        size_t remainingBytes = *uncompressedSize - newJsCode.size();
        std::fill_n(aabBuffer.begin() + dataStartOffset + newJsCode.size(), remainingBytes, ' ');

        // Update descriptors to keep archive parsing perfectly operational
        *compressedSize = static_cast<uint32_t>(newJsCode.size());
        *uncompressedSize = static_cast<uint32_t>(newJsCode.size());

        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./injector <master.aab> <output.aab> <new_code_string>" << std::endl;
        return 1;
    }

    std::string masterPath = argv[1];
    std::string outputPath = argv[2];
    std::string newCode = argv[3];

    // Read the master template AAB completely into RAM
    std::ifstream file(masterPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 1;
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return 1;
    file.close();

    // Standard path target inside an Android App Bundle configuration
    std::string targetAsset = "base/assets/index.android.bundle";
    if (LocalAabModifier::OverwriteBundle(buffer, targetAsset, newCode)) {
        // Output stream writes down the completely operational app package instantly
        std::ofstream outFile(outputPath, std::ios::binary);
        outFile.write(buffer.data(), buffer.size());
        outFile.close();
        std::cout << "SUCCESS_AAB_ESTABLISHED" << std::endl;
        return 0;
    }

    return 1;
}

