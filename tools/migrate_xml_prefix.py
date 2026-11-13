# -*- coding: utf-8 -*-
"""把"遍历所有 XML 文件/字符串"里的标签换成统一 X 前缀写法。

规则（只改元素标签，不改属性名、不改资源结构标签）：
    <XButton ...>  -> <XButton ...>
    </XButton>     -> </XButton>
    <button ...>  -> <XButton ...>   （大小写规范到本意名）
主名表取自 UIDesign/control_catalog.xml + 容器名表，避免误伤 <Style> 之类资源标签。
"""
import io
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CATALOG = os.path.join(ROOT, 'UIDesign', 'control_catalog.xml')

# 元素标签（本意名） —— 只这些加前缀
ELEMENTS = set()
# 容器（含根），这些也需要 X 前缀，保持"所有类都带前缀"一致
CONTAINERS = {
    'Window', 'UserControl', 'Layout', 'LayoutPanel', 'AbsolutePanel', 'AbsPanel',
    'CanvasPanel', 'CanvasLayout', 'Canvas', 'Grid', 'StackPanel', 'DockPanel',
    'WrapPanel', 'Frame', 'TabPage', 'ScrollViewer', 'ContentControl',
    'PaletteSet', 'PalettePage', 'PanelHeader', 'TabStrip', 'ChipBar', 'Stage',
}
# 不加前缀（声明层结构，不是控件类）
SKIP = {
    'Style', 'Setter', 'ControlTemplate', 'DataTemplate', 'Binding', 'Trigger',
    'DataTrigger', 'Resources', 'RowDefinition', 'ColumnDefinition',
    'ResourceDictionary', 'MergedDictionaries', 'PaletteSet.Resources',
    'Window.Resources', 'UserControl.Resources', 'Grid.Resources',
    'Grid.RowDefinitions', 'Grid.ColumnDefinitions', 'Style.Triggers',
    'TabItem', 'Tab',
}

ALIAS_TO_MAIN = {
    'Label': 'Label', 'Edit': 'TextField', 'Progress': 'ProgressBar',
    'TrackBar': 'Slider', 'TreeView': 'TreeView', 'ListView': 'ListCtrl',
    'Checkbox': 'CheckBox', 'Radio': 'RadioButton', 'Group': 'GroupBox',
    'Combo': 'ComboBox', 'DataGrid': 'ListCtrl', 'Separator': 'Divider',
    'PlanCanvas': 'PowerCanvas', 'Knob': 'Dial', 'Gauge': 'Dial',
    'Spinner': 'BusyIndicator', 'ActivityIndicator': 'BusyIndicator',
    'NumberBox': 'SpinBox', 'NumericUpDown': 'SpinBox',
    'TileView': 'GridView', 'CoverFlow': 'PathView', 'PagerDots': 'PageIndicator',
    'Wheel': 'Tumbler', 'DataTable': 'TableView', 'GridTable': 'TableView',
    'HierarchicalTable': 'TreeTable', 'ToolButtonBar': 'ToolBar',
    'ContextMenu': 'Menu', 'PopupMenu': 'Menu', 'Hint': 'ToolTip',
    'Overlay': 'Popup', 'PageView': 'StackView', 'CarouselView': 'StackView',
    'PageSwipe': 'SwipeView', 'DrawCanvas': 'QCanvas', 'PaintCanvas': 'QCanvas',
    'CanvasSurface': 'QCanvas', 'Painter': 'QCanvas',
    'VScrollBar': 'ScrollBar', 'HScrollBar': 'ScrollBar', 'ScrollView': 'ScrollView',
    'FlickArea': 'Flickable', 'ToggleSwitch': 'Switch', 'SliderRange': 'RangeSlider',
    'DatePickerCtrl': 'DatePicker', 'SplitterWindow': 'SplitView',
    'RichEdit': 'TextArea', 'MaskedTextBox': 'TextField', 'LinkLabel': 'Hyperlink',
    'ToggleBtn': 'ToggleButton', 'BitmapBtn': 'IconButton',
    'TabWidget': 'TabBar', 'MenuBarControl': 'MenuBar',
    'TopMenuBar': 'MenuBar', 'MessageBoxX': 'Popup', 'SideSheet': 'Popup',
}


def load_catalog_tags():
    with io.open(CATALOG, encoding='utf-8') as f:
        for m in re.finditer(r'<objectinfo\s+tag="([^"]+)"', f.read()):
            ELEMENTS.add(m.group(1))


def main(argv):
    load_catalog_tags()
    targets = []
    for p in argv[1:] or ['.']:
        if os.path.isfile(p):
            targets.append(p)
            continue
        for dirpath, dirnames, filenames in os.walk(p):
            dirnames[:] = [d for d in dirnames if d not in ('.git', 'X64', 'log')]
            for fn in filenames:
                if fn.lower().endswith(('.xml', '.py', '.xml', '.md')):
                    targets.append(os.path.join(dirpath, fn))

    changed_files = 0
    changed_tags = 0
    for path in targets:
        rel = os.path.relpath(path, ROOT).replace('\\', '/')
        if rel.startswith('docs/_') or '/X64/' in rel:
            continue
        try:
            text = io.open(path, encoding='utf-8').read()
        except (UnicodeDecodeError, OSError):
            continue
        orig = text

        def repl(m):
            slash, name = m.group(1), m.group(2)
            if name in SKIP or (name.startswith('X') and name[1:2].isupper()):
                return m.group(0)
            main_name = ALIAS_TO_MAIN.get(name, name)
            if main_name not in ELEMENTS and main_name not in CONTAINERS:
                return m.group(0)
            return '<%sX%s' % (slash, main_name)

        text = re.sub(r'<(/?)([A-Za-z][A-Za-z0-9_]*)', repl, text)
        # 目录/文档里的 tag="XButton" 也统一
        text = re.sub(r'tag="([A-Za-z][A-Za-z0-9_]*)"', lambda m: 'tag="X%s"' % ALIAS_TO_MAIN.get(
            m.group(1), m.group(1)) if (ALIAS_TO_MAIN.get(m.group(1), m.group(1)) in ELEMENTS) else m.group(0), text)

        if text != orig:
            io.open(path, 'w', encoding='utf-8', newline='\n').write(text)
            changed_files += 1
            changed_tags += len(re.findall(r'<X[A-Z]', text)) - len(re.findall(r'<X[A-Z]', orig))
    print('改写文件 %d 个，新增前缀标签 %d 处' % (changed_files, changed_tags))


if __name__ == '__main__':
    main(sys.argv)
