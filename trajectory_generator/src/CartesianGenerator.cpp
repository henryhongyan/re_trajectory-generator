// Author : Gajan <gajan@ethz.ch>
// Credits: Ruben Smits <ruben.smits@mech.kuleuven.ac.be> (for template)

#include "CartesianGenerator.hpp"
#include <ocl/Component.hpp>
#define SIMULATION 1
ORO_LIST_COMPONENT_TYPE(trajectory_generator::CartesianGenerator);
//ORO_CREATE_COMPONENT(trajectory_generator::CartesianGenerator);

namespace trajectory_generator
{

    using namespace RTT;
    using namespace KDL;
    using namespace std;

    CartesianGenerator::CartesianGenerator(string name)
        : TaskContext(name,PreOperational),
         m_motion_profile(6,VelocityProfile_Trap(0,0))
    {
        //Creating TaskContext
        //Adding Ports
        this->addPort("CartesianPoseMsr",m_position_meas_port);
        this->addPort("CartesianPoseDes",m_position_desi_port);
        this->addPort("CartesianPoseDes2ROS",m_position_desi_port2ROS);
		this->addEventPort("cmdCartPose",cmdCartPose, boost::bind(&CartesianGenerator::generateNewVelocityProfiles, this, _1));        

        //Adding Properties
        this->addProperty("max_vel",m_maximum_velocity).doc("Maximum Velocity in Trajectory");
        this->addProperty("max_acc",m_maximum_acceleration).doc("Maximum Acceleration in Trajectory");

        //Adding Methods
        this->addOperation( "resetPosition",&CartesianGenerator::resetPosition,this,OwnThread).doc("Reset generator's position" );
    }

    CartesianGenerator::~CartesianGenerator()
    {
    }

    bool CartesianGenerator::configureHook()
    {
    	for(unsigned int i=0;i<3;i++){
		  m_motion_profile[i].SetMax(m_maximum_velocity[i],m_maximum_acceleration[i]);
		  m_motion_profile[i+3].SetMax(m_maximum_velocity[i+3],m_maximum_acceleration[i+3]);
		}
		return true;
    }

    bool CartesianGenerator::startHook()
    {
		//initialize
		geometry_msgs::Pose pose;
		m_position_meas_port.read(pose);
		m_position_desi_port.write(pose);
		return true;
    }

    void CartesianGenerator::updateHook()
    {

		m_time_passed = os::TimeService::Instance()->secondsSince(m_time_begin);
		if(motionProfile.size()==(int)3){
			geometry_msgs::Pose pose;
			double theta; Vector3d q;

			pose.position.x=motionProfile[0].Pos(m_time_passed);
			pose.position.y=motionProfile[1].Pos(m_time_passed);
			pose.position.z=motionProfile[2].Pos(m_time_passed);

			//theta = motionProfile[3].Pos(m_time_passed);
			//cout << "--- Theta: " << theta << endl;
			//q = currentRotationalAxis*sin(theta/2);

			//KDL::Rotation errorRotation = KDL::Rotation::Quaternion(q(0), q(1), q(2), cos(theta/2));
			//(errorRotation*m_traject_begin.M).GetQuaternion(pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w);

			pose.orientation.x = 0.0;
			pose.orientation.y = 0.0;
			pose.orientation.z = 0.0;
			pose.orientation.w = 1.0;

			m_position_desi_port.write(pose);


			//TO ROS Visualization
			geometry_msgs::PoseStamped poseStamped;
			poseStamped.header.frame_id = "frame_id_1";
			poseStamped.header.stamp = ros::Time::now();
			poseStamped.pose = pose;
			m_position_desi_port2ROS.write(poseStamped);
#if 1
			std::cout << "DesPosePort   : " << "x:"<< pose.position.x << " y:"<< pose.position.y << " z:"
					<< pose.position.z << std::endl;
			std::cout << "-->Orientation: " << "x:"<< pose.orientation.x << " y:"<< pose.orientation.y
					<< " z:"<< pose.orientation.z << " w:"<< pose.orientation.w << std::endl;
#endif
		}//end of empty motionProfile if check
#if SIMULATION
		else{	//since we can not get the current pose from the Robot
			geometry_msgs::Pose pose;
			pose.position.x=0;
			pose.position.y=0;
			pose.position.z=0.7;
			pose.orientation.x = 0.0;
			pose.orientation.y = 0.0;
			pose.orientation.z = 0.0;
			pose.orientation.w = 1.0;
			m_position_desi_port.write(pose);
		}
#endif
    }

    void CartesianGenerator::stopHook()
    {
    }

    void CartesianGenerator::cleanupHook()
    {
    }

