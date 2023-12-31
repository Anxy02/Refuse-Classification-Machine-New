#include <ros/ros.h>
#include <iostream>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <stdbool.h>
#include <locale>
//using namespace std; 

#include <std_msgs/Float32.h>
#include <std_msgs/String.h>
#include <moveit/move_group_interface/move_group_interface.h>
#include "yolo_execute.h"
#include "yolo_new/Flag.h"
#include "yolo_new/Serial_RT.h"

#include<locale>

int yolo_sequence=1;  //夹取顺序
int yolo_confirmed_flag =0; //启动夹取的标志位
int grasp_mode=0;//抓取模式 0：单目标抓取  1：单目标刷子  2：多目标抓取
int isBusy=0,isSingle=0,countFlag=0;//回调函数中忙状态
int i_cb=0,j_cb=0,tmp_i=0;//回调函数循环变量

float auxiliary_angle; 
int recycle_count_cb=0,harm_count_cb=0,kitchen_count_cb=0,others_count_cb=0; //接收到该数据帧的回调函数次数callback
int recycle_count=0,harm_count=0,kitchen_count=0,others_count=0; //垃圾计数
int count=0;//识别到的垃圾总数量
int ObjectNum[10]={0};//识别到的垃圾号码
int grasp_done=0;  //色块是否已经夹取标志位
float cb_target_data[10][3]={0};  //回调函数目标值保存
std::string cb_class[10]={};
std::string single_class={};
float joint_target1=0,joint_target2=0,joint_target3=0; //赋值给moveit做正解的目标关节值
float link_a,link_b,link_c,link_h; //机械参数
float base_angle;
std::string arm_state="none",target_sort="noon";  //机械臂状态，夹取目标色块的颜色

bool arm_success,hand_close_success,hand_open_success; //moveit正解计算（返回值）是否成功的标志位

std::vector<double> joint_group_positions(5); //机械臂正解的目标关节位置的数组



//目标色块对应逆解的目标关节角度回调函数
void color_ik_result_callback(const yolo_new::color_ik_result_new &msg)
{
  // 单分类
  if(isBusy == 0 && isSingle == 0 && msg.count == 1){
  singleSort:
    tmp_i = 0;
    ROS_INFO("!!!!!!!!!!!single object :%s !!!!!!!!!!!!",msg.sort.c_str()); 
    count = 1;
    ObjectNum[0] = msg.ONum;
    single_class = msg.sort.c_str();
    isBusy = 1;
    isSingle = 1;
  }
  // 单分类抓取
  if(isBusy == 0 && msg.Single == 1 && msg.count == 1){
    tmp_i = 0;
    ROS_INFO("!!!!!!!!!!!single object :%s !!!!!!!!!!!!",msg.sort.c_str()); 
    count = 1;
    ObjectNum[0] = msg.ONum;
    cb_class[0] = msg.sort; 
    // single_class = msg.sort.c_str();

    cb_target_data[0][0]=msg.pedestal_angle;  //云台的目标角度
    cb_target_data[0][1]=msg.arm_angle;       //控制机械臂臂长的目标角度
    cb_target_data[0][2]=msg.hand_angle;      //控制夹取色块旋转的目标角度
    ROS_INFO("tmp_i is :%d ",i_cb); 
    ROS_INFO("cb_target_is  :(%4.2f)-(%4.2f)-(%4.2f)",cb_target_data[i_cb][0],cb_target_data[i_cb][1],cb_target_data[i_cb][2]);
    ROS_INFO("~~~~~~~~~~~~~~~~~~cpp ONum is :%d ~~~~~~~~~~~~~~~~~~~~~",ObjectNum[i_cb]); 

    isBusy = 1;
    isSingle = 0;
  }
  // 多分类
  if(isBusy == 0 && msg.count > 1)
  {
      isSingle = 0;//强制赋值
      //判断只要不是瓶子、罐子、石头就抓取  
      if(msg.ONum == 1 || msg.ONum == 2 || msg.ONum == 6)
      {
        tmp_i += 1;
        if (tmp_i == 10)
          {
            if(msg.ONum != 1 && msg.ONum != 2 /*&& msg.ONum != 7*/) // 非瓶子罐子進行抓取 药品开关
            {
              goto multiSort;
            }
            else  // 瓶子罐子進行單目標抓取
            {
              goto singleSort;
            }
          }
        else
          return;
      }

    multiSort:        
      tmp_i = 0;
      if(countFlag == 0){
        count=msg.count;//总数量
        countFlag = 1;
      }
      
        cb_target_data[i_cb][0]=msg.pedestal_angle;  //云台的目标角度
        cb_target_data[i_cb][1]=msg.arm_angle;       //控制机械臂臂长的目标角度
        cb_target_data[i_cb][2]=msg.hand_angle;      //控制夹取色块旋转的目标角度
        ROS_INFO("tmp_i is :%d ",i_cb); 
        ROS_INFO("cb_target_is  :(%4.2f)-(%4.2f)-(%4.2f)",cb_target_data[i_cb][0],cb_target_data[i_cb][1],cb_target_data[i_cb][2]);
        cb_class[i_cb] = msg.sort; 
        ObjectNum[i_cb] = msg.ONum;
        ROS_INFO("~~~~~~~~~~~~~~~~~~cpp ONum is :%d ~~~~~~~~~~~~~~~~~~~~~",ObjectNum[i_cb]); 
        
        isBusy = 1;
  }
  
}


