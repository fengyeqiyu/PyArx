# -*- coding: utf-8 -*-
r"""MfcXmlRuntime：复合控件的运行机制框架。

    from MfcXmlRuntime import xmlload, Runtime

    c = xmlload(r'G:\PyArx\plugins\side_panel.xml', app=app, x=10, y=10, w=640, h=380)
    c.BtnApply.click = myclass.on_click
    c.Table.grid = myclass.on_grid
"""
from .runtime import (Composite, Child, Runtime, available, import_plugin,
                      load, xmlload, PLUGIN_DIR)
from .html import html, Ui, DEFAULT_HEIGHT

__all__ = ['Composite', 'Child', 'Runtime', 'available', 'import_plugin',
           'load', 'xmlload', 'PLUGIN_DIR', 'html', 'Ui', 'DEFAULT_HEIGHT']
