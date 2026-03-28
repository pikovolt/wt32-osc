"""
wt32_sender.py - Wave Data Sender for WT32-OSC
Sends parameters and wave data to NTS-1 mkII via MIDI.

Based on NTS-1 mkII MIDI Implementation Chart v1.00 (2024.03.18):
  - OSC A/B (shape/altshape): 14-bit CC pair
    - OSC A: CC#54 (MSB) + CC#102 (LSB)
    - OSC B: CC#55 (MSB) + CC#103 (LSB)
  - OSC EDIT PARAM1-8: NRPN (MSB=0, LSB=0..7)
    - Data Entry: CC#38 (LSB first) + CC#6 (MSB), 14-bit (0-16383)

Requirements:
    pip install python-rtmidi

Usage:
    python wt32_sender.py --list
    python wt32_sender.py --port 0 --demo
    python wt32_sender.py --port 0 --interactive
    python wt32_sender.py --port 0 --formula "sin(x * 2 * pi)" --target user:0
"""

import argparse
import sys
import time
import math
import struct

try:
    import rtmidi
except ImportError:
    print("Error: python-rtmidi required.")
    print("  pip install python-rtmidi")
    sys.exit(1)

# ============================================================================
#  MIDI CC/NRPN Mapping (from NTS-1 mkII MIDI Implementation Chart)
# ============================================================================

# OSC A (shape = params[0]): 14-bit CC pair
CC_OSC_A_MSB = 54
CC_OSC_A_LSB = 102

# OSC B (altshape = params[1]): 14-bit CC pair
CC_OSC_B_MSB = 55
CC_OSC_B_LSB = 103

# NRPN control CCs
CC_NRPN_MSB      = 99
CC_NRPN_LSB      = 98
CC_DATA_ENTRY_MSB = 6
CC_DATA_ENTRY_LSB = 38

# NRPN addresses for OSC EDIT PARAMs (MSB=0, LSB=param_index)
NRPN_OSC_MSB = 0
# NRPN LSB 0-7 = OSC EDIT PARAM1-8 = SDK params[2]-[9]

# ============================================================================
#  WT32-OSC Parameter Definitions (must match header.c)
# ============================================================================

# params[0] = A knob (shape):    BnkA  min=0 max=1023
# params[1] = B knob (altshape): Mrph  min=0 max=1023
# params[2] = NRPN PARAM1:       BnkB  min=0 max=63
# params[3] = NRPN PARAM2:       Ctrl  min=0 max=15
# params[4] = NRPN PARAM3:       DTun  min=0 max=100
# params[5] = NRPN PARAM4:       WrAd  min=0 max=255
# params[6] = NRPN PARAM5:       WrDt  min=0 max=255

PARAM_RANGES = {
    'BnkA': {'index': 0, 'min': 0, 'max': 1023, 'type': 'cc14',
             'desc': 'Stage A bank (0-63, scaled from 0-1023)'},
    'Mrph': {'index': 1, 'min': 0, 'max': 1023, 'type': 'cc14',
             'desc': 'Morph A<>B (0-1023)'},
    'BnkB': {'index': 2, 'min': 0, 'max': 63,   'type': 'nrpn', 'nrpn_lsb': 0,
             'desc': 'Stage B bank (0-63)'},
    'Ctrl': {'index': 3, 'min': 0, 'max': 15,   'type': 'nrpn', 'nrpn_lsb': 1,
             'desc': 'Control: bit[2:0]=bit-depth, bit[3]=interp'},
    'DTun': {'index': 4, 'min': 0, 'max': 100,  'type': 'nrpn', 'nrpn_lsb': 2,
             'desc': 'Detune (0-100 = 0-50 cents)'},
    'WrAd': {'index': 5, 'min': 0, 'max': 255,  'type': 'nrpn', 'nrpn_lsb': 3,
             'desc': 'Write address'},
    'WrDt': {'index': 6, 'min': 0, 'max': 255,  'type': 'nrpn', 'nrpn_lsb': 4,
             'desc': 'Write data (auto-increment)'},
}

# ============================================================================
#  Wave Generation Helpers
# ============================================================================

def generate_sine(harmonics=None):
    """Generate 32-sample waveform from harmonic specification."""
    if harmonics is None:
        harmonics = {1: 1.0}

    wave = [0.0] * 32
    for i in range(32):
        phase = i / 32.0 * 2.0 * math.pi
        sample = 0.0
        for h, amp in harmonics.items():
            sample += amp * math.sin(phase * h)
        wave[i] = sample

    max_val = max(abs(s) for s in wave) or 1.0
    return [int(max(min(s / max_val * 127, 127), -128)) for s in wave]