int main(int argc, char **argv)
{ 
    ros::init(argc, argv, "yolo_execute");
    ros::NodeHandle n;
    ros::NodeHandle nprivate("~");
    setlocale(LC_ALL,"");

    std::locale::global(std::locale(""));
    nprivate.param<float>("/auxiliary_angle", auxiliary_angle, 0.157);
    nprivate.param<float>("/link_a", link_a, 0.105);
    nprivate.param<float>("/link_b", link_b, 0.100);
    nprivate.param<float>("/link_c", link_c, 0.175);
    nprivate.param<float>("/link_h", link_h, 0.080);//0.105
    i_cb=0;j_cb=0;  //初始化i,j

    base_angle=acos((link_c-link_h)/link_a);  //计算机械臂夹爪可触底的关节基础角度

    ros::AsyncSpinner spinner(1);
    spinner.start();
   
    moveit::planning_interface::MoveGroupInterface arm("arm");  
    moveit::planning_interface::MoveGroupInterface hand("hand"); 
    moveit::planning_interface::MoveGroupInterface::Plan my_plan; 

    arm.setNamedTarget("arm_look"); arm.move(); //sleep(1);    //机械臂运动到观测色块的位置
    hand.setNamedTarget("hand_open"); hand.move(); //sleep(1);  //机械爪张开
    ros::Subscriber color_ik_result_sub=n.subscribe("color_ik_result_new",10,color_ik_result_callback); //订阅色块目标位置对应的关节角度信息
    ros::Publisher Flag_pub = n.advertise<yolo_new::Flag>("Flag_pub",1000);
    ros::Publisher Com_pub = n.advertise<yolo_new::Serial_RT>("Com_pub",1000);  //发布串口通信消息  cpp -> py、arduino
    yolo_new::Flag pub_flag;
    yolo_new::Serial_RT pub_com;

    pub_flag.isMoving = 0;//初始化
    pub_flag.isPuting = 0;
    pub_flag.singleSortOK = 1;
    pub_flag.OverLoad = "none";
    pub_com.count = 0;
    pub_com.ONum = 0;
    pub_com.sendClass = "none";
    pub_com.close = "none";
    Flag_pub.publish(pub_flag);
    Com_pub.publish(pub_com);
    
    //*****************************添加避障障碍物*******************************
      ros::Publisher planning_scene_diff_publisher = n.advertise<moveit_msgs::PlanningScene>("planning_scene", 1);
      ros::WallDuration sleep_t(0.5);
      while (planning_scene_diff_publisher.getNumSubscribers() < 1)
      {
        sleep_t.sleep();
      }
      moveit_msgs::CollisionObject obj;

      //顶板
      obj.header.frame_id = "base_link";
      obj.id="box";
      //定义物体形状尺寸
      shape_msgs::SolidPrimitive primitive;
      primitive.type=primitive.BOX;
      primitive.dimensions.resize(3); //dimensions是一个vector,为其分配3个元素空间
      primitive.dimensions[0] =0.5;   //x轴上长度
      primitive.dimensions[1] =0.5;   //y轴上长度
      primitive.dimensions[2] =0.01;   //z轴上长度
      //定义物体方位
      geometry_msgs::Pose pose;
      pose.position.x = 0.1;
      pose.position.z = 0.42;
      //将形状添加到obj
      obj.primitives.push_back(primitive);
      obj.primitive_poses.push_back(pose);
      //定义操作为添加
      obj.operation = obj.ADD;
      //定义一个PlanningScene消息
      moveit_msgs::PlanningScene planning_scene;
      planning_scene.world.collision_objects.push_back(obj);
      planning_scene.is_diff = true;
      //发布该消息
      planning_scene_diff_publisher.publish(planning_scene);

      // //右边刷子
      // obj.header.frame_id = "base_link";
      // obj.id="box1";
      // //定义物体形状尺寸
      // shape_msgs::SolidPrimitive primitive1;
      // primitive1.type=primitive1.BOX;
      // primitive1.dimensions.resize(3); //dimensions是一个vector,为其分配3个元素空间
      // primitive1.dimensions[0] =0.02;   //x轴上长度
      // primitive1.dimensions[1] =0.22;   //y轴上长度
      // primitive1.dimensions[2] =0.07;   //z轴上长度
      // //定义物体方位
      // // geometry_msgs::Pose pose;
      // pose.position.x = 0.26;//0.25
      // pose.position.y = -0.08;//-0.11
      // pose.position.z = 0.03;//0.05
      // pose.orientation.w = 0.924;
      // pose.orientation.z = 0.383;
      // //将形状添加到obj
      // obj.primitives.push_back(primitive1);
      // obj.primitive_poses.push_back(pose);
      // //定义操作为添加
      // obj.operation = obj.ADD;
      // //定义一个PlanningScene消息
      // // moveit_msgs::PlanningScene planning_scene;
      // planning_scene.world.collision_objects.push_back(obj);
      // planning_scene.is_diff = true;
      // //发布该消息
      // planning_scene_diff_publisher.publish(planning_scene);

      // //左刷子1
      // obj.header.frame_id = "base_link";
      // obj.id="box2";
      // //定义物体形状尺寸
      // shape_msgs::SolidPrimitive primitive2;
      // primitive2.type=primitive2.BOX;
      // primitive2.dimensions.resize(3); //dimensions是一个vector,为其分配3个元素空间
      // primitive2.dimensions[0] =0.02;   //x轴上长度
      // primitive2.dimensions[1] =0.22;   //y轴上长度
      // primitive2.dimensions[2] =0.07;   //z轴上长度
      // //定义物体方位
      // // geometry_msgs::Pose pose;
      // pose.position.x = 0.26;//0.25
      // pose.position.y = 0.08;//0.11
      // pose.position.z = 0.03;//0.05
      // pose.orientation.w = 0.924;
      // pose.orientation.z = -0.383;
      // //将形状添加到obj
      // obj.primitives.push_back(primitive2);
      // obj.primitive_poses.push_back(pose);
      // //定义操作为添加
      // obj.operation = obj.ADD;
      // //定义一个PlanningScene消息
      // // moveit_msgs::PlanningScene planning_scene;
      // planning_scene.world.collision_objects.push_back(obj);
      // planning_scene.is_diff = true;
      // //发布该消息
      // planning_scene_diff_publisher.publish(planning_scene);
      
    //***********************************************************
    
    ROS_INFO("yolo_execute_node init successful");
    ROS_INFO("finding and waitting....");

    while(ros::ok())
   {
      multi_grasp_sequence();//多目标抓取顺序判断函数
      pub_com.count = count;//传入识别到的垃圾数量

      //发布moving FLAG消息-->由py接收
      if(isBusy){
        pub_flag.isMoving = isBusy;

        //投入口
        pub_com.close = "ok";
        Com_pub.publish(pub_com);
        pub_com.close = "none";

        if (isSingle == 1 && count == 1){ //单分类
          // ROS_INFO("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
          pub_com.count = 1;
          pub_com.sendClass = single_class;
          pub_com.ONum = ObjectNum[0];
          Com_pub.publish(pub_com);

          pub_com.count = 0;
          pub_com.sendClass = "none";
          pub_com.ONum = 999;
          
          pub_flag.singleSortOK = 0;
          Flag_pub.publish(pub_flag);
          pub_flag.singleSortOK = 1;  //防止不会再次进入单分类模式
          isBusy = 0;
          isSingle = 0;
          count = 0;
          ObjectNum[0]=0; //清零测试
        }
        else
          Flag_pub.publish(pub_flag);
      }

      if( arm_state=="ready" )
      {
        arm_state="working";
        // ROS_INFO("joint_target_is    :(%4.2f)-(%4.2f)-(%4.2f)",joint_target1,joint_target2,joint_target3);
        // pub_com.sendClass = target_sort;//发布垃圾类别信息至通信py
        // pub_com.ONum = ObjectNum[j_cb];
        // Com_pub.publish(pub_com);
        // pub_com.sendClass = "none";
        //关节的目标旋转角度赋值
        joint_group_positions[0] =  joint_target1;
        joint_group_positions[1] =  -1.57-joint_target2+base_angle;
        joint_group_positions[2] =  joint_target2-base_angle;
        joint_group_positions[3] =  -1.57+joint_target2+auxiliary_angle;
        joint_group_positions[4] =  joint_target3;
        arm.setJointValueTarget(joint_group_positions); //输入目标关节数组

        arm_success = ((arm.plan(my_plan) == moveit::planning_interface::MoveItErrorCode::SUCCESS)); //规划路径
        arm.execute(my_plan); //如果规划成功则执行
        // sleep(1);
        hand.setNamedTarget("hand_close");  //机械爪夹取
        while( !hand_close_success ) //判断是否规划成功，如果不成功则继续规划
        { 
           hand_close_success = ((hand.move() == moveit::planning_interface::MoveItErrorCode::SUCCESS)); //规划路径
          //  sleep(1);
        }
        hand_close_success=false;
 
        arm.setNamedTarget("arm_look");   arm.move();  //sleep(1);    //机械臂运动到观测色块的位置
        // arm.setNamedTarget("color_put_interval");  arm.move();  sleep(1); //机械臂臂身运动到放置色块的预位置后，再放置色块
        arm_put(target_sort); //根据颜色将色块放置到对应位置 

        pub_com.sendClass = target_sort;//发布垃圾类别信息至通信py
        pub_com.ONum = ObjectNum[j_cb];
        Com_pub.publish(pub_com);
        pub_com.sendClass = "none";

        grasp_done=0;  //标志位清零
        // if(j_cb >= count)//标志位清零
        // {
          i_cb = 0,j_cb = 0;
          isBusy = 0 ;
          countFlag = 0;
          pub_flag.isMoving = isBusy;
          Flag_pub.publish(pub_flag);
        // }    
        sleep(3); //为刷子延时
      }
    ros::spinOnce();
  }
  ros::shutdown(); 
  return 0;
}

