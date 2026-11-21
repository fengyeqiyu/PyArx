import io
p=r"G:\PyArx\examples\ui\power_plan_shell.xml"
s=io.open(p,encoding="utf-8").read()
rows=[("P0","10kV 总进线","0","YJV22-3x70"),("P1","1# 双电源柜","45","YJV22-4x95"),
      ("P2","2# 配电柜","36","YJV22-4x70"),("T11","中试车间照明","11","YJV22-4x35"),
      ("T12","车间动力 1#","22","YJV22-4x50"),("T13","涂布机主控","8.5","YJV22-4x25"),
      ("T21","空压站","15","YJV22-4x35"),("T22","空调机房","30","YJV22-4x50"),
      ("T23","办公区照明","6","YJV22-4x16"),("T24","UPS 机房","9","YJV22-4x25"),
      ("L111","车间插座回路","3.5","YJV22-4x16"),("L112","应急照明","1.5","YJV22-4x10")]
rowsattr="|".join("\t".join(r) for r in rows)
anchor='        <XDwgCanvas Name="Mini" Width="316" Height="160" Mini="1" MiniOf="Plan"/>'
block=anchor+"\n"+"        <XDivider Height=\"1\"/>\n"+"        <XLabel Content=\"设备清单（数据页）\" Height=\"28\"/>\n"+"        <TableView Width=\"316\" Height=\"164\" Columns=\"设备:56|名称:104|kW:44|电缆:110\" Rows=\""+rowsattr+"\"/>"
assert anchor in s, "anchor missing"
s=s.replace(anchor, block, 1)
io.open(p,"w",encoding="utf-8",newline="\n").write(s)
print("rows", len(rowsattr))