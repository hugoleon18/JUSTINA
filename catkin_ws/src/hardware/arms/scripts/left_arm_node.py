#!/usr/bin/env python
import sys
import rospy
import Dynamixel
from std_msgs.msg import Float32MultiArray
from std_msgs.msg import Float32
from geometry_msgs.msg import TransformStamped
from sensor_msgs.msg import JointState
import tf

def printHelp():
    print "LEFT ARM NODE BY MARCOSOfT. Options:"

def callbackPos(msg):
    global dynMan1

    Pos = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]
    goalPos = [0, 0, 0, 0, 0, 0, 0, 0, 0]

    ### Read the data of publisher
    for i in range(len(Pos)):
        Pos[i] = msg.data[i]

    # Conversion float to int for registers
    goalPos[0] = int(( 1530 - (Pos[0])/(360.0/4095.0*3.14159265358979323846/180.0) )  )
    goalPos[1] = int((  (Pos[1])/(360.0/4095.0*3.14159265358979323846/180.0) ) + 2107 )
    goalPos[2] = int(( 2048 - (Pos[2])/(360.0/4095.0*3.14159265358979323846/180.0) )  )
    goalPos[3] = int((  (Pos[3])/(360.0/4095.0*3.14159265358979323846/180.0) ) + 2102 )
    goalPos[4] = int(( 2048 - (Pos[4])/(360.0/4095.0*3.14159265358979323846/180.0) )  )
    goalPos[5] = int((  (Pos[5])/(360.0/4095.0*3.14159265358979323846/180.0) ) + 2068 )
    goalPos[6] = int(( 1924 - (Pos[6])/(360.0/4095.0*3.14159265358979323846/180.0) ) )
    #goalPos[7] = int((  (Pos[7])/(360.0/4095.0*3.14159265358979323846/180.0) ) + 1400 )
    #goalPos[8] = int((  (Pos[8])/(360.0/4095.0*3.14159265358979323846/180.0) ) + 1295 )


    ### Set Servomotors Torque Enable
    for i in range(len(Pos)):
        dynMan1.SetTorqueEnable(i, 1)


    ### Set Servomotors Speeds
    for i in range(len(Pos)):
        dynMan1.SetMovingSpeed(i, 100)


    ### Set GoalPosition
    for i in range(len(Pos)):
        dynMan1.SetGoalPosition(i, goalPos[i])

    
def main(portName1, portBaud1):
    print "INITIALIZING LEFT ARM NODE BY MARCOSOFT..."
    
    ###Communication with dynamixels:
    global dynMan1 
    dynMan1 = Dynamixel.DynamixelMan(portName1, portBaud1)
    
    ###Connection with ROS
    rospy.init_node("left_arm")
    br = tf.TransformBroadcaster()
    jointStates = JointState()
    jointStates.name = ["la_1_joint", "la_2_joint", "la_3_joint", "la_4_joint", "la_5_joint", "la_6_joint", "la_7_joint"]
    jointStates.position = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

    subPos = rospy.Subscriber("/hardware/left_arm/goal_pose", Float32MultiArray, callbackPos)
    pubJointStates = rospy.Publisher("/joint_states", JointState, queue_size = 1)
    pubArmPose = rospy.Publisher("left_arm/current_pose", Float32MultiArray, queue_size = 1)
    pubGripper = rospy.Publisher("left_arm/current_gripper", Float32, queue_size = 1)
    
    dynMan1.SetTorqueEnable(4, 1)
    dynMan1.SetMovingSpeed(4, 100)
    dynMan1.SetGoalPosition(4, 2050)

    
    loop = rospy.Rate(10)

    msgCurrentPose = Float32MultiArray()
    msgCurrentPose.data = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0] 
    msgCurrentGripper = Float32()
    curretPos = [0,0,0,0,0,0,0,0]
    bitsPerRadian = (4095)/((360)*(3.141592/180)) 

    while not rospy.is_shutdown():
        #print str(dynMan1.GetMovingSpeed(0))
        #for i in range(len(curretPos)):
        #    curretPos[i]= dynMan1.GetPresentPosition(i)
        #print "Poses: " + str(curretPos[0]) + " "+ str(curretPos[1]) + " "+ str(curretPos[2]) + " "+ str(curretPos[3]) + " "+ str(curretPos[4]) + " "+ str(curretPos[5]) + " "+ str(curretPos[6])+ " " + str(curretPos[7])


        bitsPosition = dynMan1.GetPresentPosition(0)
        pos0 = float( (2094-bitsPosition)/bitsPerRadian)
        pos1 = float(-(3127-dynMan1.GetPresentPosition(1))/bitsPerRadian)
        pos2 = float(-(1798-dynMan1.GetPresentPosition(2))/bitsPerRadian)
        pos3 = float(-(1997-dynMan1.GetPresentPosition(3))/bitsPerRadian)
        pos4 = float(-(2050-dynMan1.GetPresentPosition(4))/bitsPerRadian)
        pos5 = float(-(1774-dynMan1.GetPresentPosition(5))/bitsPerRadian)
        pos6 = float(-(2048-dynMan1.GetPresentPosition(6))/bitsPerRadian)
        #posD21 = float((1400-dynMan1.GetPresentPosition(7))/bitsPerRadian)
        #posD22 = float((1295-dynMan1.GetPresentPosition(8))/bitsPerRadian)
        
        #print "Poses: " + str(pos0) + "  " + str(pos1) + "  " + str(pos2) + "  " + str(pos3) + "  " + str(pos4) + "  " + str(pos5) + "  " + str(pos6) + "  " + str(posD21) + "  " + str(posD22)
        jointStates.header.stamp = rospy.Time.now()
        jointStates.position[0] = pos0
        jointStates.position[1] = pos1
        jointStates.position[2] = pos2
        jointStates.position[3] = pos3
        jointStates.position[4] = pos4
        jointStates.position[5] = pos5
        jointStates.position[6] = pos6
        msgCurrentPose.data[0] = pos0
        msgCurrentPose.data[1] = pos1
        msgCurrentPose.data[2] = pos2
        msgCurrentPose.data[3] = pos3
        msgCurrentPose.data[4] = pos4
        msgCurrentPose.data[5] = pos5
        msgCurrentPose.data[6] = pos6
        msgCurrentGripper.data = posD22
        pubJointStates.publish(jointStates)
        pubArmPose.publish(msgCurrentPose)
        pubGripper.publish(msgCurrentGripper)
        loop.sleep()

if __name__ == '__main__':
    try:
        if "--help" in sys.argv:
            printHelp()
        elif "-h" in sys.argv:
            printHelp()
        else:
            portName1 = "/dev/ttyUSB1"
            portBaud1 = 57600
            if "--port1" in sys.argv:
                portName1 = sys.argv[sys.argv.index("--port1") + 1]
            if "--port2" in sys.argv:
                portName2 = sys.argv[sys.argv.index("--port2") + 1]
            if "--baud1" in sys.argv:
                portBaud1 = int(sys.argv[sys.argv.index("--baud1") + 1])
            if "--baud2" in sys.argv:
                portBaud2 = int(sys.argv[sys.argv.index("--baud2") + 1])
            main(portName1, portBaud1)
    except rospy.ROSInterruptException:
        pass
