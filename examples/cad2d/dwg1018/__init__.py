# -*- coding: utf-8 -*-
"""R2004 (AC1018) DWG 纯 Python 容器层解码。

参考：ODA Open Design Specification for .dwg files v5.1，第 4 章。
仅做容器（文件头 / 系统页 / 页地图 / 节地图 / 数据页定位）与解压，
对象语义由上层模块映射。
"""
