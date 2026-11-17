"""Best-effort AcDb:Header variable diagnostic for AC1021 samples.

This lives outside the PyArxDwg container package: it walks the R2007 header
bit stream to expose the system variables the object layer will consume.
Field order follows the ODA DWG header description; behavior cross-checked
against public ACadSharp (MIT) field layout.  Pure stdlib, no DXF input.

Run: python tests/dwg/check_header_vars.py
"""

from __future__ import annotations

import os
import struct
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

from PyArxDwg.container import open_dwg

HERE = os.path.dirname(os.path.abspath(__file__))
SAMPLES = [
    os.path.join(HERE, "..", "楼层平面-10KV电缆桥架.dwg"),
    os.path.join(HERE, "..", "楼层平面-强电.dwg"),
]


class BitReader:
    """MSB-first DWG bit stream over a byte buffer (R2007 AC18 base layout)."""

    def __init__(self, data: bytes, bit_pos: int = 0):
        self.data = data
        self.byte_pos = bit_pos >> 3
        self.shift = bit_pos & 7
        self.last = 0
        if self.shift:
            self._advance()

    def position_bits(self) -> int:
        return self.byte_pos * 8 + self.shift - 8

    def set_position(self, bit_pos: int) -> None:
        self.byte_pos = bit_pos >> 3
        self.shift = bit_pos & 7
        if self.shift:
            self._advance()

    def _advance(self) -> None:
        if self.byte_pos >= len(self.data):
            raise EOFError("bit stream overrun")
        self.last = self.data[self.byte_pos]
        self.byte_pos += 1

    def read_byte(self) -> int:
        if self.shift == 0:
            self._advance()
            return self.last
        tail = (self.last << self.shift) & 0xFF
        self._advance()
        return (tail | (self.last >> (8 - self.shift))) & 0xFF

    def read_bytes(self, n: int) -> bytes:
        out = bytearray(n)
        if self.shift == 0:
            for i in range(n):
                self._advance()
                out[i] = self.last
            return bytes(out)
        for i in range(n):
            tail = (self.last << self.shift) & 0xFF
            self._advance()
            out[i] = (tail | (self.last >> (8 - self.shift))) & 0xFF
        return bytes(out)

    def read_short(self) -> int:
        return struct.unpack_from("<h", self.read_bytes(2), 0)[0]

    def read_ushort(self) -> int:
        return struct.unpack_from("<H", self.read_bytes(2), 0)[0]

    def read_int(self) -> int:
        return struct.unpack_from("<i", self.read_bytes(4), 0)[0]

    def read_double(self) -> float:
        return struct.unpack_from("<d", self.read_bytes(8), 0)[0]

    def read_bit(self) -> int:
        if self.shift == 0:
            self._advance()
            value = (self.last & 0x80) != 0
            self.shift = 1
            return int(value)
        value = ((self.last << self.shift) & 0x80) != 0
        self.shift = (self.shift + 1) & 7
        return int(value)

    def read_2bits(self) -> int:
        if self.shift == 0:
            self._advance()
            value = self.last >> 6
            self.shift = 2
            return value
        if self.shift == 7:
            tail = (self.last << 1) & 2
            self._advance()
            value = tail | (self.last >> 7)
            self.shift = 1
            return value
        value = (self.last >> (6 - self.shift)) & 3
        self.shift = (self.shift + 2) & 7
        return value

    def bit_long(self) -> int:
        code = self.read_2bits()
        if code == 0:
            return self.read_int()
        if code == 1:
            return self.read_byte()
        if code == 2:
            return 0
        raise ValueError("invalid BitLong code 3")

    def bit_short(self) -> int:
        code = self.read_2bits()
        if code == 0:
            return self.read_short()
        if code == 1:
            return self.read_byte()
        if code == 2:
            return 0
        return 256

    def bit_double(self) -> float:
        code = self.read_2bits()
        if code == 0:
            return self.read_double()
        if code == 1:
            return 1.0
        if code == 2:
            return 0.0
        raise ValueError("invalid BitDouble code 3")

    def variable_text(self) -> str:
        try:
            length = self.bit_short()
            if length <= 0:
                return ""
            raw = self.read_bytes(length * 2)
            return raw.decode("utf-16-le", errors="replace").rstrip("\0")
        except (EOFError, ValueError, IndexError, struct.error):
            return ""

    def handle_reference(self) -> int:
        form = self.read_byte()
        code = form >> 4
        counter = form & 0x0F
        raw = self.read_bytes(counter)
        handle = int.from_bytes(raw, "little")
        if code <= 5:
            return handle
        if code == 6:
            return handle + 1
        if code == 8:
            return handle - 1
        if code == 0xA:
            return handle + handle
        if code == 0xC:
            return handle - handle
        raise ValueError(f"invalid handle reference code {code}")

    def cm_color(self) -> tuple[int, int, str]:
        index = self.bit_short()
        rgb = self.bit_long() & 0xFFFFFFFF
        byte_id = self.read_byte()
        name = self.variable_text() if byte_id & 1 else ""
        if byte_id & 2:
            self.variable_text()
        return index, rgb, name