def generate_from_formula(formula_str):
    """Generate wave from formula. Variable 'x' = phase (0.0 to 1.0)."""
    wave = []
    safe_env = {
        "__builtins__": {},
        "sin": math.sin, "cos": math.cos, "tan": math.tan,
        "pi": math.pi, "abs": abs, "pow": pow,
        "sqrt": math.sqrt, "floor": math.floor, "ceil": math.ceil,
        "exp": math.exp, "log": math.log,
    }
    for i in range(32):
        x = i / 32.0
        safe_env["x"] = x
        try:
            val = eval(formula_str, safe_env)
            if not isinstance(val, (int, float)):
                print(f"  Warning: sample {i} is not a number ({type(val).__name__}), using 0")
                val = 0
        except Exception as e:
            print(f"  Error at sample {i}: {e}")
            val = 0
        wave.append(float(val))

    max_val = max(abs(s) for s in wave) or 1.0
    return [int(max(min(s / max_val * 127, 127), -128)) for s in wave]


# ============================================================================
#  MIDI Communication
# ============================================================================

class WT32Sender:
    def __init__(self, port_index=0, channel=0):
        self.midi_out = rtmidi.MidiOut()
        ports = self.midi_out.get_ports()
        if not ports:
            raise RuntimeError("No MIDI output ports found.")
        if port_index >= len(ports):
            raise RuntimeError(f"Port {port_index} not found. Available: {ports}")
        self.midi_out.open_port(port_index)
        self.channel = channel
        print(f"Opened MIDI port: {ports[port_index]}")
        print(f"  MIDI channel: {self.channel} (0-indexed, = ch{self.channel+1})")

    def send_cc(self, cc, value):
        """Send a single MIDI CC message (7-bit value)."""
        value = max(0, min(127, int(value)))
        # print(f"    CC#{cc:3d}  {value:3d}  (ch={self.channel})")
        self.midi_out.send_message([0xB0 | self.channel, cc, value])

    def send_cc14(self, cc_msb, cc_lsb, value14):
        """Send a 14-bit CC value as MSB+LSB pair."""
        value14 = max(0, min(16383, int(value14)))
        msb = (value14 >> 7) & 0x7F
        lsb = value14 & 0x7F
        self.send_cc(cc_lsb, lsb)
        time.sleep(0.001)
        self.send_cc(cc_msb, msb)

    def send_nrpn(self, nrpn_msb, nrpn_lsb, value14):
        """Send a 14-bit value via NRPN.

        Sequence: CC#99(NRPN MSB) -> CC#98(NRPN LSB) ->
                  CC#38(Data LSB) -> CC#6(Data MSB)
        """
        value14 = max(0, min(16383, int(value14)))
        data_msb = (value14 >> 7) & 0x7F
        data_lsb = value14 & 0x7F

        self.send_cc(CC_NRPN_MSB, nrpn_msb)
        time.sleep(0.001)
        self.send_cc(CC_NRPN_LSB, nrpn_lsb)
        time.sleep(0.001)
        self.send_cc(CC_DATA_ENTRY_LSB, data_lsb)
        time.sleep(0.001)
        self.send_cc(CC_DATA_ENTRY_MSB, data_msb)

    def set_param(self, name, value, delay=0.005):
        if name not in PARAM_RANGES:
            print(f"  Unknown parameter: {name}")
            return

        p = PARAM_RANGES[name]
        value = max(p['min'], min(p['max'], int(value)))

        if p['type'] == 'cc14':
            # CC14: NTS-1 が 14-bit→param range に線形スケールするため、逆変換が必要
            param_range = p['max'] - p['min']
            midi14 = min(16383, math.ceil((value - p['min']) * 16383 / param_range)) if param_range > 0 else 0
            if p['index'] == 0:
                self.send_cc14(CC_OSC_A_MSB, CC_OSC_A_LSB, midi14)
            elif p['index'] == 1:
                self.send_cc14(CC_OSC_B_MSB, CC_OSC_B_LSB, midi14)

        elif p['type'] == 'nrpn':
            # NTS-1 mkII は CC14 と同じ linear scale (0-16383) で解釈する
            param_range = p['max'] - p['min']
            midi14 = min(16383, math.ceil((value - p['min']) * 16383 / param_range)) if param_range > 0 else 0
            self.send_nrpn(NRPN_OSC_MSB, p['nrpn_lsb'], midi14)

        time.sleep(delay)

    def select_bank(self, stage, bank_index):
        if stage == 'a':
            param_val = math.ceil(bank_index * 1023 / 63)   # ← ceil
            self.set_param('BnkA', param_val)
            print(f"  Stage A -> bank {bank_index} (param={param_val})")
        elif stage == 'b':
            self.set_param('BnkB', bank_index)
            print(f"  Stage B -> bank {bank_index}")

    def set_morph(self, percent):
        """Set morph position. percent: 0-100."""
        param_val = int(percent * 1023 / 100)
        self.set_param('Mrph', param_val)

    def write_wave(self, target, wave_data, delay=0.008):
        """Write a 32-byte waveform to the specified target.

        Args:
            target: 'stage_a', 'stage_b', or 'user:N' (N=0-15)
            wave_data: list of 32 signed 8-bit integers (-128..127)
            delay: inter-message delay in seconds
        """
        if len(wave_data) != 32:
            raise ValueError(f"Wave data must be 32 samples, got {len(wave_data)}")

        # Determine WrAd value (matches unit.cc apply_param logic)
        if target == 'stage_a':
            addr_base = 0       # 0-31 = Stage A
        elif target == 'stage_b':
            addr_base = 32      # 32-63 = Stage B
        elif target.startswith('user:'):
            bank = int(target.split(':')[1])
            if bank < 0 or bank > 5:
                raise ValueError("User bank must be 0-5 (WrAd max=255 limits addressable banks)")
            addr_base = 64 + bank * 32   # 64+ = User banks
        else:
            raise ValueError(f"Unknown target: {target}")

        # Set write address via NRPN
        self.set_param('WrAd', addr_base)
        time.sleep(delay)

        # Write 32 samples via WrDt
        for i, sample in enumerate(wave_data):
            # Convert signed (-128..127) to unsigned (0..255)
            unsigned_val = (sample + 128) & 0xFF
            self.set_param('WrDt', unsigned_val)
            time.sleep(delay)

        print(f"  Wrote {len(wave_data)} samples to {target}")

    def close(self):
        self.midi_out.close_port()


