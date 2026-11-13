# -*- coding: utf-8 -*-
"""兼容转发：真正的运行机制在 G:\PyArx\MfcXmlRuntime。"""
import os
import sys

_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                      if False else os.path.abspath(__file__))
for _ in range(2):
    _ROOT = os.path.dirname(_ROOT)
if _ROOT not in sys.path:
    sys.path.insert(0, _ROOT)

from MfcXmlRuntime import *          # noqa: F401,F403
from MfcXmlRuntime import Composite, Child, Runtime, xmlload, load, import_plugin, available  # noqa: F401