//一个完整的放置动作
void arm_put(std::string sort)
{
    moveit::planning_interface::MoveGroupInterface arm("arm");
    moveit::planning_interface::MoveGroupInterface hand("hand");
   //根据色块的颜色判断放置位置                       根据实际修改
         if (sort == "recycle") {arm.setNamedTarget("recycle_put");  arm.move();  }//sleep(1);}
    else if (sort == "harm")   {arm.setNamedTarget("other_put");    arm.move();  }//sleep(1);}
    else if (sort == "kitchen")  {arm.setNamedTarget("other_put");   arm.move();  }//sleep(1);}
    else if (sort == "others")  {arm.setNamedTarget("other_put");   arm.move();  ROS_INFO("sorting is done");}//sleep(1);}
    hand.setNamedTarget("hand_open");   //机械爪张开
    
    while( !hand_open_success )  //判断是否规划成功，如果不成功则继续规划
    { 
       hand_open_success = ((hand.move() == moveit::planning_interface::MoveItErrorCode::SUCCESS)); //规划路径
      //  sleep(1);
    }
    hand_open_success=false;

    // arm.setNamedTarget("color_put_interval");   arm.move();  sleep(1);  //机械臂臂身运动到放置色块的预位置
    arm.setNamedTarget("arm_look");   arm.move();  //sleep(1);      //机械臂运动到观测色块的位置
    hand.setNamedTarget("hand_open");  //机械爪张开
    while( !hand_open_success ) //判断是否规划成功，如果不成功则继续规划
    { 
       hand_open_success = ((hand.move() == moveit::planning_interface::MoveItErrorCode::SUCCESS)); //规划路径
      //  sleep(1);
    }
    hand_open_success=false;
    arm_state="working";
}

