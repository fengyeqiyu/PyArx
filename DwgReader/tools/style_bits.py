p = r"G:\PyArx\samples\dwg_versions\v_2000.dwg"
data = open(p, "rb").read()
a = 14395432
print("file bytes from a:", " ".join(f"{x:02X}" for x in data[a:a+20]))
def bits(bpos, n):
    return "".join(str((data[(bpos+i)//8] >> (7-((bpos+i)%8))) & 1) for i in range(n))
for off in (0, 16, 42, 50, 58, 66):
    print(f"off{off:3d}", bits(a*8+off, 40))