def _setup_text_stream(header: bytes, last_pos: int) -> BitReader | None:
    text = BitReader(header, last_pos)
    flag = text.read_bit()
    if not flag:
        return None
    length = last_pos - 128
    text.set_position(length)
    str_size = text.read_ushort()
    if str_size & 0x8000:
        length -= 128
        text.set_position(length)
        hi = text.read_ushort()
        str_size = (str_size & 0x7FFF) | (hi << 15)
    start = length - str_size
    text.set_position(start)
    return text


def read_header_vars(header: bytes) -> dict:
    size = struct.unpack_from("<I", header, 0x10)[0]
    size_bits = struct.unpack_from("<I", header, 0x14)[0]
    initial_pos = 0x18 * 8
    last_pos = initial_pos + size_bits - 1

    main = BitReader(header, initial_pos)
    text = _setup_text_stream(header, last_pos)
    handle = BitReader(header, last_pos + 1)

    out: dict = {}

    # leading unknown doubles / texts / longs
    for _ in range(4):
        main.bit_double()
    for _ in range(4):
        text.variable_text() if text else ""
    main.bit_long()
    main.bit_long()

    # flag bits (R2007 layout)
    for _ in range(2):  # DIMASO, DIMSHO
        main.read_bit()
    for _ in range(7):  # PLINEGEN ORTHOMODE REGENMODE FILLMODE QTEXTMODE PSLTSCALE LIMCHECK
        main.read_bit()
    main.read_bit()  # R2004 undocumented
    main.read_bit()  # USRTIMER
    main.read_bit()  # SKPOLY
    main.read_bit()  # ANGDIR
    main.read_bit()  # SPLFRAME
    main.read_bit()  # MIRRTEXT
    main.read_bit()  # WORLDVIEW
    main.read_bit()  # TILEMODE
    main.read_bit()  # PLIMCHECK
    main.read_bit()  # VISRETAIN
    main.read_bit()  # DISPSILH
    main.read_bit()  # PELLIPSE
    main.bit_short()  # PROXYGRAPHICS

    # bit shorts
    main.bit_short()  # TREEDEPTH
    main.bit_short()  # LUNITS
    main.bit_short()  # LUPREC
    main.bit_short()  # AUNITS
    main.bit_short()  # AUPREC
    main.bit_short()  # ATTMODE
    main.bit_short()  # PDMODE
    for _ in range(3):  # R2004 unknown longs
        main.bit_long()
    for _ in range(5):  # USERI1..5
        main.bit_short()
    main.bit_short()  # SPLINESEGS
    main.bit_short()  # SURFU
    main.bit_short()  # SURFV
    main.bit_short()  # SURFTYPE
    main.bit_short()  # SURFTAB1
    main.bit_short()  # SURFTAB2
    main.bit_short()  # SPLINETYPE
    main.bit_short()  # SHADEDGE
    main.bit_short()  # SHADEDIF
    main.bit_short()  # UNITMODE
    main.bit_short()  # MAXACTVP
    main.bit_short()  # ISOLINES
    main.bit_short()  # CMLJUST
    main.bit_short()  # TEXTQLTY

    # doubles
    for _ in range(20):  # LTSCALE ... CELTSCALE
        main.bit_double()
    text.variable_text() if text else ""  # MENUNAME
    main.bit_long()  # TDCREATE day
    main.bit_long()  # TDCREATE ms
    main.bit_long()  # TDUPDATE day
    main.bit_long()  # TDUPDATE ms
    for _ in range(3):  # R2004 unknown longs
        main.bit_long()
    main.bit_long()  # TDINDWG day
    main.bit_long()  # TDINDWG ms
    main.bit_long()  # TDUSRTIMER day
    main.bit_long()  # TDUSRTIMER ms
    main.cm_color()  # CECOLOR

    out["HANDSEED"] = main.handle_reference()
    for _ in range(3):  # CLAYER TEXTSTYLE CELTYPE
        handle.handle_reference()
    handle.handle_reference()  # CMATERIAL
    for _ in range(2):  # DIMSTYLE CMLSTYLE
        handle.handle_reference()
    main.bit_double()  # PSVPSCALE

    # paper space block
    for _ in range(3):  # INSBASE/EXTMIN/EXTMAX PSPACE
        main.bit_double()
    for _ in range(3):  # 2RD LIMMIN LIMMAX, elevation
        main.bit_double()
    for _ in range(3):  # UCSORG XDIR YDIR PSPACE
        main.bit_double()
    handle.handle_reference()  # UCSNAME PSPACE
    handle.handle_reference()  # PUCSORTHOREF
    main.bit_short()  # PUCSORTHOVIEW
    handle.handle_reference()  # PUCSBASE
    for _ in range(6):  # ortho origins PSPACE
        main.bit_double()

    # model space block (target: EXTMIN/EXTMAX MSPACE)
    main.bit_double()  # INSBASE MSPACE
    out["EXTMIN_MSPACE"] = (main.bit_double(), main.bit_double(), main.bit_double())
    out["EXTMAX_MSPACE"] = (main.bit_double(), main.bit_double(), main.bit_double())
    for _ in range(2):  # 2RD LIMMIN LIMMAX
        main.bit_double()
    main.bit_double()  # ELEVATION
    for _ in range(3):  # UCSORG XDIR YDIR MSPACE
        main.bit_double()
    handle.handle_reference()  # UCSNAME MSPACE
    handle.handle_reference()  # UCSORTHOREF
    main.bit_short()  # UCSORTHOVIEW
    handle.handle_reference()  # UCSBASE
    for _ in range(6):  # ortho origins MSPACE
        main.bit_double()
    text.variable_text() if text else ""  # DIMPOST
    text.variable_text() if text else ""  # DIMAPOST

    # dimension doubles
    for _ in range(9):  # DIMSCALE..DIMTM
        main.bit_double()
    main.bit_double()  # DIMFXL
    main.bit_double()  # DIMJOGANG
    main.bit_short()  # DIMTFILL
    main.cm_color()  # DIMTFILLCLR
    main.read_bit(); main.read_bit(); main.read_bit(); main.read_bit()  # DIMTOL DIMLIM DIMTIH DIMTOH
    main.read_bit(); main.read_bit()  # DIMSE1 DIMSE2
    main.bit_short(); main.bit_short(); main.bit_short()  # DIMTAD DIMZIN DIMAZIN
    main.bit_short()  # DIMARCSYM
    for _ in range(8):  # DIMTXT .. DIMGAP
        main.bit_double()
    main.bit_double(); main.read_bit(); main.bit_short()  # DIMALTRND DIMALT DIMALTD
    main.read_bit(); main.read_bit(); main.read_bit(); main.read_bit()  # DIMTOFL DIMSAH DIMTIX DIMSOXD
    main.cm_color(); main.cm_color(); main.cm_color()  # DIMCLRD E T
    for _ in range(12):  # DIMADEC ... DIMATFIT shorts/bits
        main.bit_short()
    main.read_bit()  # DIMFXLON
    for _ in range(5):  # DIMTXSTY LDRBLK BLK BLK1 BLK2
        handle.handle_reference()
    for _ in range(3):  # DIMLTYPE LTEX1 LTEX2
        handle.handle_reference()
    main.bit_short(); main.bit_short()  # DIMLWD DIMLWE
    for _ in range(10):  # control objects
        handle.handle_reference()
    handle.handle_reference()  # ACAD_GROUP
    handle.handle_reference()  # ACAD_MLINESTYLE
    handle.handle_reference()  # NAMED_OBJECTS
    main.bit_short(); main.bit_short()  # TSTACKALIGN TSTACKSIZE
    text.variable_text() if text else ""  # HYPERLINKBASE
    text.variable_text() if text else ""  # STYLESHEET
    for _ in range(3):  # LAYOUTS PLOTSETTINGS PLOTSTYLES
        handle.handle_reference()
    handle.handle_reference()  # MATERIALS
    handle.handle_reference()  # COLORS
    handle.handle_reference()  # VISUALSTYLE
    main.bit_long()  # flags
    out["INSUNITS"] = main.bit_short()
    main.bit_short()  # CEPSNTYPE
    text.variable_text() if text else ""  # FINGERPRINTGUID
    text.variable_text() if text else ""  # VERSIONGUID
    for _ in range(6):  # SORTENTS INDEXCTL HIDETEXT XCLIPFRAME DIMASSOC HALOGAP
        main.read_byte()
    main.bit_short(); main.bit_short()  # OBSCUREDCOLOR INTERSECTIONCOLOR
    main.read_byte(); main.read_byte()  # OBSCUREDLTYPE INTERSECTIONDISPLAY
    text.variable_text() if text else ""  # PROJECTNAME

    out["BLOCK_RECORD_PSPACE"] = handle.handle_reference()
    out["BLOCK_RECORD_MSPACE"] = handle.handle_reference()
    for _ in range(3):  # BYLAYER BYBLOCK CONTINUOUS
        handle.handle_reference()

    return out


def main() -> int:
    for sample in SAMPLES:
        dwg = open_dwg(sample)
        header = dwg.section_bytes("AcDb:Header")
        print(f"\n=== {os.path.basename(sample)}  codepage={dwg.codepage}")
        try:
            vars_ = read_header_vars(header)
        except (EOFError, ValueError, IndexError, struct.error) as exc:
            print(f"  header bitstream not aligned yet: {exc}")
            print("  INSUNITS/EXTMIN/EXTMAX/HANDSEED/block records: pending object-stage decode")
            continue
        for key in (
            "INSUNITS", "HANDSEED", "EXTMIN_MSPACE", "EXTMAX_MSPACE",
            "BLOCK_RECORD_MSPACE", "BLOCK_RECORD_PSPACE",
        ):
            print(f"  {key}: {vars_[key]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
