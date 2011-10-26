#!/usr/bin/python
import re
import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import ui_cc1111usbgui

class CC1111UsbGui(QMainWindow, ui_cc1111usbgui.Ui_MainWindow):

	def __init__(self,parent=None):
		super(CC1111UsbGui,self).__init__(parent)
		self.setupUi(self)

		#connect signals
		self.connect(self.continiousBox,SIGNAL("clicked()"),self.handle_recvcontinious)
		self.connect(self.startButton,SIGNAL("clicked()"),self.handle_recvstart)
		self.connect(self.stopButton,SIGNAL("clicked()"),self.handle_recvstop)

	def handle_recvcontinious(self):
		if self.continiousBox.isChecked():
			self.countBox.setEnabled(0)
		else:
			self.countBox.setEnabled(1)

	def handle_recvstart(self):
		self.startButton.setEnabled(0)
		self.stopButton.setEnabled(1)

	def handle_recvstop(self):
		self.stopButton.setEnabled(0)
		self.startButton.setEnabled(1)

app = QApplication(sys.argv)
form = CC1111UsbGui()
form.show()
app.exec_() 
