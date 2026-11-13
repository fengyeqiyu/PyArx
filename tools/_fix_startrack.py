import io
p = 'G:/PyArx/MfcXamlCore/XamlCore.cpp'
s = io.open(p, encoding='utf-8-sig').read()
old = """        std::vector<int> result;
        for (const Track& t : tracks)
            result.push_back(t.kind == 2 ? (std::max)(1, unit * (std::max)(1, t.value)) : t.value);
        return result;"""
new = """        std::vector<int> result;
        int lastStar = -1;
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            const Track& t = tracks[i];
            result.push_back(t.kind == 2 ? (std::max)(1, unit * (std::max)(1, t.value)) : t.value);
            if (t.kind == 2)
                lastStar = static_cast<int>(i);
        }
        // 取整会丢掉余数：640 宽 12 + 3*star + 12 会算成 639。
        // 余数补给最后一个 star 轨道，保证各轨道之和正好等于可用宽度。
        if (lastStar >= 0)
        {
            int sum = 0;
            for (int v : result)
                sum += v;
            if (sum > 0 && sum < available)
                result[lastStar] += (available - sum);
        }
        return result;"""
assert s.count(old) == 1
s = s.replace(old, new)
io.open(p, 'w', encoding='utf-8-sig', newline='').write(s)

p = 'G:/PyArx/tests/mfc_xaml/pp_runtime_test.py'
s = io.open(p, encoding='utf-8').read()
s = s.replace("c.Table.rect == (10, 50, 639, 306)", "c.Table.rect == (10, 50, 640, 306)")
io.open(p, 'w', encoding='utf-8', newline='').write(s)

p = 'G:/PyArx/tests/mfc_xaml/pp_plugin_test.py'
s = io.open(p, encoding='utf-8').read()
s = s.replace("c.Table.rect[3] == 306, str(c.Table.rect)", "c.Table.rect[2] == 640 and c.Table.rect[3] == 306, str(c.Table.rect)")
io.open(p, 'w', encoding='utf-8', newline='').write(s)
print('ResolveTracks 余数补齐 + 用例期望 640')
