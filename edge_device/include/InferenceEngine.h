#ifndef INFERENCE_ENGINE_H
#define INFERENCE_ENGINE_H


#include <tensorflow/lite/interpreter.h>
#include <tensorflow/lite/kernels/register.h>
#include <tensorflow/lite/model.h>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

class InferenceEngine{

//used the tflite namespace and std namespace
//FlatBufferModel refers to our trained model (accessed in read only)
//unique_ptr is a smart pointer that frees the alloted space once the pointer goes out of scope
public:
	int loadModel(std::string model_path);
	std::vector<float> runInference(cv::Mat& input_image);
private:
	std::unique_ptr<tflite::FlatBufferModel> model;
	std::unique_ptr<tflite::Interpreter> interpreter;
	tflite::ops::builtin::BuiltinOpResolver resolver;
};

#endif