# ============================================================================
#  CLI Commands
# ============================================================================

def cmd_list_ports():
    """List available MIDI ports."""
    out = rtmidi.MidiOut()
    ports = out.get_ports()
    if not ports:
        print("No MIDI output ports found.")
        return
    print("Available MIDI output ports:")
    for i, name in enumerate(ports):
        print(f"  [{i}] {name}")


def cmd_demo(sender):
    """Send demo waveforms to user banks 0-5.
    Note: WrAd max=255 limits addressable user banks to 0-5.
    """
    demos = [
        ("Odd harmonics",  generate_sine({1: 1.0, 3: 0.5, 5: 0.25, 7: 0.125})),
        ("Even harmonics", generate_sine({1: 1.0, 2: 0.6, 4: 0.3, 6: 0.15})),
        ("Bell",           generate_sine({1: 1.0, 2.76: 0.7, 4.53: 0.4, 5.4: 0.3})),
        ("PWM 25%",        generate_from_formula("1 if x < 0.25 else -1")),
        ("Staircase",      generate_from_formula("floor(x * 8) / 4 - 1")),
        ("Rectified sine", generate_from_formula("abs(sin(x * 2 * pi))")),
    ]

    for i, (name, data) in enumerate(demos):
        print(f"Writing user bank {i}: {name}")
        sender.write_wave(f"user:{i}", data)
        time.sleep(0.1)

    print()
    print("Demo waves loaded into user banks 0-5.")
    print("To select them:")
    print("  bank a 48  -> user:0 (Odd harmonics)")
    print("  bank a 49  -> user:1 (Even harmonics)")
    print("  bank a 50  -> user:2 (Bell)")
    print("  bank a 51  -> user:3 (PWM 25%)")
    print("  bank a 52  -> user:4 (Staircase)")
    print("  bank a 53  -> user:5 (Rectified sine)")


def cmd_formula(sender, formula, target):
    """Generate wave from formula and send."""
    print(f"Generating from formula: {formula}")
    data = generate_from_formula(formula)
    print(f"Wave data: {data}")
    sender.write_wave(target, data)


def cmd_file(sender, filepath, target):
    """Load wave from binary file (32 bytes, signed int8) and send."""
    with open(filepath, 'rb') as f:
        raw = f.read(32)
    if len(raw) != 32:
        print(f"Error: file must be exactly 32 bytes, got {len(raw)}")
        return
    data = list(struct.unpack('32b', raw))
    print(f"Loaded from {filepath}: {data}")
    sender.write_wave(target, data)