    bool CartesianGenerator::generateNewVelocityProfiles(RTT::base::PortInterface* portInterface){
    	
#if DEBUG
    	std::cout << "A new pose arrived" << std::endl;
#endif
    	m_time_passed = os::TimeService::Instance()->secondsSince(m_time_begin);
    	
    	geometry_msgs::Pose pose;
    	geometry_msgs::PoseStamped poseStamped;
    	cmdCartPose.read(poseStamped);
    	pose = poseStamped.pose;

		m_traject_end.p.x(pose.position.x);
		m_traject_end.p.y(pose.position.y);
		m_traject_end.p.z(pose.position.z);
		m_traject_end.M=Rotation::Quaternion(pose.orientation.x,pose.orientation.y,pose.orientation.z,pose.orientation.w);

		// get current position
		geometry_msgs::Pose pose_meas;
		m_position_meas_port.read(pose_meas);
		m_traject_begin.p.x(pose_meas.position.x);
		m_traject_begin.p.y(pose_meas.position.y);
		m_traject_begin.p.z(pose_meas.position.z);
		m_traject_begin.M=Rotation::Quaternion(pose_meas.orientation.x,pose_meas.orientation.y,pose_meas.orientation.z,pose_meas.orientation.w);

//		KDL::Rotation R1 = m_traject_begin.M;
//		cout <<   " m_traject_begin  " << endl;
//		cout <<  R1(0,0) << " " << R1(0,1) << " " <<R1(0,2) << " " << endl;
//		cout <<  R1(1,0) << " " << R1(1,1) << " " <<R1(1,2) << " " << endl;
//		cout <<  R1(2,0) << " " << R1(2,1) << " " <<R1(2,2) << " " << endl << endl;
//		cout <<   " *********** " << endl;

		KDL::Rotation errorRotation = (m_traject_end.M)*(m_traject_begin.M.Inverse());

		double x,y,z,w;
		errorRotation.GetQuaternion(x,y,z,w);
		currentRotationalAxis[0]=x;
		currentRotationalAxis[1]=y;
		currentRotationalAxis[2]=z;
		currentRotationalAxis.normalize();
		deltaTheta = 2*acos(w);

		std::cout << "-------------------" << std::endl << "currentRotationalAxis: "  << std::endl << currentRotationalAxis << std::endl;
		std::cout << "deltaTheta" << deltaTheta << std::endl;

		std::vector<double> cartPositionCmd = std::vector<double>(3,0.0);
		cartPositionCmd[0] = pose.position.x;
		cartPositionCmd[1] = pose.position.y;
		cartPositionCmd[2] = pose.position.z;

		std::vector<double> cartPositionMsr = std::vector<double>(3,0.0);
		cartPositionMsr[0] = pose_meas.position.x;
		cartPositionMsr[1] = pose_meas.position.y;
		cartPositionMsr[2] = pose_meas.position.z;

		std::vector<double> cartVelocity = std::vector<double>(3,0.0);


		if ((int)motionProfile.size() == 0){//Only for the first run
			for(int i = 0; i < 3; i++)
			{
				cartVelocity[i] = 0.0;
			}
		}else{
			for(int i = 0; i < (int)motionProfile.size(); i++)
			{
				cartVelocity[i] = motionProfile[i].Vel(m_time_passed);
				cartPositionMsr[i] = motionProfile[i].Pos(m_time_passed);
			}
		}

		motionProfile.clear();

		// Set motion profiles
		for(int i = 0; i < 3; i++){
			motionProfile.push_back(VelocityProfile_NonZeroInit(m_maximum_velocity[i], m_maximum_acceleration[i]));
			motionProfile[i].SetProfile(cartPositionMsr[i], cartPositionCmd[i], cartVelocity[i]);
		}
		//motionProfile for theta
		cout << "motionProfile for theta" << endl;
		//motionProfile.push_back(VelocityProfile_NonZeroInit(m_maximum_velocity[3], m_maximum_acceleration[3]));
		//motionProfile[3].SetProfile(0.0,deltaTheta,0.0);
		cout << "motionProfile for theta: Done. Size: " << motionProfile.size() << endl;

		m_time_begin = os::TimeService::Instance()->getTicks();
		m_time_passed = 0;

		return true;
    }

    void CartesianGenerator::resetPosition()
    {
    	std::cout << "resetPosition() called" << std::endl;
    	geometry_msgs::Pose pose;
		m_position_meas_port.read(pose);
		SetToZero(m_velocity_desi_local);
		geometry_msgs::Twist twist;
		twist.linear.x=m_velocity_desi_local.vel.x();
		twist.linear.y=m_velocity_desi_local.vel.y();
		twist.linear.z=m_velocity_desi_local.vel.z();
		twist.angular.x=m_velocity_desi_local.rot.x();
		twist.angular.y=m_velocity_desi_local.rot.y();
		twist.angular.z=m_velocity_desi_local.rot.z();
		m_position_desi_port.write(pose);
		//m_is_moving = false;
    }
}//namespace