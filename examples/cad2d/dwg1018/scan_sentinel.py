# -*- coding: utf-8 -*-
data = open(r"G:\PyArx\examples\cad2d\dwg1018\obj_full.bin","rb").read()
sents = {
 "CLASS":"8D A1 C4 B8 C4 A9 F8 C5 C0 DC F4 5F E7 CF B6 8A",
 "VAR":"CF 7B 1F 23 FD DE 38 A9 5F 7C 68 B8 4E 6D 33 5F",
 "THUMB":"1F 25 6D 07 D4 36 28 28 9D 57 CA 3F 9D 44 10 2B",
 "2ND":"D4 7B 21 CE 28 93 9F BF 53 24 40 09 12 3C AA 01",
}
for k,v in sents.items():
    pat = bytes.fromhex(v)
    i = data.find(pat, 0, 4096)
    print(k, "found @", i)
print("first 16:", data[:16].hex(' '))
