import io
P = r'G:\PyArx\PyMfcXaml\PyMfcXaml.cpp'
t = io.open(P, encoding='utf-8-sig').read()

start = t.index("    // ---- 可编辑表格（SysListView32）的键盘/排序/复制粘贴支持 ----")
end_mark = """        catch (const py::error_already_set& e)
        {
            debugLogf(L"[grid] 回调异常: %hs", e.what());
        }
    }"""
end = t.index(end_mark, start) + len(end_mark)
block = t[start:end]
t = t[:start] + t[end:]

anchor = "    void setWheelRegion(int x, int y, int w, int h, int step)\n    {"
assert t.count(anchor) == 1
t = t.replace(anchor, block.rstrip('\n') + "\n\n" + anchor, 1)

# 清理留在 private 区的一处多余空行；并确保私有空对象还有内容
io.open(P, 'w', encoding='utf-8-sig', newline='').write(t)
print('成员移到 public 完成')
