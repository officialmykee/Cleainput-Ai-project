import subprocess
import json

# Zero-bloat HTTP wrapper for Cerebrium Serverless Runtime
def predict(run_arguments, status_logger):
    try:
        # Extract incoming raw payload from Cloudflare Pages editor
        payload = run_arguments.get("payload", "")
        if not payload:
            return {"status": "error", "message": "Empty layout tree structure"}

        # Compile the C++ parser on-the-fly via native subprocess if binary doesn't exist
        # This completely replaces the need for a brittle Makefile structure
        compile_cmd = ["g++", "-O3", "-std=c++17", "parser.cpp", "-o", "engine"]
        subprocess.run(compile_cmd, check=True, capture_output=True)

        # Direct execution pass: stream payload straight into our C++ machine code engine
        # Using a direct pipeline execution pattern—completely avoiding bash files
        process = subprocess.Popen(
            ["./engine"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # In a production layout, your C++ engine accepts input strings via standard input
        stdout, stderr = process.communicate(input=json.dumps(payload))

        if process.returncode != 0:
            return {"status": "error", "error_log": stderr.strip()}

        # Return the production ready React Native code straight to the front-end stream
        return {
            "status": "success",
            "react_native_source": stdout.strip()
        }

    except Exception as e:
        return {"status": "critical_failure", "message": str(e)}

