#include <iostream>
#include <string>
#include <sstream>

// Natively condenses repeating character sequences or text footprints 
std::string compressProfileData(const std::string& input) {
    if (input.empty()) return "";
    
    std::stringstream compressed;
    int count = 1;
    
    for (size_t i = 0; i < input.length(); ++i) {
        if (i + 1 < input.length() && input[i] == input[i + 1]) {
            count++;
        } else {
            compressed << input[i];
            if (count > 1) {
                compressed << count; // Appends token repetition metrics
            }
            count = 1;
        }
    }
    return compressed.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./compressor <string_to_compress>" << std::endl;
        return 1;
    }
    
    std::string rawInput = argv[1];
    std::string result = compressProfileData(rawInput);
    
    // Output string directly back to Cerebrium pipeline stdout router
    std::cout << result;
    return 0;
}

