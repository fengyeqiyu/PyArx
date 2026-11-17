import io,re
p=r"G:\PyArx\examples\ui\power_plan_shell.xml"
s=io.open(p,encoding="utf-8").read()
m=re.search(r'\n        <XStackPanel Name="NodeEditor".*?\n        </XStackPanel>\n', s, re.S)
assert m, "panel block not found"
block=m.group(0)
s=s[:m.start()]+"\n"+s[m.end():]
# 改绝对定位（Left/Top 显式 = 不参与父容器流；盖在右板上，画布同时收窄）
block=block.replace('<XStackPanel Name="NodeEditor" Width="0" Height="660"',
                    '<XStackPanel Name="NodeEditor" Left="688" Top="150" Width="0" Height="660"')
anchor="</XWindow>"
i=s.rindex(anchor)
s=s[:i]+block+"\n"+s[i:]
io.open(p,"w",encoding="utf-8",newline="\n").write(s)
print(block.strip().split("\n")[0])