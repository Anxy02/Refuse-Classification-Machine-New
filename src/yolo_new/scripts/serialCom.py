#!/usr/bin/env python3
# coding=utf-8

import rospy
from math import *
from sensor_msgs.msg import Image
import cv2, cv_bridge
import numpy as np
from geometry_msgs.msg import Twist
from yolo_new.msg import position_color as PositionMsg
from yolo_new.msg import color_ik_result_new as color_ik_result_Msg
from std_msgs.msg import Int8
from std_msgs.msg import String as StringMsg
from yolov5_ros_msgs.msg import BoundingBox,BoundingBoxes
from yolo_new.msg import Flag,Serial_RT
import serial   #导入串口通信库
from time import sleep

ser = serial.Serial()

class Serial_COM:
	def __init__(self):
        
		self.com_sub = rospy.Subscriber("/Com_pub", Serial_RT, self.com_callback)#订阅识别类别话题
		self.flagPublisher = rospy.Publisher('/Flag_pub', Flag, queue_size=10) #发布状态 

		self.port_open_recv()					#实机测试放出来!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	def com_callback(self,msg):
		rospy.loginfo("~~~~通信回调收到: %s,%d,%d~~~~",msg.sendClass, msg.count,msg.ONum)
		if msg.close == "ok":
			self.send('1P')
			rospy.loginfo("~~~~关闭盖子~~~~: %s",msg.sendClass)
			sleep(1)
		else:
			self.send_message(msg.count,msg.sendClass,msg.ONum)

	# # 超声波投入发布flag（暂时不用）
	# def input_pubflag(self):	#超声波判断投入发布flag
	# 	ismoving = 0
	# 	isputing = 1
	# 	singleOK = 1
    # 	# count  预留，为超声波计数
	# 	flagMSG = Flag(ismoving,isputing,singleOK)
	# 	self.flagPublisher.publish(flagMSG)
	# 	isputing = 0

	def single_pubflag(self):	# 刷子完成分拣后，接收到串口消息时进行发送
		ismoving = 0
		isputing = 0
		singleOK = 1
		OverLoad = "none"
		flagMSG = Flag(ismoving,isputing,singleOK,OverLoad)
		self.flagPublisher.publish(flagMSG)

	def single_grasp_pubflag(self):	# 单目标分拣失败后，接收到串口消息X时进行发送
		ismoving = 0
		isputing = 0
		singleOK = 2
		OverLoad = "none"
		flagMSG = Flag(ismoving,isputing,singleOK,OverLoad)
		self.flagPublisher.publish(flagMSG)

	def overload_pubflag(self,sort):
		ismoving = 1
		isputing = 0
		singleOK = 1
		OverLoad = sort	# ABCD
		flagMSG = Flag(ismoving,isputing,singleOK,OverLoad)
		self.flagPublisher.publish(flagMSG)
       
	def send_message(self,bcount,bclass,bONum):
		# rospy.loginfo("send!!!!!!!!!!!!!!!!!!!!!!!!!!11")
		if bONum == 1:
			if bcount == 1:
				self.send('0A')
				rospy.loginfo("0A")
			elif bcount >1:
				self.send('1A')
				rospy.loginfo("1A")
		elif bONum == 2:
			if bcount == 1:
				self.send('0B')
				rospy.loginfo("0B")
			elif bcount >1:
				self.send('1B')
				rospy.loginfo("1B")
		elif bONum == 3:
			if bcount == 1:
				self.send('0C')
				rospy.loginfo("0C")
			elif bcount >1:
				self.send('1C')
				rospy.loginfo("1C")
		elif bONum == 4:
			if bcount == 1:
				self.send('0D')
				rospy.loginfo("0D")
			elif bcount >1:
				self.send('1D')
				rospy.loginfo("1D")
		elif bONum == 5:
			if bcount == 1:
				self.send('0E')
				rospy.loginfo("0E")
			elif bcount >1:
				self.send('1E')
				rospy.loginfo("1E")
		elif bONum == 6:
			if bcount == 1:
				self.send('0F')
				rospy.loginfo("0F")
			elif bcount >1:
				self.send('1F')
				rospy.loginfo("1F")
		elif bONum == 7:
			if bcount == 1:
				self.send('0G')
				rospy.loginfo("0G")
			elif bcount >1:
				self.send('1G')
				rospy.loginfo("1G")
		elif bONum == 8:
			if bcount == 1:
				self.send('0H')
				rospy.loginfo("0H")
			elif bcount >1:
				self.send('1H')
				rospy.loginfo("1H")
		elif bONum == 9:
			if bcount == 1:
				self.send('0I')
				rospy.loginfo("0I")
			elif bcount >1:
				self.send('1I')
				rospy.loginfo("1I")
		elif bONum == 10:
			if bcount == 1:
				self.send('0J')
				rospy.loginfo("0J")
			elif bcount >1:
				self.send('1J')
				rospy.loginfo("1J")
		elif bONum == 11:
			if bcount == 1:
				self.send('0K')
				rospy.loginfo("0K")
			elif bcount >1:
				self.send('1K')
				rospy.loginfo("1K")
		else :
			rospy.loginfo("未发送！！！")

	def port_open_recv(self):#对串口的参数进行配置
		ser.port='/dev/ttyACM0'	
		ser.baudrate=9600
		ser.bytesize=8
		ser.stopbits=1
		ser.parity="N"#奇偶校验位
		ser.open()
		if(ser.isOpen()):
			rospy.loginfo("串口打开成功！")
		else:
			rospy.loginfo("串口打开失败！")
	#isOpen()函数来查看串口的开闭状态

	def port_close(self):
		ser.close()
		if(ser.isOpen()):
			rospy.loginfo("串口关闭失败！")
		else:
			rospy.loginfo("串口关闭成功！")

	def send(self,send_data):
		if(ser.isOpen()):
			ser.write(send_data.encode())#编码  'utf-8'
			rospy.loginfo("发送成功! %s",send_data)
		else:
			rospy.loginfo("发送失败！")


rospy.init_node("Serial_COM")
rt_serial = Serial_COM()

# # 实际调试放出来！！！！！！！！！！！！！！！！！！！！！！！！
while 1:
	strRead = ser.read().decode()#"utf-8"
	if strRead:
		rospy.loginfo("串口读取到：%s",strRead)
		if strRead == 'A' or strRead == 'B' or strRead == 'C' or strRead == 'D':
			rospy.loginfo("满载返包！")
			rt_serial.overload_pubflag(strRead)
		elif strRead == 'S':
			rospy.loginfo("单目标返包！")
			rt_serial.single_pubflag()	
		elif strRead == 'X':
			rospy.loginfo("单目标抓取返包！")
			rt_serial.single_grasp_pubflag()	

rospy.spin()

# 测试用！！！！！！！！！！！！！！！！！！！！！！！！！！！！
# rospy.spin()