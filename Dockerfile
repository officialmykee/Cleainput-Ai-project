FROM python:3.11-slim

# Install system utilities for C++ compilation, Java for bundletool, and unzippers
RUN apt-get update && apt-get install -y \
    g++ \
    default-jre \
    zip \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Establish application run path
WORKDIR /app

# Install Python packages
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copy all remaining source codes and assets (including your .cpp files and assets/master_shell.aab)
COPY . .

# Expose the default networking port required by Hugging Face Spaces
EXPOSE 7860

# Command to launch the FastAPI server engine instantly on container start
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "7860"]

