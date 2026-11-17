import io
p=r"G:\PyArx\examples\ui\power_plan_shell.xml"
s=io.open(p,encoding="utf-8").read()

# 1) Ribbon 编辑页加「节点编辑」组（放在撤销/重做之后）
anchor='''          <XRibbonButton Name="btnUndo" Icon="res/undo.png" Text="撤销"/>
          <XRibbonButton Name="btnRedo" Icon="res/redo.png" Text="重做"/>
        </XRibbonGroup>'''
add=anchor+'''
        <XRibbonGroup Header="节点">
          <XRibbonButton Name="btnNodeEdit" Icon="res/alignleft.png" Text="节点编辑"/>
        </XRibbonGroup>'''
assert anchor in s
s=s.replace(anchor,add,1)

# 2) 主画布宽度可直接改（侧板展开时收窄），并保持 Name
assert '<PowerCanvas Name="Plan" Width="978" Height="660"' in s

# 3) 右板之后追加节点编辑侧板
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

panel='''
        <XStackPanel Name="NodeEditor" Width="0" Height="660" Background="#FFFFFFFF" BorderColor="#FFD7DEE7">
          <XLabel Content="节点编辑" Height="34"/>
          <XLabel Name="lblEdHint" Content="点表格一行选中设备，改负载后按「应用」回写画布" Height="24"/>
          <TableView Name="tblNodes" Width="608" Height="284" Columns="编号:64|名称:150|上游:88|电缆:140|电压:64|负载:74" Rows="'''+rowsattr+'''"/>
          <XLabel Name="lblEdCode" Content="编号　—" Height="26"/>
          <XLabel Name="lblEdName" Content="名称　—" Height="26"/>
          <XLabel Name="lblEdUp" Content="上游　—" Height="26"/>
          <XLabel Name="lblEdCable" Content="电缆　—" Height="26"/>
          <XLabel Name="lblEdVolt" Content="电压　—" Height="26"/>
          <XLabel Name="lblEdLoad" Content="负载　— kW" Height="26"/>
          <XLabel Name="lblEdLen" Content="长度　— m" Height="26"/>
          <XStackPanel Orientation="Horizontal" Height="44">
            <Button Name="btnLoadDown" Content="-1 kW" Width="92" Height="32"/>
            <Button Name="btnLoadUp" Content="+1 kW" Width="92" Height="32"/>
            <Button Name="btnApply" Content="应用" Width="92" Height="32"/>
            <Button Name="btnClose" Content="关闭" Width="92" Height="32"/>
          </XStackPanel>
        </XStackPanel>
'''
tail='      </XStackPanel>\n'
i=s.rfind(tail)
assert i>0
s=s[:i]+panel+s[i:]
io.open(p,"w",encoding="utf-8",newline="\n").write(s)
print("patched")