# -*- coding: utf-8 -*-
"""MfcXamlRuntime：复合控件的运行机制框架。

    from MfcXamlRuntime import xamlload, Runtime

    c = xamlload(r'G:\PyArx\plugins\side_panel.xaml', app=app, x=10, y=10, w=640, h=380)
    c.BtnApply.click = myclass.on_click
    c.Table.grid = myclass.on_grid
"""
from .runtime import (Composite, Child, Runtime, available, import_plugin,
                      load, xamlload, PLUGIN_DIR)

__all__ = ['Composite', 'Child', 'Runtime', 'available', 'import_plugin',
           'load', 'xamlload', 'PLUGIN_DIR']