int multi_grasp_sequence()//多目标抓取
{
  if (isSingle == 1 && isBusy == 1 && count == 1)
    return 1;

  if(isBusy == 1 && j_cb < count && grasp_done == 0)
  {
    ROS_INFO("多目标---->>>抓取");
    joint_target1=cb_target_data[j_cb][0];//关节目标值赋值
    joint_target2=cb_target_data[j_cb][1];
    joint_target3=cb_target_data[j_cb][2];
    if(cb_class[j_cb] == "recycle")
    {
      target_sort="recycle";
      arm_state="ready";
      recycle_count += 1;
      ROS_INFO("sorting is:   recycle");
    }
    else if(cb_class[j_cb] == "harm")
    {
      target_sort="harm";
      arm_state="ready";
      harm_count += 1;
      ROS_INFO("sorting is:   harm");
    }
    else if(cb_class[j_cb] == "kitchen")
    {
      target_sort="kitchen";
      arm_state="ready";
      kitchen_count += 1;
      ROS_INFO("sorting is:   kitchen");
    }
    else if(cb_class[j_cb] == "others")
    {
      target_sort="others";
      arm_state="ready";
      others_count += 1;
      ROS_INFO("sorting is:   others");
    }
    grasp_done=1;
    // ++j_cb;
  }

}



