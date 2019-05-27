#! /usr/bin/env python

import rospy
import actionlib
import dynamic_reconfigure.client

from riptide_msgs.msg import DepthCommand, AttitudeCommand
from geometry_msgs.msg import Vector3
from std_msgs.msg import Float32
import riptide_controllers.msg

import time
import math
import yaml

class CalibrationAction(object):

    def __init__(self):
        self.depthPub = rospy.Publisher("/command/depth", DepthCommand, queue_size=1)  
        self.attitudePub = rospy.Publisher("/command/attitude", AttitudeCommand, queue_size=1)
        self._as = actionlib.SimpleActionServer("calibration_action", riptide_controllers.msg.CalibrationAction, execute_cb=self.execute_cb, auto_start=False)
        self._as.start()

      
    def execute_cb(self, goal):
        client = dynamic_reconfigure.client.Client("thruster_controller", timeout=30)

        with open(rospy.get_param('~properties_file'), 'r') as stream:
            mass = yaml.safe_load(stream)['properties']['mass']

        Fb = mass * 9.81
        CobX = 0
        CobY = 0
        CobZ = 0

        client.update_configuration({"Buoyant_Force": Fb, "Buoyancy_X_POS": CobX, "Buoyancy_Y_POS": CobY, "Buoyancy_Z_POS": CobZ})

        self.depthPub.publish(True, 1)
        att = AttitudeCommand()
        att.roll_active = True
        att.pitch_active = True
        att.euler_rpy.x = 0
        att.euler_rpy.y = 0
        self.attitudePub.publish(att)

        rospy.sleep(3)

        for i in range(1, 20):
            rospy.sleep(0.5)
            forceMsg = rospy.wait_for_message("/command/force_depth", Vector3)
            force = math.sqrt(forceMsg.x**2 + forceMsg.y**2 + forceMsg.z**2)

            if forceMsg.z < 0:
                force *= -1

            Fb += force * 0.2
            client.update_configuration({"Buoyant_Force": Fb, "Buoyancy_X_POS": CobX, "Buoyancy_Y_POS": CobY, "Buoyancy_Z_POS": CobZ})


        for i in range(1, 20):
            rospy.sleep(0.5)
            momentMsg = rospy.wait_for_message("/command/moment", Vector3Stamped)
            
            CobY += momentMsg.vector.X / Fb * 0.2
            CobX += momentMsg.vector.Y / Fb * 0.2

            client.update_configuration({"Buoyant_Force": Fb, "Buoyancy_X_POS": CobX, "Buoyancy_Y_POS": CobY, "Buoyancy_Z_POS": CobZ})

        att.euler_rpy.x = 90
        self.attitudePub.publish(att)

        rospy.sleep(3)

        for i in range(1, 20):
            rospy.sleep(0.5)
            momentMsg = rospy.wait_for_message("/command/moment", Vector3Stamped)
            
            CobZ -= momentMsg.vector.X / Fb * 0.2

            client.update_configuration({"Buoyant_Force": Fb, "Buoyancy_X_POS": CobX, "Buoyancy_Y_POS": CobY, "Buoyancy_Z_POS": CobZ})

            

        
        self._as.set_succeeded()

        
        
if __name__ == '__main__':
    rospy.init_node('calibration')
    server = CalibrationAction()
    rospy.spin()