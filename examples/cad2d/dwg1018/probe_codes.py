import sys, collections
sys.path.insert(0, r"G:\PyArx\examples\cad2d\dwg1018")
cnt = collections.Counter(); names = {}
for line in open(r"G:\PyArx\examples\cad2d\dwg1018\ref_map.txt", encoding="utf-8"):
    p = line.rstrip("\n").split("\t")
    if len(p) >= 4:
        cnt[p[2]] += 1
        names.setdefault(p[2], p[3])
for c, n in cnt.most_common():
    print(c, n, names[c])