def cmd_interactive(sender):
    """Interactive wave editor in terminal."""
    print()
    print("=== WT32 Interactive Wave Editor ===")
    print("Commands:")
    print("  formula <expr>              - Generate wave (var: x = 0..1)")
    print("  harmonics <h:a h:a ...>     - Generate from harmonics")
    print("  send <target>               - Send wave (stage_a / stage_b / user:N)")
    print("  bank a|b <index>            - Select bank (0-63)")
    print("  morph <0-100>               - Set morph position")
    print("  param <name> <value>        - Set raw parameter")
    print("  show                        - Display current wave")
    print("  help                        - Show this help")
    print("  quit                        - Exit")
    print()

    current_wave = generate_sine()

    while True:
        try:
            line = input("wt32> ").strip()
        except (EOFError, KeyboardInterrupt):
            print()
            break

        if not line:
            continue

        parts = line.split(None, 1)
        cmd = parts[0].lower()
        arg = parts[1] if len(parts) > 1 else ""

        if cmd in ('quit', 'q', 'exit'):
            break

        elif cmd == 'help':
            print("  formula <expr>     - e.g. formula sin(x * 2 * pi)")
            print("  harmonics <h:a>    - e.g. harmonics 1:1.0 3:0.5 5:0.25")
            print("  send <target>      - stage_a, stage_b, user:0..15")
            print("  bank a|b <N>       - e.g. bank a 6  (brass preset)")
            print("  morph <0-100>      - e.g. morph 50")
            print("  param <name> <val> - e.g. param Ctrl 8")
            print("  show               - ASCII waveform display")

        elif cmd == 'formula' and arg:
            current_wave = generate_from_formula(arg)
            print(f"  Generated: {current_wave[:8]}... ({len(current_wave)} samples)")

        elif cmd == 'harmonics' and arg:
            harmonics = {}
            for token in arg.split():
                if ':' in token:
                    h, a = token.split(':')
                    harmonics[float(h)] = float(a)
            current_wave = generate_sine(harmonics)
            print(f"  Generated: {current_wave[:8]}... ({len(current_wave)} samples)")

        elif cmd == 'send' and arg:
            target = arg.strip()
            sender.write_wave(target, current_wave)

        elif cmd == 'bank' and arg:
            tokens = arg.split()
            if len(tokens) == 2:
                stage = tokens[0].lower()
                idx = int(tokens[1])
                sender.select_bank(stage, idx)
            else:
                print("  Usage: bank a|b <index>")

        elif cmd == 'morph' and arg:
            sender.set_morph(int(arg))
            print(f"  Morph set to {arg}%")

        elif cmd == 'param' and arg:
            tokens = arg.split()
            if len(tokens) == 2:
                sender.set_param(tokens[0], int(tokens[1]))
                print(f"  Set {tokens[0]} = {tokens[1]}")
            else:
                print("  Usage: param <name> <value>")
                print("  Names: BnkA, Mrph, BnkB, Ctrl, DTun, WrAd, WrDt")

        elif cmd == 'show':
            print("  " + "-" * 34)
            for i, sv in enumerate(current_wave):
                bar_pos = int((sv + 128) / 256 * 32)
                bar = " " * bar_pos + "#"
                print(f"  {i:2d}| {bar}")
            print("  " + "-" * 34)
            print(f"  Raw: {current_wave}")

        else:
            print(f"  Unknown command: {cmd}. Type 'help' for commands.")

    print("Bye.")


# ============================================================================
#  Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(
        description="WT32-OSC Wave Data Sender for NTS-1 mkII",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
MIDI protocol (from NTS-1 mkII Implementation Chart):
  OSC A/B (knobs):     14-bit CC pairs (CC#54+102, CC#55+103)
  OSC EDIT PARAM1-8:   NRPN (MSB=0, LSB=0-7), Data 14-bit

Examples:
  %(prog)s --list
  %(prog)s --port 0 --demo
  %(prog)s --port 0 --formula "sin(x * 2 * pi)" --target user:0
  %(prog)s --port 0 --interactive
""")

    parser.add_argument('--list', action='store_true',
                        help='List available MIDI ports')
    parser.add_argument('--port', type=int, default=0,
                        help='MIDI output port index')
    parser.add_argument('--channel', type=int, default=1,
                        help='MIDI channel (1-16)')
    parser.add_argument('--demo', action='store_true',
                        help='Send demo waveforms to user banks')
    parser.add_argument('--formula', type=str,
                        help='Generate wave from formula (var: x=0..1)')
    parser.add_argument('--file', type=str,
                        help='Load wave from binary file (32 bytes signed)')
    parser.add_argument('--target', type=str, default='stage_a',
                        help='Write target: stage_a, stage_b, user:N')
    parser.add_argument('--interactive', action='store_true',
                        help='Interactive wave editor')

    args = parser.parse_args()

    if args.list:
        cmd_list_ports()
        return

    sender = WT32Sender(args.port, args.channel - 1)

    try:
        if args.demo:
            cmd_demo(sender)
        elif args.formula:
            cmd_formula(sender, args.formula, args.target)
        elif args.file:
            cmd_file(sender, args.file, args.target)
        elif args.interactive:
            cmd_interactive(sender)
        else:
            parser.print_help()
    finally:
        sender.close()


if __name__ == '__main__':
    main()