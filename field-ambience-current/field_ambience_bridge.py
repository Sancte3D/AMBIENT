#!/usr/bin/env python3
"""
Field Ambience Bridge v29-p (= v29-h + sync mit SC v29-o/p)
============================

Bridges three components:
  Browser (HTML panel)  ←WebSocket→  Bridge  ←OSC→  SuperCollider
  Pico (USB-Serial)     ←JSON-line→  Bridge  ←OSC→  SuperCollider

Run with:
  python3 field_ambience_bridge.py

Dependencies:
  pip install python-osc websockets pyserial

Ports:
  WebSocket server: 8765  (browser connects here)
  OSC outgoing:     57120 (to SuperCollider)
  OSC incoming:     57121 (SC state replies)

Pico hardware support is optional. If pyserial is missing or no Pico is
connected, the bridge runs in browser-only mode and reconnects to a Pico
automatically when one appears.
"""

import asyncio
import json
import sys
import threading
import time
from pythonosc import udp_client
from pythonosc import dispatcher
from pythonosc import osc_server
import websockets

try:
    import serial
    import serial.tools.list_ports
    HAVE_SERIAL = True
except ImportError:
    HAVE_SERIAL = False

# ----- Configuration -----
WS_HOST = "0.0.0.0"
WS_PORT = 8765
SC_HOST = "127.0.0.1"
SC_PORT = 57120
REPLY_PORT = 57121

# v29-h: Pico hardware bridge config
PICO_VID = 0x2E8A
PICO_BAUD = 115200
PICO_RECONNECT_S = 3.0

osc_client = udp_client.SimpleUDPClient(SC_HOST, SC_PORT)
ws_clients = set()
_main_loop = None
_pico_bridge = None


# ============================================================
# WebSocket handler (browser ↔ bridge)
# ============================================================
async def ws_handler(websocket):
    ws_clients.add(websocket)
    try:
        async for message in websocket:
            try:
                obj = json.loads(message)
                if obj.get("type") == "osc":
                    addr = obj.get("addr")
                    args = obj.get("args", [])
                    if addr:
                        osc_client.send_message(addr, args)
            except Exception as e:
                print(f"[ws] parse error: {e}")
    except websockets.ConnectionClosed:
        pass
    finally:
        ws_clients.discard(websocket)


# ============================================================
# OSC state listener (SC → bridge → all clients)
# ============================================================
def on_fam_state(addr, *args):
    try:
        state = {
            "type":         "state",
            "running":      int(args[0])   if len(args) > 0  else 0,
            "auto":         int(args[1])   if len(args) > 1  else 0,
            "key":          int(args[2])   if len(args) > 2  else 60,
            "mode":         int(args[3])   if len(args) > 3  else 0,
            "prog":         int(args[4])   if len(args) > 4  else -1,
            "degree":       int(args[5])   if len(args) > 5  else 1,
            "bpm":          float(args[6]) if len(args) > 6  else 72.0,
            "mood":         float(args[7]) if len(args) > 7  else 0.5,
            "hold":         int(args[8])   if len(args) > 8  else 0,
            "vibe":         int(args[9])   if len(args) > 9  else 0,
            "voice":        int(args[10])  if len(args) > 10 else 0,
            "midi":         int(args[11])  if len(args) > 11 else 0,
            "chord":        [int(args[i]) if len(args) > i else 0
                             for i in (12, 13, 14, 15, 16)],
            "vol":          float(args[17]) if len(args) > 17 else 0.85,
            "ampEnabled":   bool(int(args[18])) if len(args) > 18 else True,
            "jackInserted": bool(int(args[19])) if len(args) > 19 else False,
            # v29-j additions
            "padVoice":     int(args[20]) if len(args) > 20 else 0,
            "padOctave":    int(args[21]) if len(args) > 21 else 1,
            # v29-n additions
            "drive":        float(args[22]) if len(args) > 22 else 0.0,
            "brightness":   float(args[23]) if len(args) > 23 else 0.0,
            # v29-o additions
            "texture":      float(args[24]) if len(args) > 24 else 0.0,
            "space":        float(args[25]) if len(args) > 25 else 0.5,
            "depth":        float(args[26]) if len(args) > 26 else 0.5,
            "bloom":        float(args[27]) if len(args) > 27 else 0.5,
            "generative":   int(args[28]) if len(args) > 28 else 0,
            "drone":        int(args[29]) if len(args) > 29 else 0,
        }
        msg = json.dumps(state)
        if _main_loop is not None:
            for ws in list(ws_clients):
                asyncio.run_coroutine_threadsafe(ws.send(msg), _main_loop)
        if _pico_bridge is not None:
            try:
                _pico_bridge.on_sc_state(state)
            except Exception as e:
                print(f"[pico] state mirror error: {e}")
    except Exception as e:
        print(f"[osc] state parse error: {e}")


