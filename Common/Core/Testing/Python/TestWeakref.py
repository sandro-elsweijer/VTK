"""Test if VTK-Python objects support weak referencing.

Run this test like so:
 $ vtkpython TestWeakref.py
or
 $ python TestWeakref.py

Created on July, 8 2005
Prabhu Ramachandran <prabhu_r at users dot sf dot net>

"""

import vtk
from vtk.test import Testing
import weakref


class TestWeakref(Testing.vtkTest):
    def testWeakref(self):
        o = vtk.vtkObject()
        ref = weakref.ref(o)
        self.assertEqual(ref().GetClassName(), 'vtkObject')
        del o
        self.assertEqual(ref(), None)

    def testProxy(self):
        o = vtk.vtkObject()
        proxy = weakref.proxy(o)
        self.assertEqual(proxy.GetClassName(), 'vtkObject')
        del o
        self.assertRaises(ReferenceError, getattr,
                          proxy, 'GetClassName')


if __name__ == "__main__":
    Testing.main([(TestWeakref, 'test')])
