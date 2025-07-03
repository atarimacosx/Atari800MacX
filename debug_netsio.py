#!/usr/bin/env python3
"""
NetSIO Debug Tool - Listen on UDP port and show packet contents
This helps debug communication between Atari800MacX and FujiNet-PC
"""

import socket
import sys
import time

# NetSIO packet types for reference
NETSIO_PACKETS = {
    0x01: "DATA_BYTE",
    0x02: "DATA_BLOCK", 
    0x09: "DATA_BYTE_SYNC",
    0x10: "COMMAND_OFF",
    0x11: "COMMAND_ON",
    0x18: "COMMAND_OFF_SYNC",
    0x20: "MOTOR_OFF",
    0x21: "MOTOR_ON",
    0x30: "PROCEED_OFF",
    0x31: "PROCEED_ON",
    0x40: "INTERRUPT_OFF",
    0x41: "INTERRUPT_ON",
    0x80: "SPEED_CHANGE",
    0x81: "SYNC_RESPONSE",
    0xC0: "DEVICE_DISCONNECTED",
    0xC1: "DEVICE_CONNECTED",
    0xC2: "PING_REQUEST",
    0xC3: "PING_RESPONSE",
    0xC4: "ALIVE_REQUEST",
    0xC5: "ALIVE_RESPONSE",
    0xC6: "CREDIT_STATUS",
    0xC7: "CREDIT_UPDATE",
    0xFE: "WARM_RESET",
    0xFF: "COLD_RESET"
}

def listen_and_respond(port=9997):
    """Listen on UDP port and respond to emulator packets"""
    
    # Create UDP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        sock.bind(('localhost', port))
        print(f"NetSIO Debug Tool listening on localhost:{port}")
        print("Waiting for packets from Atari800MacX emulator...")
        print("-" * 50)
        
        while True:
            # Receive packet
            data, addr = sock.recvfrom(4096)
            timestamp = time.strftime("%H:%M:%S.%f")[:-3]
            
            if len(data) > 0:
                packet_type = data[0]
                packet_name = NETSIO_PACKETS.get(packet_type, f"UNKNOWN_{packet_type:02X}")
                
                print(f"[{timestamp}] From {addr}")
                print(f"  Packet: {packet_name} (0x{packet_type:02X})")
                print(f"  Length: {len(data)} bytes")
                print(f"  Data: {' '.join(f'{b:02X}' for b in data)}")
                
                # Respond to ping requests
                if packet_type == 0xC2:  # PING_REQUEST
                    response = bytes([0xC3])  # PING_RESPONSE
                    sock.sendto(response, addr)
                    print(f"  -> Sent PING_RESPONSE")
                    
                    # Send DEVICE_CONNECTED after ping
                    time.sleep(0.1)
                    connect_packet = bytes([0xC1])  # DEVICE_CONNECTED
                    sock.sendto(connect_packet, addr)
                    print(f"  -> Sent DEVICE_CONNECTED")
                
                # Respond to alive requests
                elif packet_type == 0xC4:  # ALIVE_REQUEST
                    response = bytes([0xC5])  # ALIVE_RESPONSE
                    sock.sendto(response, addr)
                    print(f"  -> Sent ALIVE_RESPONSE")
                
                print()
            
    except KeyboardInterrupt:
        print("\nStopping NetSIO debug tool...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    port = 9997
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    
    listen_and_respond(port)