def start_osc_server():
    disp = dispatcher.Dispatcher()
    disp.map("/fam/state", on_fam_state)
    server = osc_server.ThreadingOSCUDPServer((SC_HOST, REPLY_PORT), disp)
    print(f"[osc] listening for SC replies on port {REPLY_PORT}")
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server


# ============================================================
# v29-h: Pico Hardware Bridge
# ============================================================
class PicoBridge:
    """Manages serial connection to Pico, parses events, sends OSC.
    Maintains an internal mirror of menu state for OLED rendering.
    """

    MENU_ITEMS = ["KEY", "MODE", "VOICE", "PAD", "OCT", "PROG", "TEMPO", "VIBE", "VOL", "MIDI"]

    # v29-p: Modifier-IDs aligned mit SPEC v0.6 §7 / Sheet 4 BOM:
    # SW6=SHIFT, SW7=HOLD, SW8=DRONE, SW9=GENERATE, SW10=CLEAR
    MOD_SHIFT    = 1
    MOD_HOLD     = 2
    MOD_DRONE    = 3
    MOD_GENERATE = 4
    MOD_CLEAR    = 5
    # Legacy aliases for old Pico-firmware that still uses these names
    MOD_AUTO     = MOD_GENERATE   # AUTO-Button → triggers generative mode
    MOD_FREEZE   = MOD_DRONE      # FREEZE-Button → toggles drone

    KEYS_NAMES   = ["C", "D", "Eb", "F", "G", "A", "Bb"]
    KEY_MIDI     = [60, 62, 63, 65, 67, 69, 70]
    # v29-p: Synced mit SC v29-o — 6 modes (Locrian removed), 5 progs
    # (Slow added as default index 0), 4 vibes (Sharp removed)
    MODE_NAMES   = ["Ion", "Dor", "Phr", "Lyd", "Mix", "Aeo"]
    PROG_NAMES   = ["Slow", "I-V-vi-IV", "I-IV-V-I", "i-VI-III-VII", "i-iv-VI-V"]
    VIBE_NAMES   = ["Warm", "Bright", "Deep", "Float"]
    VOICE_NAMES  = ["FM", "Saw", "Square"]
    PAD_VOICE_NAMES   = ["Warm", "Strings", "Brass"]
    PAD_OCTAVE_NAMES  = ["LOW", "MID", "HIGH"]
    NOTE_NAMES   = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"]

    def __init__(self, osc_client_obj):
        self.osc = osc_client_obj
        self.serial = None
        self.running = True
        self.shifted = False
        self.hold_mode = False
        # v29-p: explicit init prevents AttributeError on first toggle / state read
        self.generative = False
        self.drone = False
        # v29-p: Drive/Brightness macro state for the 4-encoder routing.
        # Defaults match the Sound Constitution (drive 0.18, brightness -0.2).
        self.drive = 0.18
        self.brightness = -0.2
        self.cursor_idx = 0
        self.mode_ui = "nav"
        self.sc_state = {
            "key": 60, "mode": 0, "voice": 0, "prog": -1,
            "bpm": 72, "vibe": 0, "vol": 0.85, "midi": 0,
            "chord": [60, 64, 67, 71, 74],
            "padVoice": 0, "padOctave": 1,
        }

    def find_port(self):
        if not HAVE_SERIAL:
            return None
        for port in serial.tools.list_ports.comports():
            if port.vid == PICO_VID:
                return port.device
        return None

    def connect(self):
        port = self.find_port()
        if port is None:
            return False
        try:
            self.serial = serial.Serial(port, PICO_BAUD, timeout=0.1)
            print(f"[pico] connected: {port}")
            return True
        except Exception as e:
            print(f"[pico] connect failed on {port}: {e}")
            self.serial = None
            return False

    def run(self):
        if not HAVE_SERIAL:
            print("[pico] pyserial not installed; hardware bridge disabled.")
            return
        while self.running:
            if self.serial is None:
                if self.connect():
                    self.update_display()
                else:
                    time.sleep(PICO_RECONNECT_S)
                    continue
            try:
                line = self.serial.readline()
                if line:
                    text = line.decode("utf-8", errors="replace").strip()
                    if text:
                        self.handle_event(text)
            except (serial.SerialException, OSError) as e:
                print(f"[pico] disconnected: {e}")
                try:
                    self.serial.close()
                except Exception:
                    pass
                self.serial = None
                time.sleep(PICO_RECONNECT_S)

    def send_to_pico(self, obj):
        if self.serial is None:
            return
        try:
            self.serial.write((json.dumps(obj) + "\n").encode("utf-8"))
        except Exception as e:
            print(f"[pico] write error: {e}")

    def handle_event(self, line):
        try:
            ev = json.loads(line)
        except Exception:
            print(f"[pico] non-JSON: {line!r}")
            return

        e_type = ev.get("event")

        if e_type == "hello":
            print(f"[pico] firmware {ev.get('version', '?')} online")
        elif e_type == "log":
            print(f"[pico:log] {ev.get('msg', '')}")

        elif e_type == "cell":
            cell_id = ev["id"]
            down = ev["down"]
            degree = cell_id + (5 if self.shifted else 0)
            if down:
                if self.hold_mode:
                    self.osc.send_message("/fam/hold", [degree, 1])
                else:
                    # v29-l: cell tap = sustained pad voice
                    self.osc.send_message("/fam/cell", [degree, 0, 1.0])
            else:
                # v29-l: cell release -- gate off the sustained voice
                if not self.hold_mode:
                    self.osc.send_message("/fam/celloff", [degree])

        elif e_type == "mod":
            mid = ev["id"]
            down = ev["down"]
            if down:
                if mid == self.MOD_GENERATE:
                    # v29-p Fix: v29-o trennt /fam/auto (motif-glitter) von
                    # /fam/generative (pad-bed mit Akkord-Progression).
                    # Hardware-GENERATE-Button toggelt das generative Pad-Bed.
                    self.generative = not getattr(self, "generative", False)
                    self.osc.send_message("/fam/generative",
                                          [1 if self.generative else 0])
                elif mid == self.MOD_SHIFT:
                    self.shifted = True
                elif mid == self.MOD_HOLD:
                    self.hold_mode = not self.hold_mode
                    if not self.hold_mode:
                        self.osc.send_message("/fam/holdclear", [])
                elif mid == self.MOD_DRONE:
                    # v29-p NEW: SW8 DRONE-Button toggelt Tonika-Drone-Voice.
                    self.drone = not getattr(self, "drone", False)
                    self.osc.send_message("/fam/drone",
                                          [1 if self.drone else 0])
                elif mid == self.MOD_CLEAR:
                    self.osc.send_message("/fam/holdclear", [])
            else:
                if mid == self.MOD_SHIFT:
                    self.shifted = False

        elif e_type == "enc":
            # v29-p: 4 physical encoders, routed by id.
            #   1=Drive  2=Brightness  3=Display(menu)  4=Volume
            enc_id = ev.get("id", 3)
            delta = ev.get("delta", 0)
            if enc_id == 1:      # Drive macro 0..1
                self.drive = max(0.0, min(1.0, self.drive + delta * 0.05))
                self.osc.send_message("/fam/drive", [self.drive])
            elif enc_id == 2:    # Brightness macro -1..1
                self.brightness = max(-1.0, min(1.0, self.brightness + delta * 0.05))
                self.osc.send_message("/fam/brightness", [self.brightness])
            elif enc_id == 4:    # Volume 0..1
                new_vol = max(0.0, min(1.0, self.sc_state["vol"] + delta * 0.05))
                self.sc_state["vol"] = new_vol
                self.osc.send_message("/fam/vol", [new_vol])
            else:                # id 3 (or unknown) = Display/menu controller
                self.handle_encoder(delta)

        elif e_type == "push":
            # Only the Display encoder (id 3) toggles nav/edit. The other
            # encoders' push acts as a "reset to default" for their macro.
            enc_id = ev.get("id", 3)
            if ev.get("down"):
                if enc_id == 3:
                    self.toggle_mode_ui()
                elif enc_id == 1:
                    self.drive = 0.18  # Sound-Constitution default
                    self.osc.send_message("/fam/drive", [self.drive])
                elif enc_id == 2:
                    self.brightness = -0.2
                    self.osc.send_message("/fam/brightness", [self.brightness])
                elif enc_id == 4:
                    self.sc_state["vol"] = 0.85
                    self.osc.send_message("/fam/vol", [0.85])

        elif e_type == "jack":
            inserted = ev["inserted"]
            self.osc.send_message("/fam/jackdetect", [inserted])
            self.send_to_pico({"set": "amp", "enabled": 0 if inserted else 1})

    def handle_encoder(self, delta):
        if self.mode_ui == "nav":
            new_idx = max(0, min(len(self.MENU_ITEMS) - 1, self.cursor_idx + delta))
            if new_idx != self.cursor_idx:
                self.cursor_idx = new_idx
                self.update_display()
        elif self.mode_ui == "edit":
            self.apply_value_delta(delta)

    def apply_value_delta(self, delta):
        item = self.MENU_ITEMS[self.cursor_idx]
        s = self.sc_state

        if item == "KEY":
            try:
                cur_idx = self.KEY_MIDI.index(s["key"])
            except ValueError:
                cur_idx = 0
            new_idx = max(0, min(6, cur_idx + delta))
            s["key"] = self.KEY_MIDI[new_idx]
            self.osc.send_message("/fam/key", [s["key"]])
        elif item == "MODE":
            # v29-p: SC clamps 0..5 (Locrian removed). Bridge matches.
            s["mode"] = max(0, min(5, s["mode"] + delta))
            self.osc.send_message("/fam/mode", [s["mode"]])
        elif item == "VOICE":
            s["voice"] = max(0, min(2, s["voice"] + delta))
            self.osc.send_message("/fam/voice", [s["voice"]])
        elif item == "PAD":
            s["padVoice"] = max(0, min(2, s["padVoice"] + delta))
            self.osc.send_message("/fam/padvoice", [s["padVoice"]])
        elif item == "OCT":
            s["padOctave"] = max(0, min(2, s["padOctave"] + delta))
            self.osc.send_message("/fam/padoctave", [s["padOctave"]])
        elif item == "PROG":
            # v29-p: SC clamps -1..4 (5 progs, slow_bed at index 0).
            s["prog"] = max(-1, min(4, s["prog"] + delta))
            self.osc.send_message("/fam/prog", [s["prog"]])
        elif item == "TEMPO":
            # v29-p: SC clamps 40..90 per Sound Constitution (never fast).
            s["bpm"] = max(40, min(90, s["bpm"] + delta))
            self.osc.send_message("/fam/tempo", [s["bpm"]])
        elif item == "VIBE":
            # v29-p: SC clamps 0..3 (Sharp removed). Bridge matches.
            s["vibe"] = max(0, min(3, s["vibe"] + delta))
            self.osc.send_message("/fam/vibe", [s["vibe"]])
        elif item == "VOL":
            new_vol = max(0.0, min(1.0, s["vol"] + delta * 0.05))
            s["vol"] = new_vol
            self.osc.send_message("/fam/vol", [new_vol])
        elif item == "MIDI":
            s["midi"] = 1 if (s["midi"] == 0) else 0
            self.osc.send_message("/fam/midi", [s["midi"], 0])
        self.update_display()

    def toggle_mode_ui(self):
        self.mode_ui = "edit" if self.mode_ui == "nav" else "nav"
        self.update_display()

    def midi_to_name(self, m):
        return self.NOTE_NAMES[m % 12] if 0 <= m <= 127 else "?"

    def update_display(self):
        s = self.sc_state
        item = self.MENU_ITEMS[self.cursor_idx]

        try:
            key_idx = self.KEY_MIDI.index(s["key"])
            key_str = self.KEYS_NAMES[key_idx]
        except ValueError:
            key_str = "?"

        # v29-p: bounds updated to match SC v29-o ranges
        mode_str = self.MODE_NAMES[s["mode"]] if 0 <= s["mode"] < 6 else "?"
        prog_str = "AUTO" if s["prog"] < 0 else self.PROG_NAMES[s["prog"]][:4]
        vibe_str = self.VIBE_NAMES[s["vibe"]] if 0 <= s["vibe"] < 4 else "?"

        chord_str = " ".join(self.midi_to_name(m) for m in s["chord"][:5] if m > 0)

        if item == "KEY":
            param, value, bar = "KEY", key_str, int((s["key"] - 48) / 24 * 100)
        elif item == "MODE":
            param, value, bar = "MODE", mode_str, int(s["mode"] / 5 * 100)
        elif item == "VOICE":
            param, value, bar = "VOICE", self.VOICE_NAMES[s["voice"]], int(s["voice"] / 2 * 100)
        elif item == "PAD":
            param, value, bar = "PAD", self.PAD_VOICE_NAMES[s["padVoice"]], int(s["padVoice"] / 2 * 100)
        elif item == "OCT":
            param, value, bar = "OCT", self.PAD_OCTAVE_NAMES[s["padOctave"]], int(s["padOctave"] / 2 * 100)
        elif item == "PROG":
            param, value, bar = "PROG", prog_str, int((s["prog"] + 1) / 5 * 100)
        elif item == "TEMPO":
            # v29-p: SC range 40..90 BPM (was 40..140)
            param, value, bar = "TEMPO", f"{int(s['bpm'])}", int((s["bpm"] - 40) / 50 * 100)
        elif item == "VIBE":
            param, value, bar = "VIBE", vibe_str, int(s["vibe"] / 3 * 100)
        elif item == "VOL":
            param, value, bar = "VOL", f"{int(s['vol'] * 100)}%", int(s["vol"] * 100)
        elif item == "MIDI":
            param, value, bar = "MIDI", "ON" if s["midi"] else "OFF", 100 if s["midi"] else 0
        else:
            param, value, bar = "?", "?", 0

        self.send_to_pico({
            "set": "display",
            "key": key_str,
            "mode": mode_str,
            "prog": prog_str,
            "vibe": vibe_str,
            "chord": chord_str,
            "param": param,
            "value": value,
            "bar_pct": max(0, min(100, bar)),
            "mode_ui": self.mode_ui,
            "cursor": self.cursor_idx,
        })

    def on_sc_state(self, state):
        for k in ("key", "mode", "voice", "prog", "bpm", "vibe", "vol", "midi", "chord",
                  "padVoice", "padOctave"):
            if k in state:
                self.sc_state[k] = state[k]
        # v29-p: mirror generative/drone from SC so the hardware toggle stays
        # in sync if the HTML panel changed these (prevents toggle de-sync).
        if "generative" in state:
            self.generative = bool(state["generative"])
        if "drone" in state:
            self.drone = bool(state["drone"])
        if "drive" in state:
            self.drive = float(state["drive"])
        if "brightness" in state:
            self.brightness = float(state["brightness"])
        self.update_display()


