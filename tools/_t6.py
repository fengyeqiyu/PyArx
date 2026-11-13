import io
p='G:/PyArx/PyMfcXaml/Table.cpp'
c=io.open(p,encoding='utf-8-sig').read()
old='''        // 表头区域：点漏斗图标 = 输入筛选；双击表头 = 弹出菜单
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int col = self->ColAtX(GET_X_LPARAM(lp));
            if (col >= 0)
            {
                int colRight = 0;
                for (int i = 0; i <= col; ++i) colRight += self->m_colW[i];
                const bool onFunnel = GET_X_LPARAM(lp) >= colRight - 20;
                if (msg == WM_LBUTTONDBLCLK)      self->RunHeaderMenu(col);
                else if (onFunnel)                self->RunHeaderFilter(col);
            }
            return 0;
        }'''
new='''        // 表头区域：点漏斗图标 = 输入筛选；双击表头也进筛选输入（菜单改到右键）
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int col = self->ColAtX(GET_X_LPARAM(lp));
            if (col >= 0)
            {
                int colRight = 0;
                for (int i = 0; i <= col; ++i) colRight += self->m_colW[i];
                const bool onFunnel = GET_X_LPARAM(lp) >= colRight - 20;
                if (msg == WM_LBUTTONDBLCLK || onFunnel) self->RunHeaderFilter(col);
            }
            return 0;
        }'''
assert c.count(old)==1
c=c.replace(old,new,1)
old2='''    case WM_KEYDOWN:'''
new2='''    case WM_RBUTTONDOWN:
    {
        // 右键表头：弹出菜单（升序 / 降序 / 清除该列筛选 / 清除全部筛选）
        if (GET_Y_LPARAM(lp) < self->m_headerH)
        {
            const int col = self->ColAtX(GET_X_LPARAM(lp));
            if (col >= 0) self->RunHeaderMenu(col);
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:'''
if 'case WM_RBUTTONDOWN:' not in c:
    c=c.replace(old2,new2,1)
# 更新注释
c=c.replace('// 双击表头：弹出菜单（升序 / 降序 / 清本列筛选 / 清全部筛选）',
            '// 右键表头：弹出菜单（升序 / 降序 / 清本列筛选 / 清全部筛选）')
io.open(p,'w',encoding='utf-8-sig',newline='').write(c)
print('菜单已改到右键')
