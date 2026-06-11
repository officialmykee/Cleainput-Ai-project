import base64
import io
import os
import subprocess
import json
import torch
from PIL import Image
from pydantic import BaseModel
from typing import Optional
from transformers import AutoProcessor, AutoModelForVision2Seq

# ==========================================
# 1. INITIALIZE MULTIMODAL AI VISION LAYER
# ==========================================
device = "cuda" if torch.cuda.is_available() else "cpu"
print(f"Initializing Multimodal Target Matrix on device: {device}")

model_id = "microsoft/Phi-3.5-vision-instruct"
processor = AutoProcessor.from_pretrained(model_id, trust_remote_code=True)
model = AutoModelForVision2Seq.from_pretrained(
    model_id, 
    device_map="auto", 
    torch_dtype=torch.float16 if device == "cuda" else torch.float32, 
    trust_remote_code=True
)

class PayloadData(BaseModel):
    prompt: Optional[str] = None
    image_data: Optional[str] = None

class RequestModel(BaseModel):
    payload: PayloadData

# ==========================================
# 2. MAIN EXECUTION ROUTER (PREDICT)
# ==========================================
def predict(request: RequestModel):
    try:
        user_prompt = request.payload.prompt or "Analyze this UI mockup and extract all design tokens, colors, layouts, and components."
        base64_image = request.payload.image_data
        
        # --- STAGE 1: RUN VISION LLM ON IMAGE ---
        ai_extracted_tokens = ""
        if base64_image:
            if "," in base64_image:
                base64_image = base64_image.split(",")[1]
            image_bytes = base64.b64decode(base64_image)
            image = Image.open(io.BytesIO(image_bytes)).convert("RGB")
            
            messages = [
                {
                    "role": "user",
                    "content": f"<|image_1|>\nExtract structural design tokens, component tags, spacing attributes, and hex colors for: {user_prompt}"
                }
            ]
            prompt_string = processor.tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
            inputs = processor(text=prompt_string, images=[image], return_tensors="pt").to(device)
            
            with torch.no_grad():
                generated_ids = model.generate(
                    **inputs, 
                    max_new_tokens=600, 
                    eos_token_id=processor.tokenizer.eos_token_id,
                    temperature=0.2
                )
            generated_ids = [output_ids[len(input_ids):] for input_ids, output_ids in zip(inputs.input_ids, generated_ids)]
            ai_extracted_tokens = processor.tokenizer.batch_decode(generated_ids, skip_special_tokens=True)[0]
        else:
            ai_extracted_tokens = user_prompt

        # --- STAGE 2: COMPILE & EXECUTE HIGH-PERFORMANCE C++ ROUTER ---
        print("Passing data to native C++ compilation engine...")
        
        # Compile C++ binaries on-the-fly inside the container if they don't exist
        if not os.path.exists("./parser_engine"):
            subprocess.run(["g++", "-O3", "-std=c++17", "parser.cpp", "-o", "parser_engine"], check=True)
            
        if not os.path.exists("./aab_injector"):
            subprocess.run(["g++", "-O3", "-std=c++17", "aab_injector.cpp", "-o", "aab_injector"], check=True)

        # Pipe the AI text results directly into your C++ parser_engine via stdin
        parser_proc = subprocess.Popen(
            ["./parser_engine"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        js_bundle_code, parser_err = parser_proc.communicate(input=json.dumps({"tokens": ai_extracted_tokens}))
        
        if parser_proc.returncode != 0:
            return {"status": "error", "phase": "AST_parsing_failure", "log": parser_err.strip()}

        # Run your binary modifier asset injection pass
        master_template = "assets/master_shell.aab"
        output_bundle = "output/generated_app.aab"
        os.makedirs("output", exist_ok=True)

        injector_proc = subprocess.run(
            ["./aab_injector", master_template, output_bundle, js_bundle_code.strip()],
            capture_output=True,
            text=True
        )

        if injector_proc.returncode != 0:
            return {"status": "error", "phase": "binary_AAB_injection_failure", "log": injector_proc.stderr.strip()}

        # Return everything back cleanly to your frontend chat window
        return {
            "status": "success",
            "message": ai_extracted_tokens,  # This streams the text explanation to the user chat bubble
            "react_native_source_preview": js_bundle_code[:200] + "..."
        }

    except Exception as e:
        print(f"Pipeline failure: {str(e)}")
        return {
            "status": "error",
            "message": f"Critical pipeline failure: {str(e)}"
        }

