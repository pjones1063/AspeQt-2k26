from google import genai

# PASTE YOUR KEY HERE
API_KEY = "xxxxxxxxxxxxxxxxxxxxxxx"

client = genai.Client(api_key=API_KEY)

print("--- MENU OF AVAILABLE MODELS ---")
try:
    # List all models available to your key
    for m in client.models.list():
        # Only show models that support generating content (chat)
        if "generateContent" in m.supported_actions:
            print(f"Name: {m.name}")
            
except Exception as e:
    print(f"Error: {e}")
