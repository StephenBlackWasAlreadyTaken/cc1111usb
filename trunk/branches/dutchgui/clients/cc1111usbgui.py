#!/usr/bin/python
import re
import sys
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtXml import *
import ui_cc1111usbgui
from cc1111client import *

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
        self.connect(self.plainButton,SIGNAL("clicked()"),self.handle_sendhex)
        self.connect(self.hexButton,SIGNAL("clicked()"),self.handle_sendhex)
    
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
        d = USBDongle()
        print "stepone"
        d.setModeIDLE()
        print "steptwo"
        for i in range(self.registerTable.rowCount()):
			if(self.registerTable.item(i,0).text() == "IOCFG2"):
				d.poke(IOCFG2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "IOCFG1"):
				d.poke(IOCFG1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "IOCFG0"):
				d.poke(IOCFG0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "SYNC1"):
				d.poke(SYNC1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "SYNC0"):
				d.poke(SYNC0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "PKTLEN"):
				d.poke(PKTLEN,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "PKTCTRL1"):
				d.poke(PKTCTRL1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "PKTCTRL0"):
				d.poke(PKTCTRL0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "ADDR"):
				d.poke(ADDR,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "CHANNR"):
				d.poke(CHANNR,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCTRL1"):
				d.poke(FSCTRL1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCTRL0"):
				d.poke(FSCTRL0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FREQ2"):
				d.poke(FREQ2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FREQ1"):
				d.poke(FREQ1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FREQ0"):
				d.poke(FREQ0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MDMCFG4"):
				d.poke(MDMCFG4,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MDMCFG3"):
				d.poke(MDMCFG3,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MDMCFG2"):
				d.poke(MDMCFG2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MDMCFG1"):
				d.poke(MDMCFG1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MDMCFG0"):
				d.poke(MDMCFG0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "DEVIATN"):
				d.poke(DEVIATN,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MCSM2"):
				d.poke(MCSM2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MCSM1"):
				d.poke(MCSM1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "MCSM0"):
				d.poke(MCSM0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FOCCFG"):
				d.poke(FOCCFG,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "BSCFG"):
				d.poke(BSCFG,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "AGCCTRL2"):
				d.poke(AGCCTRL2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "AGCCTRL1"):
				d.poke(AGCCTRL1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "AGCCTRL0"):
				d.poke(AGCCTRL0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FREND1"):
				d.poke(FREND1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FREND0"):
				d.poke(FREND0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCAL3"):
				d.poke(FSCAL3,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCAL2"):
				d.poke(FSCAL2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCAL1"):
				d.poke(FSCAL1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "FSCAL0"):
				d.poke(FSCAL0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "TEST2"):
				d.poke(TEST2,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "TEST1"):
				d.poke(TEST1,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "TEST0"):
				d.poke(TEST0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
			elif(self.registerTable.item(i,0).text() == "PA_TABLE0"):
				d.poke(PA_TABLE0,"%c"%chr(int(str(self.registerTable.item(i,1).text()),0)))
        print "stepthree"
        d.setModeTX();
        print "stepfour"
        sendData = chr(0x0b)
        sendData += "hallocc1111"
        d.send(APP_SYSTEM,CMD_RFMODE,"%c%s" % (len(data)+1, data))
        print "stepfive"

    def handle_sendstop(self):
        self.stopSendButton.setEnabled(0)
        self.startSendButton.setEnabled(1)

    def handle_sendhex(self):
        if self.hexButton.isChecked():
            self.connect(self.dataText,SIGNAL("textChanged()"),self.handle_checkhex)
        else:
            self.disconnect(self.dataText,SIGNAL("textChanged()"),self.handle_checkhex)

    def handle_checkhex(self):
        lastChar = self.dataText[self.dataText.length()]
        print lastChar
    
app = QApplication(sys.argv)
form = CC1111UsbGui()
form.show()
app.exec_()
