#!/usr/bin/python
import re
import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXml import *
import ui_cc1111usbgui

class CC1111UsbGui(QMainWindow, ui_cc1111usbgui.Ui_MainWindow):

    def __init__(self,parent=None):
        super(CC1111UsbGui,self).__init__(parent)
        self.setupUi(self)

        #connect signals
        self.connect(self.continiousRecvBox,SIGNAL("clicked()"),self.handle_recvcontinious)
        self.connect(self.startRecvButton,SIGNAL("clicked()"),self.handle_recvstart)
        self.connect(self.stopRecvButton,SIGNAL("clicked()"),self.handle_recvstop)
        self.connect(self.continiousSendBox,SIGNAL("clicked()"),self.handle_sendcontinious)
        self.connect(self.startSendButton,SIGNAL("clicked()"),self.handle_sendstart)
        self.connect(self.stopSendButton,SIGNAL("clicked()"),self.handle_sendstop)
        self.connect(self.actionLoad,SIGNAL("triggered()"),self.load_configuration)
        self.connect(self.actionSave,SIGNAL("triggered()"),self.write_configuration)
    
    def load_configuration(self):
        xmlFile = QFileDialog.getOpenFileName(self,"Load configuration","/home/gerard", "*.xml *.XML")
        if xmlFile != None: 
            dom = QDomDocument()
            error = None
            fh = None
            rowCount = 0
            try:
                fh = QFile(xmlFile)
                if not fh.open(QIODevice.ReadOnly):
                    raise IOError, unicode(fh.errorString())
                if not dom.setContent(fh):
                    raise ValueError, "could not parse XML"
            except (IOError, OSError, ValueError), e:
                error = "Failed to import %s " % e
            finally:
                if fh is not None:
                    fh.close()
                if error is not None:
                    return False, error
        
            self.registerTable.clearContents()

            try:
                root = dom.documentElement()
                if root.tagName() != "registers":
                    raise ValueError, "Not a valid configuration file.\n"
                node = root.firstChild()
                while not node.isNull():
                    if node.toElement().tagName() == "register":
                        registerName = node.toElement().attribute("name")
                        registerVal = node.toElement().attribute("value")
                        self.registerTable.setItem(rowCount,0,QTableWidgetItem(registerName))
                        self.registerTable.setItem(rowCount,1,QTableWidgetItem(registerVal))
                        rowCount = rowCount + 1
                    node = node.nextSibling()
            except ValueError, e:
                return False, "Failed to import %s" % e
            self.__fname = QString()
            self.__dirty = True
            return True, "Config loaded.\n"

    def write_configuration(self):
        xmlFile = QFileDialog.getSaveFileName(self,"Save configuration","/home/gerard", "*.xml *.XML")
        if xmlFile != None: 
            error = None
            fh = None
            fh = QFile(xmlFile)
            if not fh.open(QIODevice.WriteOnly):
                raise IOError, unicode(fh.errorString())
            stream = QTextStream(fh)
            stream.setCodec("UTF-8")
            stream << ("<?xml version='1.0' encoding='UTF-8'?>\n"
                        "<registers>\n");
            for i in range(self.registerTable.rowCount()):
                stream << ("\t<register name='%s' value='%s' />\n" % (self.registerTable.item(i,0).text(),self.registerTable.item(i,1).text()))
            stream << ("</registers>\n")

    def handle_recvcontinious(self):
        if self.continiousRecvBox.isChecked():
            self.countRecvBox.setEnabled(0)
        else:
            self.countRecvBox.setEnabled(1)

    def handle_recvstart(self):
        self.startRecvButton.setEnabled(0)
        self.stopRecvButton.setEnabled(1)

    def handle_recvstop(self):
        self.stopRecvButton.setEnabled(0)
        self.startRecvButton.setEnabled(1)
    
    def handle_sendcontinious(self):
        if self.continiousSendBox.isChecked():
            self.countSendBox.setEnabled(0)
        else:
            self.countSendBox.setEnabled(1)

    def handle_sendstart(self):
        self.startSendButton.setEnabled(0)
        self.stopSendButton.setEnabled(1)

    def handle_sendstop(self):
        self.stopSendButton.setEnabled(0)
        self.startSendButton.setEnabled(1)

app = QApplication(sys.argv)
form = CC1111UsbGui()
form.show()
app.exec_()
