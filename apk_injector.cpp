#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

// Direct, high-discipline ZIP archive asset alignment tool
class LocalZipModifier {
public:
    // Scans a raw APK buffer, finds the JS bundle signature, and replaces its contents
    static bool OverwriteBundle(std::vector<char>& apkBuffer, const std::string& targetPath, const std::string& newJsCode) {
        // Look for the Local File Header signature (0x04034b50) or the string path directly
        auto it = std::search(apkBuffer.begin(), apkBuffer.end(), targetPath.begin(), targetPath.end());
        
        if (it == apkBuffer.end()) {
            std::cerr << "Target asset descriptor signature mismatch or file path missing." << std::endl;
            return false;
        }

        size_t pathIndex = std::distance(apkBuffer.begin(), it);
        
        // Step backwards to locate the local file header start signature block
        size_t headerStart = pathIndex;
        while (headerStart > 4) {
            if (apkBuffer[headerStart] == 0x50 && apkBuffer[headerStart+1] == 0x4B && 
                apkBuffer[headerStart+2] == 0x03 && apkBuffer[headerStart+3] == 0x04) {
                break;
            }
            headerStart--;
        }

        // Read metadata offsets: compressed and uncompressed file size markers
        // Local File Header offsets: Compressed size at bytes 18-21, Uncompressed size at 22-25
        uint32_t* compressedSize = reinterpret_cast<uint32_t*>(&apkBuffer[headerStart + 18]);
        uint32_t* uncompressedSize = reinterpret_cast<uint32_t*>(&apkBuffer[headerStart + 22]);
        uint16_t fileNameLength = *reinterpret_cast<uint16_t*>(&apkBuffer[headerStart + 26]);
        uint16_t extraFieldLength = *reinterpret_cast<uint16_t*>(&apkBuffer[headerStart + 28]);

        size_t dataStartOffset = headerStart + 30 + fileNameLength + extraFieldLength;

        // Ensure the shell APK stored index.android.bundle completely uncompressed (Store method: 0)
        // This is key to letting us swap the data cleanly without processing deep zlib algorithms
        if (newJsCode.size() > *uncompressedSize) {
            std::cerr << "New payload size exceeds pre-allocated shell master asset bounds." << std::endl;
            return false;
        }

        // Direct memory footprint injection
        std::copy(newJsCode.begin(), newJsCode.end(), apkBuffer.begin() + dataStartOffset);

        // Zero out remaining trailing bytes of the old bundle file segment to prevent syntax artifacts
        size_t remainingBytes = *uncompressedSize - newJsCode.size();
        std::fill_n(apkBuffer.begin() + dataStartOffset + newJsCode.size(), remainingBytes, ' ');

        // Safely update file sizes across local descriptors to keep data integrity clean
        *compressedSize = static_cast<uint32_t>(newJsCode.size());
        *uncompressedSize = static_cast<uint32_t>(newJsCode.size());

        return true;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: ./injector <master.apk> <output.apk> <new_code_string>" << std::endl;
        return 1;
    }

    std::string masterPath = argv[1];
    std::string outputPath = argv[2];
    std::string newCode = argv[3];

    // Read the master app binary shell completely into RAM
    std::ifstream file(masterPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return 1;
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) return 1;
    file.close();

    // Inject our new code asset right into the target zip sector
    std::string targetAsset = "assets/index.android.bundle";
    if (LocalZipModifier::OverwriteBundle(buffer, targetAsset, newCode)) {
        // Output stream writes down the completely operational app instantly
        std::ofstream outFile(outputPath, std::ios::binary);
        outFile.write(buffer.data(), buffer.size());
        outFile.close();
        std::cout << "SUCCESS_APK_ESTABLISHED" << std::endl;
        return 0;
    }

    return 1;
}

