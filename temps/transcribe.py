import whisper
import sys
import warnings

# Suppress the FP16 warning
warnings.filterwarnings("ignore", message="FP16 is not supported on CPU; using FP32 instead")

# model = whisper.load_model("small")
# model = whisper.load_model("base")
model = whisper.load_model("tiny")
result = model.transcribe(sys.argv[1], language="hu")
print(result["text"])