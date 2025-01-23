import asyncio
import websockets
import json
import logging

logging.basicConfig(level=logging.INFO)

clients = set()

async def handler(websocket, path):
    clients.add(websocket)
    try:
        async for message in websocket:
            await broadcast_message(websocket, message)
    except websockets.ConnectionClosedError:
        pass  # Ignore graceful disconnections
    finally:
        clients.remove(websocket)


async def broadcast_message(sender_websocket, message):
    try:
        message_data = json.loads(message)
        if 'text' in message_data and isinstance(message_data['text'], str):
            text = message_data['text'].strip()
            if text:
                for client in clients:
                    if client != sender_websocket:
                        try:
                            await client.send(message)
                        except websockets.ConnectionClosedError:
                            pass
            else:
                logging.warning(f"Client sent an empty message.")
        else:
            logging.warning(f"Client sent invalid message: {message}")
    except json.JSONDecodeError:
        logging.error(f"Client sent invalid JSON: {message}")


async def main():
    async with websockets.serve(handler, "localhost", 8765):
        logging.info("Server started on ws://localhost:8765")
        await asyncio.Future()

if __name__ == "__main__":
    asyncio.run(main())