import asyncio
import websockets
import json
import getpass

async def hello():
    uri = "ws://localhost:8765"
    username = getpass.getuser()

    try:
        async with websockets.connect(uri) as websocket:
            while True:
                message = input(f"{username}: ")
                if message.lower() == 'quit':
                    break
                await websocket.send(json.dumps({'text': message}))
                #Simplified: No error handling or response processing
    except websockets.ConnectionClosedError:
        print("Connection closed.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    asyncio.run(hello())