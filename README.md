# Refuse-Classification-Machine
垃圾分类机器人——垃圾识别（YOLOv5）与机械臂分类（moveit!）程序

## 环境配置
- sudo安装moveit,ros-noetic-usb-cam
sudo apt-get install ros-noetic-moveit
- 安装Kinematics trac_ik求解器
sudo apt install ros-noetic-trac-ik-kinematics-plugin

- 修改串口绑定
cd /etc/udev/rules.d
修改串口绑定https://blog.csdn.net/qq_43326927/article/details/128680244
KERNEL=="ttyUSB*", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60",ATTRS{serial}=="0002", MODE:="0777", GROUP:="dialout", SYMLINK+="wheeltec_controller"

- 串口包安装
pip3 install pyserial
sudo apt-get install ros-noetic-serial

- 串口开权限
ls /dev/tty*
sudo chmod 777 /dev/ttyACM0
永久开权限
ls -l /dev/ttyACM0
whoami
sudo usermod -aG dialout USERNAME

- 关闭相机显示
/opt/ros/noetic/share/usb_cam/launch
将image_view注释掉

- 报错
warnings.warn('User provided device_type of \'cuda\', but CUDA is not available. Disabling')
解决：注释掉

- pip报错SOCKS 
unset all_proxy && unset ALL_PROXY
切换ZJU源

- 开机自启动
https://blog.csdn.net/qq_44339029/article/details/131220282
https://blog.csdn.net/dui845819593/article/details/128613251
https://blog.csdn.net/qq_35395195/article/details/129973797
rc-local或者`gnome-session-properties`

## 执行方式
1. 拉取最新版本代码
进入主文件夹-进入Refuse-Classification-Machine-New文件夹-右键打开终端-输入
git pull
2. 编译代码
在Refuse-Classification-Machine-New文件夹-右键打开终端-输入
catkin_make
3. 执行代码
在Refuse-Classification-Machine-New文件夹-右键打开终端-输入
roslaunch yolo_new yolo.launch 
4. 其他操作
关闭程序：在终端中，Ctrl+C键同时按下


## DEBUG
vedio.py
serialCom.py: main, init

