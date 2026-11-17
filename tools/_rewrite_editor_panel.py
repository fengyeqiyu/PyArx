import io,re
p=r"G:\PyArx\examples\ui\power_plan_shell.xml"
s=io.open(p,encoding="utf-8").read()
m=re.search(r'\n        <XStackPanel Name="NodeEditor".*?\n        </XStackPanel>\n', s, re.S)
assert m, "panel block missing"
rows=[("P0","10kV 总进线","—","YJV22-3x70","10kV","0","—"),
      ("P1","1# 双电源柜","P0","YJV22-4x95","380V","45","22"),
      ("P2","2# 配电柜","P0","YJV22-4x70","380V","36","18"),
      ("T11","中试车间照明","P1","YJV22-4x35","380V","11","12"),
      ("T12","车间动力 1#","P1","YJV22-4x50","380V","22","26"),
      ("T13","涂布机主控","P1","YJV22-4x25","380V","8.5","10"),
      ("T21","空压站","P2","YJV22-4x35","380V","15","14"),
      ("T22","空调机房","P2","YJV22-4x50","380V","30","32"),
      ("T23","办公区照明","P2","YJV22-4x16","220V","6","6"),
      ("T24","UPS 机房","P2","YJV22-4x25","380V","9","8"),
      ("L111","车间插座回路","T12","YJV22-4x16","220V","3.5","3"),
      ("L112","应急照明","T11","YJV22-4x10","220V","1.5","2")]
rowsattr="|".join("\t".join(r) for r in rows)
fields=[("lblEdCode","编号　—"),("lblEdName","名称　—"),("lblEdUp","上游　—"),
        ("lblEdCable","电缆　—"),("lblEdVolt","电压　—"),("lblEdLoad","负载　— kW"),("lblEdLen","长度　— m")]
fl="\n".join('          <XLabel Name="%s" Left="20" Top="%d" Width="592" Height="28" Content="%s"/>' % (n, 366+i*30, c) for i,(n,c) in enumerate(fields))
block='''
        <XStackPanel Name="NodeEditor" Left="688" Top="150" Width="0" Height="660" Background="#FFFFFFFF" BorderColor="#FFD7DEE7">
          <XLabel Left="20" Top="14" Width="592" Height="34" Content="节点编辑"/>
          <XLabel Name="lblEdHint" Left="20" Top="50" Width="592" Height="22" Content="点表格一行选中设备，改负载后按「应用」回写画布"/>
          <TableView Name="tblNodes" Left="16" Top="78" Width="600" Height="280" Columns="编号:56|名称:146|上游:84|电缆:134|电压:60|负载:68" Rows="%s"/>
%s
          <Button Name="btnLoadDown" Left="20" Top="586" Width="88" Height="30" Content="-1 kW"/>
          <Button Name="btnLoadUp" Left="116" Top="586" Width="88" Height="30" Content="+1 kW"/>
          <Button Name="btnApply" Left="212" Top="586" Width="88" Height="30" Content="应用"/>
          <Button Name="btnClose" Left="308" Top="586" Width="88" Height="30" Content="关闭"/>
        </XStackPanel>
''' % (rowsattr, fl)
s=s[:m.start()]+block+s[m.end():]
io.open(p,"w",encoding="utf-8",newline="\n").write(s)
print("rewritten")