from transformers import Wav2Vec2ForCTC, Wav2Vec2CTCTokenizer
import torchaudio
import torch

# Define the model and tokenizer
model_name = "wav2vec2-large-xlsr-53"  # Base model, needs fine-tuning for Hungarian
tokenizer = Wav2Vec2CTCTokenizer.from_pretrained(model_name)
model = Wav2Vec2ForCTC.from_pretrained(model_name)

# Load the audio file
waveform, sample_rate = torchaudio.load("text.wav")

# Resample audio to 16kHz if it's not already in that sample rate
if sample_rate != 16000:
    resampler = torchaudio.transforms.Resample(orig_freq=sample_rate, new_freq=16000)
    waveform = resampler(waveform)
    sample_rate = 16000

# Preprocess the waveform (convert to tensor, normalize, etc.)
input_values = tokenizer(waveform.squeeze(), return_tensors="pt", sampling_rate=sample_rate).input_values

# Perform inference (forward pass)
with torch.no_grad():
    logits = model(input_values).logits

# Decode the predicted IDs
predicted_ids = torch.argmax(logits, dim=-1)
transcription = tokenizer.batch_decode(predicted_ids)[0]

# Print the transcription
print("Transcription:", transcription)