def start_pico_bridge():
    global _pico_bridge
    _pico_bridge = PicoBridge(osc_client)
    thread = threading.Thread(target=_pico_bridge.run, daemon=True)
    thread.start()
    return _pico_bridge


# ============================================================
# Main entry
# ============================================================
async def main():
    global _main_loop
    _main_loop = asyncio.get_running_loop()
    start_osc_server()
    start_pico_bridge()

    import socket
    local_ips = []
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        route_ip = s.getsockname()[0]
        s.close()
        local_ips.append(route_ip)
    except Exception:
        pass

    print(f"[bridge] Field Ambience v29-p bridge running (synced to SC v29-o/p)")
    print(f"[bridge] OSC → SC: {SC_HOST}:{SC_PORT}")
    print(f"[bridge] OSC ← SC: {REPLY_PORT}")
    print(f"[bridge] WebSocket: {WS_HOST}:{WS_PORT}")
    if HAVE_SERIAL:
        print(f"[bridge] Pico hardware: enabled (auto-detect)")
    else:
        print(f"[bridge] Pico hardware: disabled (install pyserial to enable)")
    print()
    if local_ips:
        print(f"[bridge] LAN access from phone/tablet:")
        for ip in local_ips:
            print(f"           http://{ip}:8000/field_ambience_panel.html")
        print(f"         (run  python3 -m http.server 8000  in the folder with the HTML)")
    print()

    async with websockets.serve(ws_handler, WS_HOST, WS_PORT):
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[bridge] stopped by user")
        sys.exit(0)
