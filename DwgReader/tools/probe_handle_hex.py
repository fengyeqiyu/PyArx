# -*- coding: utf-8 -*-
"""查看 6 处偏移 +2 的对象流前后字节，判断差 2 的来源。"""
OBJ = r"G:\PyArx\examples\cad2d\dwg1021\obj_full.bin"
CASES = [
    (1, 10420401, 10420403),
    (14, 15715423, 15715425),
    (5073, 367719, 367721),
    (62738, 3905965, 3905967),
    (65687, 15662626, 15662628),
    (101561, 4620482, 4620484),
]

b = open(OBJ, "rb").read()
print("obj bytes", len(b))


def hd(b, addr):
    return " ".join(f"{x:02x}" for x in b[addr - 10:addr + 6])


for idx, calc, ref in CASES:
    print(f"#{idx} calc={calc} ref={ref}")
    print(f"  @calc-10..+6 : {hd(b, calc)}")
    print(f"  @ref -10..+6 : {hd(b, ref)}")
    # 前一条对象的地址若已知无用，打印 calc 处原字节含义
    print(f"  bytes@calc: {b[calc]:02x} {b[calc+1]:02x}  bytes@ref: {b[ref]:02x} {b[ref+1]:02x}")
