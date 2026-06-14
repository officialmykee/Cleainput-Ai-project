#include <iostream>
#include <string>
#include <vector>
#include <sstream>

struct UIComponent {
    std::string type;    
    std::string content; 
    std::string style;   
};

class ClnInputParser {
private:
    static std::string ExtractValue(const std::string& json, const std::string& key) {
        size_t pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        
        size_t start = json.find(":", pos);
        if (start == std::string::npos) return "";
        
        size_t quoteStart = json.find("\"", start);
        size_t quoteEnd = json.find("\"", quoteStart + 1);
        
        if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
            return json.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
        }
        return "";
    }

public:
    static std::string ParseToReactNative(const std::string& jsonLayout) {
        UIComponent component;
        component.type = ExtractValue(jsonLayout, "type");
        component.content = ExtractValue(jsonLayout, "content");
        component.style = ExtractValue(jsonLayout, "style");

        if (component.type.empty()) component.type = "View";

        std::stringstream src;
        src << "import React from 'react';\n";
        src << "import { View, Text, TouchableOpacity, StyleSheet } from 'react-native';\n\n";
        src << "export default function App() {\n";
        src << "  return (\n";
        src << "    <" << component.type << " style={styles.element}>\n";
        
        if (!component.content.empty()) {
            src << "      <Text>" << component.content << "</Text>\n";
        }
        
        src << "    </" << component.type << ">\n";
        src << "  );\n";
        src << "}\n\n";
        
        src << "const styles = StyleSheet.create({\n";
        src << "  element: { " << (component.style.empty() ? "flex: 1, justifyContent: 'center', alignItems: 'center'" : component.style) << " }\n";
        src << "});\n";

        return src.str();
    }
};

int main() {
    // FIX: Read incoming streamed layout definitions from the Python main.py pipeline
    std::string incomingJson;
    std::string line;
    while (std::getline(std::cin, line)) {
        incomingJson += line;
    }
    
    // If Python sends empty data, use your mock layout as a fallback safety net
    if (incomingJson.empty()) {
        incomingJson = "{\"type\":\"TouchableOpacity\",\"content\":\"Click Me\",\"style\":\"backgroundColor:'#007AFF',padding:15,borderRadius:8\"}";
    }
    
    std::string outSource = ClnInputParser::ParseToReactNative(incomingJson);
    std::cout << outSource << std::endl;
    
    return 0;
}
