import io
P = r'G:\PyArx\tests\mfc_xaml\pp_real_test.py'
s = io.open(P, encoding='utf-8').read()
s = s.replace("""    send(MOUSEEVENTF_LEFTUP)
    time.sleep(0.25)""",
"""    send(MOUSEEVENTF_LEFTUP)
    time.sleep(0.05)
    M.pump(120)          # 真实输入也要宿主消息循环分发，脚本必须 pump
    time.sleep(0.10)""")
s = s.replace("""        send(MOUSEEVENTF_WHEEL, 120 if delta > 0 else 0xFFFFFF88)
        time.sleep(0.06)
    time.sleep(0.25)""",
"""        send(MOUSEEVENTF_WHEEL, 120 if delta > 0 else 0xFFFFFF88)
        time.sleep(0.05)
        M.pump(60)
    time.sleep(0.10)""")
io.open(P,'w',encoding='utf-8').write(s)
print('real 测试加 pump')
