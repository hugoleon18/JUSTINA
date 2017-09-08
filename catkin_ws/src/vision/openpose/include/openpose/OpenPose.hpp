#include <string>

//OpenCV dependencies
#include <opencv2/core/core.hpp>

//3rdparty dependencies
#include <gflags/gflags.h>

//Open poses dependencies
#include <openpose/core/headers.hpp>
#include <openpose/filestream/headers.hpp>
#include <openpose/gui/headers.hpp>
#include <openpose/pose/headers.hpp>
#include <openpose/utilities/headers.hpp>


class OpenPose{
    public:
        OpenPose();
        ~OpenPose();

        void initOpenPose(std::string modelFoler, op::PoseModel modelPose, op::Point<int> netResolution, op::Point<int> outputSize, int numGpuStart, float scaleGap, float scaleNumber, bool disableBlending, float renderThreshold, float alphaPose);
        cv::Mat framePoseEstimation(cv::Mat inputImage);

    private:
        //Atributes Settings for init the openpose library
        std::string modelFoler;
        op::PoseModel modelPose;
        op::Point<int> netResolution;
        op::Point<int> outputSize;
        int numGpuStart;
        float scaleGap;
        float scaleNumber;
        bool disableBlending;
        float renderThreshold;
        float alphaPOse;        

        op::CvMatToOpInput *cvMatToOpInput;
        op::CvMatToOpOutput *cvMatToOpOutput;
        op::PoseExtractorCaffe *poseExtractorCaffe;
        op::PoseRenderer *poseRenderer;
        op::OpOutputToCvMat *opOutputToCvMat;
};
